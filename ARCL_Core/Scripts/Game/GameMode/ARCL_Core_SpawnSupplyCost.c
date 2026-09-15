modded class SCR_BaseGameMode
{

	override bool CanPlayerSpawn_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, out SCR_ESpawnResult result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED)
	{
		if (!super.CanPlayerSpawn_S(requestComponent, handlerComponent, data, result))
			return false;

		if (!ARCL_IsSpawnSupplyCostEnabled() || !ARCL_IsSpawnSupplyGateEnabled())
			return true;

		SCR_SpawnPointSpawnData spawnPointData = SCR_SpawnPointSpawnData.Cast(data);
		if (!spawnPointData)
			return true;

		PlayerController playerController = requestComponent.GetPlayerController();
		if (!playerController)
			return true;

		SCR_PlayerLoadoutComponent loadoutComp = SCR_PlayerLoadoutComponent.Cast(playerController.FindComponent(SCR_PlayerLoadoutComponent));
		if (!loadoutComp)
			return true;

		SCR_CampaignMilitaryBaseComponent base;
		SCR_ResourceComponent resourceComp;

		float spawnSupplyCost = ARCL_GetSpawnSupplyCost(requestComponent.GetPlayerId(), spawnPointData.GetSpawnPoint(), loadoutComp, base, resourceComp);

		if (spawnSupplyCost <= 0)
			return true;

		if (base)
		{
			float baseSupplies = base.GetSupplies();
			if (baseSupplies >= spawnSupplyCost)
				return true;

			if (ARCL_IsSpawnSupplyCostLogged())
				PrintFormat("[ARCL_Core] Spawn denied at %1: %2 supplies available, %3 required", base.GetBaseName(), baseSupplies, spawnSupplyCost);

			result = SCR_ESpawnResult.NOT_ALLOWED_NOT_ENOUGH_SUPPLIES;
			return false;
		}

		if (!resourceComp || !ARCL_IsSpawnSupplyGateOnDeployables())
			return true;

		float availableSupplies;
		if (!SCR_ResourceSystemHelper.GetAvailableResources(resourceComp, availableSupplies))
			return true;

		if (availableSupplies >= spawnSupplyCost)
			return true;

		if (ARCL_IsSpawnSupplyCostLogged())
			PrintFormat("[ARCL_Core] Spawn denied at deployable spawn point: %1 supplies available, %2 required", availableSupplies, spawnSupplyCost);

		result = SCR_ESpawnResult.NOT_ALLOWED_NOT_ENOUGH_SUPPLIES;
		return false;
	}

	override protected void ConsumeSuppliesOnPlayerSpawn_S(int playerID, IEntity spawnPoint, SCR_PlayerLoadoutComponent loadoutComp)
	{
		if (!ARCL_IsSpawnSupplyCostEnabled())
		{
			super.ConsumeSuppliesOnPlayerSpawn_S(playerID, spawnPoint, loadoutComp);
			return;
		}

		SCR_CampaignMilitaryBaseComponent base;
		SCR_ResourceComponent resourceComp;

		float spawnSupplyCost = ARCL_GetSpawnSupplyCost(playerID, spawnPoint, loadoutComp, base, resourceComp);

		if (spawnSupplyCost <= 0)
			return;

		if (base)
		{
			float baseSupplies = base.GetSupplies();
			if (spawnSupplyCost > baseSupplies)
				spawnSupplyCost = baseSupplies;

			if (spawnSupplyCost <= 0)
				return;

			base.AddSupplies(spawnSupplyCost * -1);
			return;
		}

		if (resourceComp)
			SCR_ResourceSystemHelper.ConsumeResources(resourceComp, spawnSupplyCost, false);
	}

	protected float ARCL_GetSpawnSupplyCost(int playerID, IEntity spawnPoint, SCR_PlayerLoadoutComponent loadoutComp, out SCR_CampaignMilitaryBaseComponent base, out SCR_ResourceComponent resourceComp)
	{
		SCR_CampaignMilitaryBaseComponent spawnPointBase;
		SCR_ResourceComponent spawnPointResourceComp;

		float spawnSupplyCost;
		if (loadoutComp)
			spawnSupplyCost = SCR_ArsenalManagerComponent.GetLoadoutCalculatedSupplyCost(loadoutComp.GetLoadout(), false, playerID, null, spawnPoint, spawnPointBase, spawnPointResourceComp);

		base = spawnPointBase;
		resourceComp = spawnPointResourceComp;

		return ARCL_ClampSpawnCost(spawnSupplyCost, spawnPointBase);
	}

	protected float ARCL_ClampSpawnCost(float cost, SCR_CampaignMilitaryBaseComponent base)
	{

		if (ARCL_IsFreeByDesign(base) && !ARCL_IsSpawnSupplyCostChargedAtHQ())
			return cost;

		float original = cost;

		float minimum = ARCL_GetSpawnSupplyCostMinimum();
		if (cost < minimum)
			cost = minimum;

		float maximum = ARCL_GetSpawnSupplyCostMaximum();
		if (maximum > 0 && cost > maximum)
			cost = maximum;

		if (ARCL_IsSpawnSupplyCostLogged() && cost != original)
			PrintFormat("[ARCL_Core] Spawn cost %1 clamped to %2 (min=%3 max=%4)", original, cost, minimum, maximum);

		return cost;
	}

	protected bool ARCL_IsFreeByDesign(SCR_CampaignMilitaryBaseComponent base)
	{
		return base && !base.CostSuppliesToSpawn();
	}
}
