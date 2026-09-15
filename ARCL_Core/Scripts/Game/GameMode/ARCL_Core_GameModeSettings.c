modded class SCR_BaseGameMode : BaseGameMode
{
	protected static const string ARCL_SETTINGS_PATH = "$profile:ARCL_Core.json";
	protected static const int ARCL_MIN_DISTANT_SHADOWS_QUALITY = 1;
	protected static const int ARCL_MIN_GRASS_DISTANCE = 150;
	protected static const int ARCL_MIN_GRASS_LOD = 2;
	protected static const int ARCL_REQUIRED_SSDO = 1;
	protected static const int ARCL_SHADOW_ENFORCE_INTERVAL_MS = 30000;
	protected static const int ARCL_DEPLOYABLE_POLICY_DELAY_MS = 5000;

	[RplProp()]
	protected bool m_bARCL_FirstPersonOnFoot = true;

	[RplProp()]
	protected bool m_bARCL_ThirdPersonInVehicles = true;

	[RplProp()]
	protected bool m_bARCL_RemoveBuildRankRequirements = true;

	[RplProp()]
	protected bool m_bARCL_RevertEnemyRadios = true;

	[RplProp()]
	protected bool m_bARCL_IndestructibleBuildables = true;

	[RplProp()]
	protected bool m_bARCL_ForceDistantShadows = true;

	[RplProp()]
	protected bool m_bARCL_ForceGrassSettings = true;

	[RplProp()]
	protected bool m_bARCL_ForceContactShadows = true;

	[RplProp()]
	protected bool m_bARCL_SpawnSupplyCostEnabled = true;

	[RplProp()]
	protected int m_iARCL_SpawnSupplyCostMinimum = 100;

	[RplProp()]
	protected int m_iARCL_SpawnSupplyCostMaximum = 100;

	[RplProp()]
	protected bool m_bARCL_SpawnSupplyCostChargeAtHQ = false;

	[RplProp()]
	protected bool m_bARCL_SpawnSupplyCostLog = false;

	[RplProp()]
	protected bool m_bARCL_SpawnSupplyGate = true;

	[RplProp()]
	protected bool m_bARCL_SpawnSupplyGateOnDeployables = false;

	[RplProp()]
	protected bool m_bARCL_DisableDeployableSpawnPoints = false;

	bool ARCL_IsFirstPersonOnFoot()
	{
		return m_bARCL_FirstPersonOnFoot;
	}

	bool ARCL_IsThirdPersonInVehicles()
	{
		return m_bARCL_ThirdPersonInVehicles;
	}

	bool ARCL_IsRemoveBuildRankRequirements()
	{
		return m_bARCL_RemoveBuildRankRequirements;
	}

	bool ARCL_IsRevertEnemyRadios()
	{
		return m_bARCL_RevertEnemyRadios;
	}

	bool ARCL_IsIndestructibleBuildables()
	{
		return m_bARCL_IndestructibleBuildables;
	}

	bool ARCL_IsForceDistantShadows()
	{
		return m_bARCL_ForceDistantShadows;
	}

	bool ARCL_IsForceGrassSettings()
	{
		return m_bARCL_ForceGrassSettings;
	}

	bool ARCL_IsForceContactShadows()
	{
		return m_bARCL_ForceContactShadows;
	}

	bool ARCL_IsSpawnSupplyCostEnabled()
	{
		return m_bARCL_SpawnSupplyCostEnabled;
	}

	float ARCL_GetSpawnSupplyCostMinimum()
	{
		return m_iARCL_SpawnSupplyCostMinimum;
	}

	float ARCL_GetSpawnSupplyCostMaximum()
	{
		return m_iARCL_SpawnSupplyCostMaximum;
	}

	bool ARCL_IsSpawnSupplyCostChargedAtHQ()
	{
		return m_bARCL_SpawnSupplyCostChargeAtHQ;
	}

	bool ARCL_IsSpawnSupplyCostLogged()
	{
		return m_bARCL_SpawnSupplyCostLog;
	}

	bool ARCL_IsSpawnSupplyGateEnabled()
	{
		return m_bARCL_SpawnSupplyGate;
	}

	bool ARCL_IsSpawnSupplyGateOnDeployables()
	{
		return m_bARCL_SpawnSupplyGateOnDeployables;
	}

	bool ARCL_IsDisableDeployableSpawnPoints()
	{
		return m_bARCL_DisableDeployableSpawnPoints;
	}

	bool ARCL_IsForceVideoSettings()
	{
		return m_bARCL_ForceDistantShadows || m_bARCL_ForceGrassSettings || m_bARCL_ForceContactShadows;
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		GetGame().GetCallqueue().CallLater(ARCL_EnforceVideoSettings, ARCL_SHADOW_ENFORCE_INTERVAL_MS, true);
		ARCL_EnforceVideoSettings();

		if (!Replication.IsServer())
			return;

		ARCL_LoadSettings();

		GetGame().GetCallqueue().CallLater(ARCL_ApplyDeployableSpawnPointPolicy, ARCL_DEPLOYABLE_POLICY_DELAY_MS, false);
	}

	protected void ARCL_ApplyDeployableSpawnPointPolicy()
	{
		if (!ARCL_IsDisableDeployableSpawnPoints())
			return;

		SCR_PlayerSpawnPointManagerComponent spawnPointManager = SCR_PlayerSpawnPointManagerComponent.Cast(FindComponent(SCR_PlayerSpawnPointManagerComponent));
		if (!spawnPointManager)
		{
			Print("[ARCL_Core] No SCR_PlayerSpawnPointManagerComponent on the game mode -- deployable spawn points left as authored", LogLevel.WARNING);
			return;
		}

		if (!spawnPointManager.IsDeployingSpawnPointsEnabled())
		{
			Print("[ARCL_Core] Deployable spawn points already disabled by the world");
			return;
		}

		spawnPointManager.EnableDeployableSpawnPoints(false);

		Print("[ARCL_Core] Deployable spawn points disabled by disableDeployableSpawnPoints");
	}

	void ARCL_MakeCompositionIndestructible(IEntity root)
	{
		if (!root)
			return;

		SCR_DestructionDamageManagerComponent destruction = SCR_DestructionDamageManagerComponent.Cast(root.FindComponent(SCR_DestructionDamageManagerComponent));
		if (destruction)
			destruction.EnableDamageHandling(false);

		IEntity child = root.GetChildren();
		while (child)
		{
			ARCL_MakeCompositionIndestructible(child);
			child = child.GetSibling();
		}
	}

	void ARCL_EnforceVideoSettings()
	{
		ARCL_ApplyVideoMinimums();
	}

	bool ARCL_ApplyVideoMinimums()
	{
		UserSettings engineUserSettings = GetGame().GetEngineUserSettings();
		if (!engineUserSettings)
			return false;

		bool changed = false;

		if (m_bARCL_ForceDistantShadows)
		{
			BaseContainer videoSettings = engineUserSettings.GetModule("VideoUserSettings");
			int distantShadowsQuality;
			if (videoSettings && videoSettings.Get("DistantShadowsQuality", distantShadowsQuality) && distantShadowsQuality < ARCL_MIN_DISTANT_SHADOWS_QUALITY)
			{
				videoSettings.Set("DistantShadowsQuality", ARCL_MIN_DISTANT_SHADOWS_QUALITY);
				changed = true;
			}
		}

		if (m_bARCL_ForceContactShadows)
		{
			BaseContainer displaySettings = engineUserSettings.GetModule("DisplayUserSettings");
			int ssdoQuality;
			if (displaySettings && displaySettings.Get("SSDO", ssdoQuality) && ssdoQuality < ARCL_REQUIRED_SSDO)
			{
				displaySettings.Set("SSDO", ARCL_REQUIRED_SSDO);
				changed = true;
			}
		}

		if (m_bARCL_ForceGrassSettings)
		{
			BaseContainer grassSettings = engineUserSettings.GetModule("GrassMaterialSettings");
			int grassLod;
			if (grassSettings && grassSettings.Get("Lod", grassLod) && grassLod < ARCL_MIN_GRASS_LOD)
			{
				grassSettings.Set("Lod", ARCL_MIN_GRASS_LOD);
				changed = true;
			}

			if (GetGame().GetGrassDistance() < ARCL_MIN_GRASS_DISTANCE)
			{
				GetGame().SetGrassDistance(ARCL_MIN_GRASS_DISTANCE);
				changed = true;
			}
		}

		if (changed)
			GetGame().UserSettingsChanged();

		return changed;
	}

	protected void ARCL_LoadSettings()
	{
		if (!FileIO.FileExists(ARCL_SETTINGS_PATH))
		{
			ARCL_WriteSettings();
			Replication.BumpMe();
			return;
		}

		JsonLoadContext loadContext = new JsonLoadContext();
		if (!loadContext.LoadFromFile(ARCL_SETTINGS_PATH))
		{
			Print(string.Format("[ARCL_Core] ERROR: %1 exists but failed to parse -- using defaults", ARCL_SETTINGS_PATH), LogLevel.ERROR);
			return;
		}

		m_bARCL_FirstPersonOnFoot = ARCL_ReadBool(loadContext, "firstPersonOnFoot", m_bARCL_FirstPersonOnFoot);
		m_bARCL_ThirdPersonInVehicles = ARCL_ReadBool(loadContext, "thirdPersonInVehicles", m_bARCL_ThirdPersonInVehicles);
		m_bARCL_RemoveBuildRankRequirements = ARCL_ReadBool(loadContext, "removeBuildRankRequirements", m_bARCL_RemoveBuildRankRequirements);
		m_bARCL_RevertEnemyRadios = ARCL_ReadBool(loadContext, "revertEnemyRadios", m_bARCL_RevertEnemyRadios);
		m_bARCL_IndestructibleBuildables = ARCL_ReadBool(loadContext, "indestructibleBuildables", m_bARCL_IndestructibleBuildables);
		m_bARCL_ForceDistantShadows = ARCL_ReadBool(loadContext, "forceDistantShadows", m_bARCL_ForceDistantShadows);
		m_bARCL_ForceGrassSettings = ARCL_ReadBool(loadContext, "forceGrassSettings", m_bARCL_ForceGrassSettings);
		m_bARCL_ForceContactShadows = ARCL_ReadBool(loadContext, "forceContactShadows", m_bARCL_ForceContactShadows);
		m_bARCL_SpawnSupplyCostEnabled = ARCL_ReadBool(loadContext, "spawnSupplyCostEnabled", m_bARCL_SpawnSupplyCostEnabled);
		m_iARCL_SpawnSupplyCostMinimum = ARCL_ReadInt(loadContext, "spawnSupplyCostMinimum", m_iARCL_SpawnSupplyCostMinimum);
		m_iARCL_SpawnSupplyCostMaximum = ARCL_ReadInt(loadContext, "spawnSupplyCostMaximum", m_iARCL_SpawnSupplyCostMaximum);
		m_bARCL_SpawnSupplyCostChargeAtHQ = ARCL_ReadBool(loadContext, "spawnSupplyCostChargeAtHQ", m_bARCL_SpawnSupplyCostChargeAtHQ);
		m_bARCL_SpawnSupplyCostLog = ARCL_ReadBool(loadContext, "spawnSupplyCostLog", m_bARCL_SpawnSupplyCostLog);
		m_bARCL_SpawnSupplyGate = ARCL_ReadBool(loadContext, "spawnSupplyGate", m_bARCL_SpawnSupplyGate);
		m_bARCL_SpawnSupplyGateOnDeployables = ARCL_ReadBool(loadContext, "spawnSupplyGateOnDeployables", m_bARCL_SpawnSupplyGateOnDeployables);
		m_bARCL_DisableDeployableSpawnPoints = ARCL_ReadBool(loadContext, "disableDeployableSpawnPoints", m_bARCL_DisableDeployableSpawnPoints);

		PrintFormat("[ARCL_Core] Spawn supply cost: enabled=%1 minimum=%2 maximum=%3 chargeAtHQ=%4 log=%5 gate=%6 gateOnDeployables=%7",
			m_bARCL_SpawnSupplyCostEnabled, m_iARCL_SpawnSupplyCostMinimum, m_iARCL_SpawnSupplyCostMaximum,
			m_bARCL_SpawnSupplyCostChargeAtHQ, m_bARCL_SpawnSupplyCostLog, m_bARCL_SpawnSupplyGate,
			m_bARCL_SpawnSupplyGateOnDeployables);

		ARCL_WriteSettings();

		Replication.BumpMe();

		PrintFormat("[ARCL_Core] Settings loaded: firstPersonOnFoot=%1 thirdPersonInVehicles=%2 removeBuildRankRequirements=%3 revertEnemyRadios=%4 indestructibleBuildables=%5 forceDistantShadows=%6 forceGrassSettings=%7 forceContactShadows=%8",
			m_bARCL_FirstPersonOnFoot, m_bARCL_ThirdPersonInVehicles, m_bARCL_RemoveBuildRankRequirements, m_bARCL_RevertEnemyRadios, m_bARCL_IndestructibleBuildables, m_bARCL_ForceDistantShadows, m_bARCL_ForceGrassSettings, m_bARCL_ForceContactShadows);
	}

	protected bool ARCL_ReadBool(notnull JsonLoadContext loadContext, string key, bool current)
	{
		bool value = current;
		if (loadContext.ReadValue(key, value))
			return value;

		return current;
	}

	protected int ARCL_ReadInt(notnull JsonLoadContext loadContext, string key, int current)
	{
		int value = current;
		if (loadContext.ReadValue(key, value))
			return value;

		return current;
	}

	protected string ARCL_JsonBool(bool value)
	{
		if (value)
			return "true";

		return "false";
	}

	protected string ARCL_JsonLine(string key, string value)
	{
		return "\t\"" + key + "\": " + value + ",\n";
	}

	protected void ARCL_WriteSettings()
	{
		FileHandle file = FileIO.OpenFile(ARCL_SETTINGS_PATH, FileMode.WRITE);
		if (!file)
		{
			Print(string.Format("[ARCL_Core] ERROR: unable to write %1 -- using loaded values", ARCL_SETTINGS_PATH), LogLevel.ERROR);
			return;
		}

		string content = "{\n";

		content = content + ARCL_JsonLine("firstPersonOnFoot", ARCL_JsonBool(m_bARCL_FirstPersonOnFoot));
		content = content + ARCL_JsonLine("thirdPersonInVehicles", ARCL_JsonBool(m_bARCL_ThirdPersonInVehicles));
		content = content + ARCL_JsonLine("removeBuildRankRequirements", ARCL_JsonBool(m_bARCL_RemoveBuildRankRequirements));
		content = content + ARCL_JsonLine("revertEnemyRadios", ARCL_JsonBool(m_bARCL_RevertEnemyRadios));
		content = content + ARCL_JsonLine("indestructibleBuildables", ARCL_JsonBool(m_bARCL_IndestructibleBuildables));
		content = content + ARCL_JsonLine("forceDistantShadows", ARCL_JsonBool(m_bARCL_ForceDistantShadows));
		content = content + ARCL_JsonLine("forceGrassSettings", ARCL_JsonBool(m_bARCL_ForceGrassSettings));
		content = content + ARCL_JsonLine("forceContactShadows", ARCL_JsonBool(m_bARCL_ForceContactShadows));
		content = content + ARCL_JsonLine("spawnSupplyCostEnabled", ARCL_JsonBool(m_bARCL_SpawnSupplyCostEnabled));
		content = content + ARCL_JsonLine("spawnSupplyCostMinimum", m_iARCL_SpawnSupplyCostMinimum.ToString());
		content = content + ARCL_JsonLine("spawnSupplyCostMaximum", m_iARCL_SpawnSupplyCostMaximum.ToString());
		content = content + ARCL_JsonLine("spawnSupplyCostChargeAtHQ", ARCL_JsonBool(m_bARCL_SpawnSupplyCostChargeAtHQ));
		content = content + ARCL_JsonLine("spawnSupplyCostLog", ARCL_JsonBool(m_bARCL_SpawnSupplyCostLog));
		content = content + ARCL_JsonLine("spawnSupplyGate", ARCL_JsonBool(m_bARCL_SpawnSupplyGate));
		content = content + ARCL_JsonLine("spawnSupplyGateOnDeployables", ARCL_JsonBool(m_bARCL_SpawnSupplyGateOnDeployables));
		content = content + "\t\"disableDeployableSpawnPoints\": " + ARCL_JsonBool(m_bARCL_DisableDeployableSpawnPoints) + "\n}\n";

		file.Write(content, content.Length());
		file.Close();

		PrintFormat("[ARCL_Core] Wrote settings file: %1", ARCL_SETTINGS_PATH);
	}
}
