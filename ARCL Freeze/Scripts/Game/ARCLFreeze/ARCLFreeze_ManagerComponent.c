//------------------------------------------------------------------------------------------------
// ARCL Freeze - Manager Component
//
// Attach to the game mode entity. Holds the single replicated freeze flag and drives every layer.
//
// Architecture mirrors vanilla SCR_BaseGameMode.m_bAllowControls:
//   - authority decides and flips an RplProp bool
//   - the flag streams to every client (JIP included)
//   - every machine re-asserts its local lockout each frame
// The per-frame re-assert is what makes this survive menus, vehicle transitions and respawns.
//
// Two independent transitions are tracked, and keeping them separate matters:
//   WORLD state  (m_bFrozen)        -> vehicles on every machine, AI on the server
//   LOCAL lock   (frozen && !exempt) -> VON, banner, weapon re-assert
// A Game Master opening the editor flips the LOCAL lock without touching the WORLD state.
//------------------------------------------------------------------------------------------------

class ARCLFreeze_ManagerComponentClass : SCR_BaseGameModeComponentClass
{
}

class ARCLFreeze_ManagerComponent : SCR_BaseGameModeComponent
{
	[Attribute("1", desc: "Exempt players who are currently in Game Master", category: "ARCL Freeze")]
	protected bool m_bExemptGameMasters;

	[Attribute("1", desc: "Also exempt server / session administrators, even when not in Game Master", category: "ARCL Freeze")]
	protected bool m_bExemptAdmins;

	[Attribute("1", desc: "Freeze view / look controls as well as movement", category: "ARCL Freeze")]
	protected bool m_bFreezeView;

	[Attribute("1", desc: "Disable voice chat while frozen", category: "ARCL Freeze")]
	protected bool m_bDisableVON;

	[Attribute("1", desc: "Deactivate AI agents while frozen (server side)", category: "ARCL Freeze")]
	protected bool m_bFreezeAI;

	[Attribute("1", desc: "Show the on-screen banner to frozen players", category: "ARCL Freeze")]
	protected bool m_bShowBanner;

	[Attribute("MATCH PAUSED - STAND BY", desc: "Banner text shown to frozen players", category: "ARCL Freeze")]
	protected string m_sBannerText;

	//! Banner tuning. 48 overflowed 1080p in testing.
	protected const int BANNER_FONT_SIZE_ARCLFREEZE = 38;
	protected const int BANNER_HEIGHT_ARCLFREEZE = 80;

	//! The one replicated bit of state. Authority writes, everyone reads.
	[RplProp(onRplName: "OnFrozenChanged_ARCLFreeze")]
	protected bool m_bFrozen;

	protected static ARCLFreeze_ManagerComponent s_Instance;

	//! Agents this mod deactivated, so unfreeze only wakes what it put to sleep. Server only.
	protected ref array<AIAgent> m_aFrozenAgents_ARCLFreeze = {};

	//! Local VON state captured at freeze time.
	protected bool m_bPrevVONDisabled_ARCLFreeze;
	protected bool m_bVONTouched_ARCLFreeze;

	//! Edge detection. World state drives vehicles/AI; local lock drives VON/banner/weapons.
	protected bool m_bWorldStateApplied_ARCLFreeze;
	protected bool m_bLocalLockActive_ARCLFreeze;

	protected TextWidget m_wBanner_ARCLFreeze;

	//------------------------------------------------------------------------------------------------
	static ARCLFreeze_ManagerComponent GetInstance()
	{
		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	//! Hot path - called every frame from the character control hooks. Plain field read.
	static bool IsFrozen()
	{
		return s_Instance && s_Instance.m_bFrozen;
	}

	//------------------------------------------------------------------------------------------------
	//! True when the freeze is on AND this machine has a local player who is not exempt.
	//! Returns false on a dedicated server, which has no local player at all.
	static bool IsLocalFrozen()
	{
		if (!s_Instance || !s_Instance.m_bFrozen)
			return false;

		if (!s_Instance.HasLocalPlayer_ARCLFreeze())
			return false;

		return !ARCLFreeze_Util.IsLocalExempt(s_Instance.m_bExemptGameMasters, s_Instance.m_bExemptAdmins);
	}

	//------------------------------------------------------------------------------------------------
	static bool ShouldFreezeView()
	{
		return s_Instance && s_Instance.m_bFreezeView;
	}

	//------------------------------------------------------------------------------------------------
	bool IsFrozenState()
	{
		return m_bFrozen;
	}

	//------------------------------------------------------------------------------------------------
	//! A dedicated server has no PlayerController and no workspace. Everything player-facing has to
	//! be gated on this or the server ends up "locally frozen" with nothing to freeze.
	protected bool HasLocalPlayer_ARCLFreeze()
	{
		ArmaReforgerScripted game = GetGame();
		if (!game)
			return false;

		return game.GetPlayerController() != null;
	}

	//------------------------------------------------------------------------------------------------
	// Constructor - runs before any other component's OnPostInit, closing the window where
	// VehicleControllerComponent.OnPostInit could query IsFrozen() and find a null instance.
	// Duplicate guard copied from vanilla SCR_AdditionalGameModeSettingsComponent.
	//------------------------------------------------------------------------------------------------
	void ARCLFreeze_ManagerComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (s_Instance)
		{
			Print(ARCLFreeze_Util.TAG + " ARCLFreeze_ManagerComponent exists twice in the world - the second one is ignored. Remove the duplicate from your game mode prefab.", LogLevel.WARNING);
			return;
		}

		s_Instance = this;
	}

	//------------------------------------------------------------------------------------------------
	//! Fires unconditionally once all components are initialised - no event mask required.
	override protected void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (s_Instance != this)
			return;

		string role = "CLIENT";
		if (Replication.IsServer())
			role = "SERVER";

		Print("------------------------------------------------------------", LogLevel.NORMAL);
		Print(string.Format("%1 v%2 loaded on %3", ARCLFreeze_Util.TAG, ARCLFreeze_Util.VERSION, role), LogLevel.NORMAL);
		Print(string.Format("%1   exempt GM: %2 | exempt admins: %3 | freeze view: %4", ARCLFreeze_Util.TAG, m_bExemptGameMasters, m_bExemptAdmins, m_bFreezeView), LogLevel.NORMAL);
		Print(string.Format("%1   disable VON: %2 | freeze AI: %3 | banner: %4", ARCLFreeze_Util.TAG, m_bDisableVON, m_bFreezeAI, m_bShowBanner), LogLevel.NORMAL);
		Print(string.Format("%1   commands: #freeze | #unfreeze", ARCLFreeze_Util.TAG), LogLevel.NORMAL);
		Print("------------------------------------------------------------", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Clear the singleton and tear down the banner. Without this a mission restart leaves a stale
	//! static pointing at a dead component, and a leaked full-screen widget behind.
	override protected void OnDelete(IEntity owner)
	{
		DestroyBanner_ARCLFreeze();

		if (s_Instance == this)
			s_Instance = null;

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Authority entry point. Route the chat command through here.
	//! \param frozen Target state
	void SetFrozen(bool frozen)
	{
		if (!Replication.IsServer())
		{
			ARCLFreeze_Util.Log("SetFrozen called on a client - ignored", LogLevel.WARNING);
			return;
		}

		if (m_bFrozen == frozen)
			return;

		m_bFrozen = frozen;
		Replication.BumpMe();

		// onRplName does not fire on the authority, and on a listen server the host is also a
		// client - so drive the apply path by hand.
		OnFrozenChanged_ARCLFreeze();

		ARCLFreeze_Util.Log(string.Format("World freeze %1", frozen));
	}

	//------------------------------------------------------------------------------------------------
	//! Replication callback on proxies, and called manually on the authority.
	//! Tick_ARCLFreeze is idempotent, so applying on the edge is just an early call of the same path.
	protected void OnFrozenChanged_ARCLFreeze()
	{
		Tick_ARCLFreeze();
	}

	//------------------------------------------------------------------------------------------------
	//! Called every frame from the modded game mode. Idempotent - safe to call from the edge too.
	void Tick_ARCLFreeze()
	{
		// ---- WORLD state: vehicles everywhere, AI on the server ----
		if (m_bFrozen != m_bWorldStateApplied_ARCLFreeze)
		{
			m_bWorldStateApplied_ARCLFreeze = m_bFrozen;

			ApplyVehicles_ARCLFreeze(m_bFrozen);

			if (m_bFreezeAI && Replication.IsServer())
				ApplyAI_ARCLFreeze(m_bFrozen);
		}

		// ---- LOCAL lock: this player only ----
		bool affected = IsLocalFrozen();

		if (affected != m_bLocalLockActive_ARCLFreeze)
		{
			m_bLocalLockActive_ARCLFreeze = affected;

			ApplyVON_ARCLFreeze(affected);
			UpdateBanner_ARCLFreeze(affected);

			if (!affected)
				ReleaseLocalControls_ARCLFreeze();
		}

		if (!affected)
		{
			// Exempt (Game Master / admin) while the world is still frozen.
			//
			// Vanilla is asymmetric about control restoration:
			//   SCR_BaseGameMode.SetLocalControls  -> re-asserts MOVEMENT + WEAPONS every frame
			//   SCR_PlayerController.SetDisableControls -> writes VIEW, but only edge-triggered on
			//                                             MenuManager.IsAnyMenuOpen() changing
			//
			// Nothing re-asserts view. So if view gets latched off during the transition into Game
			// Master, movement comes back every frame and the mouse never does - WASD works, look
			// does not. That is exactly the reported bug. Clear it actively.
			if (m_bFrozen)
				ClearStuckViewControls_ARCLFreeze();

			return;
		}

		SCR_CharacterControllerComponent controller = GetLocalCharacterController_ARCLFreeze();
		if (!controller)
			return;

		// No engine poll exists for weapon controls, so drive the setter every frame - exactly
		// what vanilla SCR_BaseGameMode.SetLocalControls does. Ours runs after super.EOnFrame,
		// so it wins the tie.
		controller.SetDisableWeaponControls(true);

		// Belt and braces on alt-look. SCR_GetDisableViewControls already covers it.
		if (m_bFreezeView)
			controller.SetFreeLook(false, false, false);
	}

	//------------------------------------------------------------------------------------------------
	// Vehicles - applied on EVERY machine.
	//
	// The driver's client owns its vehicle, so relying on the authority alone would leave the one
	// machine that matters unblocked if the controller state does not replicate. Each machine
	// snapshots and restores its own view of each vehicle, which stays consistent because the
	// snapshot lives on the component instance.
	//------------------------------------------------------------------------------------------------
	protected void ApplyVehicles_ARCLFreeze(bool frozen)
	{
		array<VehicleControllerComponent> controllers = VehicleControllerComponent.ARCLFreeze_GetAll();
		if (!controllers)
			return;

		int count = 0;
		foreach (VehicleControllerComponent controller : controllers)
		{
			if (!controller)
				continue;

			controller.ARCLFreeze_SetFrozen(frozen);
			count++;
		}

		ARCLFreeze_Util.Log(string.Format("%1 vehicle controller(s) -> frozen=%2", count, frozen));
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyAI_ARCLFreeze(bool frozen)
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		array<AIAgent> agents = {};
		aiWorld.GetAIAgents(agents);

		if (frozen)
		{
			m_aFrozenAgents_ARCLFreeze.Clear();

			foreach (AIAgent agent : agents)
			{
				if (!agent || !agent.IsAIActivated())
					continue;

				// Only record agents that were actually running. Vanilla deactivates AI for its own
				// reasons (delayed spawn, player-led groups) and those must stay asleep.
				agent.DeactivateAI();
				m_aFrozenAgents_ARCLFreeze.Insert(agent);
			}

			ARCLFreeze_Util.Log(string.Format("%1 AI agent(s) deactivated", m_aFrozenAgents_ARCLFreeze.Count()));
		}
		else
		{
			// Walk the LIVE agent list and only touch ones we put to sleep. Stored pointers are
			// compared, never dereferenced, so an agent deleted mid-freeze is simply skipped.
			int restored = 0;
			foreach (AIAgent agent : agents)
			{
				if (!agent)
					continue;

				if (m_aFrozenAgents_ARCLFreeze.Find(agent) == -1)
					continue;

				agent.ActivateAI();
				restored++;
			}

			m_aFrozenAgents_ARCLFreeze.Clear();
			ARCLFreeze_Util.Log(string.Format("%1 AI agent(s) reactivated", restored));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! VON uses the controller's own switch rather than an input context, because a context blanket
	//! has to stand down while a menu or chat is open - and VON would come back in that window.
	protected void ApplyVON_ARCLFreeze(bool disable)
	{
		if (!m_bDisableVON || !HasLocalPlayer_ARCLFreeze())
			return;

		SCR_VONController von = SCR_VONController.Cast(GetGame().GetPlayerController().FindComponent(SCR_VONController));
		if (!von)
			return;

		if (disable)
		{
			if (m_bVONTouched_ARCLFreeze)
				return;

			m_bPrevVONDisabled_ARCLFreeze = von.IsVONDisabled();
			m_bVONTouched_ARCLFreeze = true;
			von.SetVONDisabled(true);
		}
		else
		{
			if (!m_bVONTouched_ARCLFreeze)
				return;

			// Restore what they had. A player already VON-disabled (unconscious, muted) must not be
			// handed voice back by the unfreeze.
			von.SetVONDisabled(m_bPrevVONDisabled_ARCLFreeze);
			m_bVONTouched_ARCLFreeze = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ReleaseLocalControls_ARCLFreeze()
	{
		SCR_CharacterControllerComponent controller = GetLocalCharacterController_ARCLFreeze();
		if (!controller)
			return;

		controller.SetDisableWeaponControls(false);

		// Clear view on the release edge too, so an exempt player gets their mouse back immediately
		// rather than waiting for the next per-frame sweep.
		controller.SetDisableViewControls(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Per-frame sweep for exempt players during a freeze. Only writes when view is actually stuck,
	//! and never while a real menu is open - a menu legitimately wants view disabled, and vanilla
	//! will restore it on the menu's own close edge.
	protected void ClearStuckViewControls_ARCLFreeze()
	{
		if (!HasLocalPlayer_ARCLFreeze())
			return;

		if (GetGame().GetMenuManager() && GetGame().GetMenuManager().IsAnyMenuOpen())
			return;

		SCR_CharacterControllerComponent controller = GetLocalCharacterController_ARCLFreeze();
		if (!controller)
			return;

		if (controller.GetDisableViewControls())
			controller.SetDisableViewControls(false);
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_CharacterControllerComponent GetLocalCharacterController_ARCLFreeze()
	{
		IEntity controlled = SCR_PlayerController.GetLocalControlledEntity();
		if (!controlled)
			return null;

		ChimeraCharacter character = ChimeraCharacter.Cast(controlled);
		if (!character)
			return null;

		return SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
	}

	//------------------------------------------------------------------------------------------------
	// Banner
	//
	// Script-created so v1 needs no .layout file.
	//
	// IGNORE_CURSOR | NOFOCUS are NOT optional: without them this full-width widget swallows the
	// mouse, which breaks cursor interaction in Game Master. Vanilla's own SCR_ManualCamera creates
	// its overlay with exactly these flags for the same reason.
	// CENTER | VCENTER do the alignment - TextWidget has no align setter, but the widget flags do it.
	//------------------------------------------------------------------------------------------------
	protected void UpdateBanner_ARCLFreeze(bool show)
	{
		if (!show)
		{
			DestroyBanner_ARCLFreeze();
			return;
		}

		if (!m_bShowBanner || m_wBanner_ARCLFreeze || !HasLocalPlayer_ARCLFreeze())
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		if (!workspace)
			return;

		// GetWidth/GetHeight return PHYSICAL pixels, but widget coordinates are in REFERENCE
		// resolution. Without DPIUnscale the strip is wider than the screen at any DPI scale != 1,
		// and CENTER then lands the text right of true centre. Vanilla does the same conversion in
		// SCR_TeleportToCursorManualCameraComponent and SCR_BaseContextMenuEditorUIComponent.
		int screenW = workspace.DPIUnscale(workspace.GetWidth());
		int screenH = workspace.DPIUnscale(workspace.GetHeight());
		if (screenW <= 0 || screenH <= 0)
			return;

		// Full-width strip a quarter of the way down: reads as centred without covering the crosshair.
		m_wBanner_ARCLFreeze = TextWidget.Cast(workspace.CreateWidgetInWorkspace(
			WidgetType.TextWidgetTypeID,
			0, screenH / 4, screenW, BANNER_HEIGHT_ARCLFREEZE,
			WidgetFlags.VISIBLE | WidgetFlags.BLEND | WidgetFlags.CENTER | WidgetFlags.VCENTER | WidgetFlags.IGNORE_CURSOR | WidgetFlags.NOFOCUS,
			new Color(1.0, 0.85, 0.1, 1.0), 1024));

		if (!m_wBanner_ARCLFreeze)
			return;

		m_wBanner_ARCLFreeze.SetExactFontSize(BANNER_FONT_SIZE_ARCLFREEZE);
		m_wBanner_ARCLFreeze.SetOutline(2, 0xFF000000);   // readable against sky
		m_wBanner_ARCLFreeze.SetText(m_sBannerText);
	}

	//------------------------------------------------------------------------------------------------
	protected void DestroyBanner_ARCLFreeze()
	{
		if (!m_wBanner_ARCLFreeze)
			return;

		m_wBanner_ARCLFreeze.RemoveFromHierarchy();
		m_wBanner_ARCLFreeze = null;
	}
}
