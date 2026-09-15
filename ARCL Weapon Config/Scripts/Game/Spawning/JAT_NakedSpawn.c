// Players spawn with nothing unless they spawn on a saved WCS loadout.
//
// Hierarchy: SCR_BasePlayerLoadout -> SCR_PlayerLoadout -> SCR_FactionPlayerLoadout
//            -> SCR_PlayerArsenalLoadout (the WCS/RHS saved-loadout type).
//
// SCR_FactionPlayerLoadout covers every default loadout of both factions. SCR_PlayerArsenalLoadout
// is stripped BEFORE super so the saved loadout is applied onto a naked character - that also means
// a save that fails to deserialise leaves the player naked instead of falling back to the loadout
// prefab's default gear.
class JAT_NakedSpawn
{
	static bool JAT_LOG = true;

	//! Deletes everything the character carries or wears. Authority only. Returns items removed.
	static int Strip(IEntity character)
	{
		if (!character || !Replication.IsServer())
			return 0;

		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(
			character.FindComponent(SCR_InventoryStorageManagerComponent));

		if (!inventory)
			return 0;

		int deleted = 0;

		// Deleting a container takes its contents with it, so re-query between passes rather than
		// iterating one stale snapshot.
		for (int pass = 0; pass < 5; pass++)
		{
			array<IEntity> items = {};
			inventory.GetItems(items);

			// Equipped clothing sits in loadout slots; pick it up via the root-item list too.
			array<IEntity> rootItems = {};
			inventory.GetAllRootItems(rootItems);
			foreach (IEntity root : rootItems)
			{
				items.Insert(root);
			}

			if (items.IsEmpty())
				break;

			bool removedAny = false;
			foreach (IEntity item : items)
			{
				if (!item)
					continue;

				if (inventory.TryDeleteItem(item))
				{
					deleted++;
					removedAny = true;
				}
			}

			if (!removedAny)
				break;
		}

		return deleted;
	}
};

//------------------------------------------------------------------------------------------------
// Default loadouts - both factions. Decorator repeated from vanilla: this class is config
// serialised, and dropping it would break every loadout entry with "Unknown class".
[BaseContainerProps(configRoot: true), BaseContainerCustomTitleField("m_sLoadoutName")]
modded class SCR_FactionPlayerLoadout
{
	override void OnLoadoutSpawned(GenericEntity pOwner, int playerId)
	{
		super.OnLoadoutSpawned(pOwner, playerId);

		// Saved loadouts are handled by the SCR_PlayerArsenalLoadout override below. In practice they
		// never reach here (neither WCS nor RHS calls super), but do not rely on that.
		if (SCR_PlayerArsenalLoadout.Cast(this))
			return;

		int deleted = JAT_NakedSpawn.Strip(pOwner);

		if (JAT_NakedSpawn.JAT_LOG)
			PrintFormat("[JAT_NakedSpawn] player %1 spawned naked (%2 item(s) removed).", playerId, deleted);

		// Second pass next frame, in case anything is attached after the loadout callback.
		GetGame().GetCallqueue().Call(JAT_StripAgain, pOwner);
	}

	protected void JAT_StripAgain(IEntity character)
	{
		JAT_NakedSpawn.Strip(character);
	}
};

//------------------------------------------------------------------------------------------------
// Saved WCS loadouts: strip first, then let the saved loadout populate the empty character.
// NOTE: this addon must load AFTER WCS_LoadoutEditor for this override to sit outermost in the
// chain. If it loads first, saved loadouts still work - only the failed-deserialise edge case
// stops being covered.
[BaseContainerProps(configRoot: true), BaseContainerCustomTitleField("m_sLoadoutName")]
modded class SCR_PlayerArsenalLoadout
{
	override void OnLoadoutSpawned(GenericEntity pOwner, int playerId)
	{
		JAT_NakedSpawn.Strip(pOwner);

		super.OnLoadoutSpawned(pOwner, playerId);
	}
};
