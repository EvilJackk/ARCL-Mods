modded class SCR_InventorySlotWeaponSlotsUI
{
	protected static ResourceName s_sARCL_LauncherSlotIcon = "{CDE4B2BF08F5EA5F}UI/Textures/InventoryIcons/InvenorySlot-Primary_UI.edds";

	override void Init()
	{
		super.Init();

		if (!m_pItem && m_sWeaponSlotType == "launcher")
			SetIcon(s_sARCL_LauncherSlotIcon);
	}

	override string SetSlotSize()
	{
		string slotLayout = super.SetSlotSize();

		if (m_sWeaponSlotType == "launcher")
		{
			m_iSizeX = 2;
			m_iSizeY = 1;
		}

		return slotLayout;
	}
}
