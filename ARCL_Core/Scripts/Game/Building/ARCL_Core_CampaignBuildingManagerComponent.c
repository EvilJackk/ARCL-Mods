modded class SCR_CampaignBuildingManagerComponent
{

	protected static const ResourceName ARCL_MORTAR_M252_USSR = "{51B4D61744C47709}PrefabsEditable/E_ARCL_MortarPlacement_M252_USSR.et";
	protected static const ResourceName ARCL_MG_NSV_US = "{75DB985A5DFC5929}PrefabsEditable/E_ARCL_Emplacement_MG_NSV_US.et";
	protected static const ResourceName ARCL_MG_NSV_SPP_US = "{0B000D944B1A3F09}PrefabsEditable/E_ARCL_Emplacement_MG_NSV_SPP_US.et";
	protected static const ResourceName ARCL_MG_M2HB_USSR = "{9AF2198DABF0B660}PrefabsEditable/E_ARCL_Emplacement_MG_M2HB_USSR.et";
	protected static const ResourceName ARCL_AA_MG_M2HB_USSR = "{147E197FA29B6918}PrefabsEditable/E_ARCL_Emplacement_AA_MG_M2HB_USSR.et";

	override void GetPrefabListFromConfig()
	{
		super.GetPrefabListFromConfig();

		if (!m_aPlaceablePrefabs)
			return;

		for (int i = m_aPlaceablePrefabs.Count() - 1; i >= 0; i--)
		{
			if (ARCL_IsRemovedBuildable(m_aPlaceablePrefabs[i]))
				m_aPlaceablePrefabs.Remove(i);
		}

		ARCL_InsertBuildable(ARCL_MORTAR_M252_USSR);
		ARCL_InsertBuildable(ARCL_MG_NSV_US);
		ARCL_InsertBuildable(ARCL_MG_NSV_SPP_US);
		ARCL_InsertBuildable(ARCL_MG_M2HB_USSR);
		ARCL_InsertBuildable(ARCL_AA_MG_M2HB_USSR);
	}

	protected void ARCL_InsertBuildable(ResourceName prefab)
	{
		if (m_aPlaceablePrefabs.Find(prefab) == -1)
			m_aPlaceablePrefabs.Insert(prefab);
	}

	protected bool ARCL_IsRemovedBuildable(ResourceName prefab)
	{
		string path = prefab.GetPath();

		if (path.Contains("E_Sandbag") && !path.Contains("E_SandbagPosition"))
			return true;

		if (path.Contains("E_MortarPlacement_S_USSR")
			|| path.Contains("E_MortarPlacement_S_FIA")
			|| path.Contains("E_MortarPlacement_S_AFRF"))
			return true;

		if (path.Contains("E_LivingArea"))
			return true;

		if (path.Contains("E_Emplacement_AA_MG_") && path.Contains("NSV"))
			return true;

		return false;
	}
}
