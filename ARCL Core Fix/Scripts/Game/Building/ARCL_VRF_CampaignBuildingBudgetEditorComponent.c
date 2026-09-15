//------------------------------------------------------------------------------------------------
//! Restores the rank requirement authored on vehicle prefabs when they are requested through the
//! campaign building ("build mode") flow.
//!
//! ARCL_Core removes every RANK_* budget from SCR_CampaignBuildingBudgetEditorComponent so that any
//! rank can place compositions. That removal is entity-agnostic - it drops rank costs from whatever
//! passes through GetEntityPreviewBudgetCosts(). Vehicles go through the exact same build-mode
//! budget component, so the RANK_* cost authored on a vehicle's SCR_EditableVehicleUIInfo (for
//! example the UH-60's RANK_LIEUTENANT) is stripped as collateral and every vehicle ends up
//! available at Private.
//!
//! This mod loads after ARCL_Core, so its override wraps ARCL_Core's: super() strips the rank costs
//! as before, then the authored rank cost is put back for VEHICLE entities only. Compositions keep
//! the ARCL_Core behaviour untouched.
//!
//! Both the client preview path (SCR_BudgetEditorComponent.CanPlaceEntityInfo) and the authoritative
//! placement check (SCR_CampaignBuildingBudgetEditorComponent.CanPlaceEntitySource, via
//! GetEntitySourcePreviewBudgetCosts) funnel through this method, so one hook covers both.
//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingBudgetEditorComponent
{
	//! Flip to true to log every restored rank cost to the console while testing.
	protected static const bool ARCL_VRF_LOG_RESTORES = false;

	//------------------------------------------------------------------------------------------------
	override bool GetEntityPreviewBudgetCosts(SCR_EditableEntityUIInfo entityUIInfo, out notnull array<ref SCR_EntityBudgetValue> budgetCosts)
	{
		bool result = super.GetEntityPreviewBudgetCosts(entityUIInfo, budgetCosts);

		if (!result || !entityUIInfo)
			return result;

		if (!ARCL_VRF_IsVehicle(entityUIInfo))
			return result;

		ARCL_VRF_RestoreRankBudgets(entityUIInfo, budgetCosts);

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Vehicles are the only entities whose rank cost must survive the ARCL_Core strip.
	//! Helicopters inherit from Vehicle, so they resolve to EEditableEntityType.VEHICLE as well.
	protected bool ARCL_VRF_IsVehicle(notnull SCR_EditableEntityUIInfo entityUIInfo)
	{
		if (SCR_EditableVehicleUIInfo.Cast(entityUIInfo))
			return true;

		return entityUIInfo.GetEntityType() == EEditableEntityType.VEHICLE;
	}

	//------------------------------------------------------------------------------------------------
	//! Re-add the RANK_* costs authored on the prefab that were removed further down the chain.
	protected void ARCL_VRF_RestoreRankBudgets(notnull SCR_EditableEntityUIInfo entityUIInfo, notnull array<ref SCR_EntityBudgetValue> budgetCosts)
	{
		array<ref SCR_EntityBudgetValue> authoredCosts = {};

		//--- No authored costs means vanilla fell back to the entity type budget, which is never a rank
		if (!entityUIInfo.GetEntityBudgetCost(authoredCosts))
			return;

		array<ref SCR_EntityBudgetValue> childrenCosts = {};
		entityUIInfo.GetEntityChildrenBudgetCost(childrenCosts);

		//--- Sum per rank type the way vanilla MergeBudgetCosts() would, without touching the
		//--- SCR_EntityBudgetValue instances owned by the prefab's UI info
		map<EEditableEntityBudget, int> rankCosts = new map<EEditableEntityBudget, int>();
		ARCL_VRF_CollectRankBudgets(authoredCosts, rankCosts);
		ARCL_VRF_CollectRankBudgets(childrenCosts, rankCosts);

		foreach (EEditableEntityBudget budgetType, int budgetValue : rankCosts)
		{
			if (budgetValue <= 0)
				continue;

			//--- Mirror the vanilla FilterAvailableBudgets() pass: never add a budget this component
			//--- has no maximum configured for
			if (!IsBudgetAvailable(budgetType))
				continue;

			//--- Still present (ARCL_Core rank removal turned off) - leave it exactly as it is
			if (ARCL_VRF_ContainsBudget(budgetCosts, budgetType))
				continue;

			budgetCosts.Insert(new SCR_EntityBudgetValue(budgetType, budgetValue));

			if (ARCL_VRF_LOG_RESTORES)
			{
				PrintFormat("[ARCL_VehicleRankFix] Restored %1 (%2) on '%3'",
					typename.EnumToString(EEditableEntityBudget, budgetType), budgetValue, entityUIInfo.GetName());
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ARCL_VRF_CollectRankBudgets(notnull array<ref SCR_EntityBudgetValue> sourceCosts, notnull map<EEditableEntityBudget, int> rankCosts)
	{
		foreach (SCR_EntityBudgetValue sourceCost : sourceCosts)
		{
			if (!sourceCost)
				continue;

			EEditableEntityBudget budgetType = sourceCost.GetBudgetType();
			if (!ARCL_VRF_IsRankBudget(budgetType))
				continue;

			int total;
			rankCosts.Find(budgetType, total);
			rankCosts.Set(budgetType, total + sourceCost.GetBudgetValue());
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool ARCL_VRF_ContainsBudget(notnull array<ref SCR_EntityBudgetValue> budgetCosts, EEditableEntityBudget budgetType)
	{
		foreach (SCR_EntityBudgetValue budgetCost : budgetCosts)
		{
			if (budgetCost && budgetCost.GetBudgetType() == budgetType)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool ARCL_VRF_IsRankBudget(EEditableEntityBudget type)
	{
		return type == EEditableEntityBudget.RANK_PRIVATE
			|| type == EEditableEntityBudget.RANK_CORPORAL
			|| type == EEditableEntityBudget.RANK_SERGEANT
			|| type == EEditableEntityBudget.RANK_LIEUTENANT
			|| type == EEditableEntityBudget.RANK_CAPTAIN
			|| type == EEditableEntityBudget.RANK_MAJOR
			|| type == EEditableEntityBudget.RANK_COLONEL
			|| type == EEditableEntityBudget.RANK_GENERAL;
	}
}
