modded class SCR_CampaignBuildingBudgetEditorComponent
{

	override bool GetEntityPreviewBudgetCosts(SCR_EditableEntityUIInfo entityUIInfo, out notnull array<ref SCR_EntityBudgetValue> budgetCosts)
	{
		bool result = super.GetEntityPreviewBudgetCosts(entityUIInfo, budgetCosts);

		if (!result || !entityUIInfo)
			return result;

		if (!ARCL_RemoveBuildRankRequirements())
			return result;

		if (ARCL_IsVehicle(entityUIInfo))
			return result;

		for (int i = budgetCosts.Count() - 1; i >= 0; i--)
		{
			if (budgetCosts[i] && ARCL_IsRankBudget(budgetCosts[i].GetBudgetType()))
				budgetCosts.Remove(i);
		}

		return result;
	}

	protected bool ARCL_IsVehicle(notnull SCR_EditableEntityUIInfo entityUIInfo)
	{
		if (SCR_EditableVehicleUIInfo.Cast(entityUIInfo))
			return true;

		return entityUIInfo.GetEntityType() == EEditableEntityType.VEHICLE;
	}

	protected bool ARCL_RemoveBuildRankRequirements()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return true;

		return gameMode.ARCL_IsRemoveBuildRankRequirements();
	}

	protected bool ARCL_IsRankBudget(EEditableEntityBudget type)
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
