class ARCL_Core_RadioSecurity
{
	static bool IsEnabled()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return true;

		return gameMode.ARCL_IsRevertEnemyRadios();
	}

	static BaseRadioComponent GetRadio(IEntity item)
	{
		if (!item)
			return null;

		SCR_RadioComponent gadget = SCR_RadioComponent.Cast(item.FindComponent(SCR_RadioComponent));
		if (!gadget)
			return null;

		return gadget.GetRadioComponent();
	}

	static ChimeraCharacter GetHolder(IEntity item)
	{
		if (!item)
			return null;

		return ChimeraCharacter.Cast(SCR_EntityHelper.GetMainParent(item, true));
	}

	static bool AdoptHolderKey(notnull BaseRadioComponent radio, IEntity holder)
	{
		if (!holder || radio.IsEditorRadio())
			return false;

		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(holder.FindComponent(FactionAffiliationComponent));
		if (!affiliation)
			return false;

		Faction faction = affiliation.GetAffiliatedFaction();
		if (!faction)
			return false;

		string key = faction.GetFactionRadioEncryptionKey();
		if (key.IsEmpty())
			return false;

		if (radio.GetEncryptionKey() == key)
			return false;

		radio.SetEncryptionKey(key);
		RetuneToFaction(radio, faction);

		return true;
	}

	static int AdoptCarriedRadios(IEntity character)
	{
		if (!character)
			return 0;

		InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(character.FindComponent(InventoryStorageManagerComponent));
		if (!inventory)
			return 0;

		array<IEntity> foundItems = {};
		inventory.FindItemsWithComponents(foundItems, {SCR_RadioComponent}, EStoragePurpose.PURPOSE_ANY);

		int adopted;
		foreach (IEntity item : foundItems)
		{
			BaseRadioComponent radio = GetRadio(item);
			if (radio && AdoptHolderKey(radio, character))
				adopted++;
		}

		return adopted;
	}

	static void RetuneToFaction(notnull BaseRadioComponent radio, notnull Faction faction)
	{
		SCR_Faction scriptedFaction = SCR_Faction.Cast(faction);
		if (!scriptedFaction)
			return;

		int factionFrequency = scriptedFaction.GetFactionRadioFrequency();
		if (factionFrequency <= 0)
			return;

		int count = radio.TransceiversCount();
		for (int i = 0; i < count; i++)
		{
			BaseTransceiver transceiver = radio.GetTransceiver(i);
			if (!transceiver)
				continue;

			radio.SetTransceiverFrequency(transceiver, Math.ClampInt(factionFrequency, transceiver.GetMinFrequency(), transceiver.GetMaxFrequency()));
		}
	}
}

modded class SCR_RadioComponent
{
	override void OnParentSlotChanged(InventoryStorageSlot oldSlot, InventoryStorageSlot newSlot)
	{
		super.OnParentSlotChanged(oldSlot, newSlot);

		if (!Replication.IsServer() || !m_BaseRadioComp)
			return;

		if (!ARCL_Core_RadioSecurity.IsEnabled())
			return;

		ARCL_Core_RadioSecurity.AdoptHolderKey(m_BaseRadioComp, ARCL_Core_RadioSecurity.GetHolder(GetOwner()));
	}
}

modded class SCR_GadgetManagerComponent
{
	override void OnItemAdded(IEntity item, BaseInventoryStorageComponent storageOwner)
	{
		super.OnItemAdded(item, storageOwner);

		if (!Replication.IsServer() || !item)
			return;

		if (!item.FindComponent(BaseInventoryStorageComponent))
			return;

		if (!ARCL_Core_RadioSecurity.IsEnabled())
			return;

		ARCL_Core_RadioSecurity.AdoptCarriedRadios(GetOwner());
	}
}
