modded class SCR_CampaignBuildingLayoutComponent
{
	override void SpawnComposition()
	{
		super.SpawnComposition();

		if (!Replication.IsServer())
			return;

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode || !gameMode.ARCL_IsIndestructibleBuildables())
			return;

		IEntity root = GetOwner().GetRootParent();
		if (!root)
			return;

		gameMode.ARCL_MakeCompositionIndestructible(root);
		GetGame().GetCallqueue().CallLater(gameMode.ARCL_MakeCompositionIndestructible, 1000, false, root);
	}
}
