// Sorts weapons to the front at the entity-catalog level - the single source every arsenal consumer
// reads from.
//
// Sorting inside SCR_ArsenalComponent.GetFilteredArsenalItems was provably running (221 weapons
// partitioned) but the arsenal still rendered in catalog order, so at least one consumer reaches the
// catalog directly instead of going through the arsenal component. This covers all of them:
// SCR_ArsenalComponent, the storage fill, and WCS LoadoutEditor's WCS_GetItemsOfCategory.
//
// Safe to mutate the returned array: vanilla builds a fresh one per call (SCR_EntityCatalogManager
// Component.c - "filteredItems" is a new array, populated from refFilteredItems) rather than handing
// back an internal cache.
modded class SCR_EntityCatalogManagerComponent
{
	override array<SCR_ArsenalItem> GetFilteredArsenalItems(SCR_EArsenalItemType typeFilter, SCR_EArsenalItemMode modeFilter, SCR_EArsenalGameModeType arsenalGameModeType, SCR_Faction faction = null, EArsenalItemDisplayType requiresDisplayType = -1)
	{
		array<SCR_ArsenalItem> items = super.GetFilteredArsenalItems(typeFilter, modeFilter, arsenalGameModeType, faction, requiresDisplayType);

		if (items)
			JAT_ArsenalWeaponsFirst.Apply(items);

		return items;
	}
};
