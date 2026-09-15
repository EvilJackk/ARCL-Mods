//------------------------------------------------------------------------------------------------
// ARCL Freeze - Character Controller
//
// Movement and view are locked through the engine's own per-frame poll events
// (SCR_GetDisableMovementControls / SCR_GetDisableViewControls). Because the engine asks script
// every frame, this cannot drift out of sync with menus, vehicle transitions or respawns - and it
// sits BELOW the key-binding layer, so rebinding keys cannot defeat it.
//
// Weapons have no poll event; they are driven from ARCLFreeze_ManagerComponent.Tick_ARCLFreeze().
//------------------------------------------------------------------------------------------------

modded class SCR_CharacterControllerComponent
{
	//------------------------------------------------------------------------------------------------
	//! Engine poll. Vanilla uses this for loitering animations; we OR the freeze into it.
	override bool SCR_GetDisableMovementControls()
	{
		if (ARCLFreeze_ShouldLock())
			return true;

		return super.SCR_GetDisableMovementControls();
	}

	//------------------------------------------------------------------------------------------------
	//! Engine poll. Covers turning, look and free look.
	//!
	//! Returns false rather than calling super deliberately. Vanilla SCR_CharacterControllerComponent
	//! does NOT implement this event at 1.7.0.54 - only the bodyless declaration on
	//! CharacterControllerComponent exists - so super's return value is undefined. Since this poll
	//! runs every frame and decides whether the player has a working mouse, an undefined value here
	//! is not something to gamble on.
	override bool SCR_GetDisableViewControls()
	{
		return ARCLFreeze_ShouldLock() && ARCLFreeze_ManagerComponent.ShouldFreezeView();
	}

	//------------------------------------------------------------------------------------------------
	//! Vanilla reads "GetOut" / "JumpOut" straight off the ActionManager here, which never consults
	//! the disable flags - so without this a frozen player could still bail out of a vehicle.
	//! Skipping super also suppresses prone roll and weapon magnification, which is what we want.
	override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
	{
		// ARCLFreeze_ShouldLock, not IsLocalFrozen: on a listen server this event also runs for
		// other players' characters, and gating those on the HOST's exempt status would be wrong.
		if (player && ARCLFreeze_ShouldLock())
			return;

		super.OnPrepareControls(owner, am, dt, player);
	}

	//------------------------------------------------------------------------------------------------
	//! True only for the locally controlled player character, and only when that player is not
	//! exempt. Remote proxies and AI are left alone.
	protected bool ARCLFreeze_ShouldLock()
	{
		// Cheap static field read first - this runs every frame, on every character.
		if (!ARCLFreeze_ManagerComponent.IsFrozen())
			return false;

		if (GetOwner() != SCR_PlayerController.GetLocalControlledEntity())
			return false;

		return ARCLFreeze_ManagerComponent.IsLocalFrozen();
	}
}
