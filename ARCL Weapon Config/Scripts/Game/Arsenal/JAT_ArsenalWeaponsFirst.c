// Moves every weapon to the top of the arsenal. Everything else keeps its existing catalog order.
//
// Deliberately NOT a second "modded class SCR_ArsenalComponent" overriding GetFilteredArsenalItems:
// that method is already overridden by JAT_ArsenalForceTracerMagazines, and no known mod
// overrides the same method from two modded blocks in one addon. This is a plain helper the existing
// override calls instead, so there is only ever one override in the chain.
//
// Ordering works because nothing downstream re-sorts:
//   GetFilteredArsenalItems -> GetAvailablePrefabs / RefreshArsenal -> m_ItemsInArsenal
//   -> SCR_InventoryOpenedStorageArsenalUI: foreach (prefabsToSpawn) Insert(...)
//   -> WCS LoadoutEditor: WCS_GetItemsOfCategory -> WCS_SortSlots (lays out, never reorders)
class JAT_ArsenalWeaponsFirst
{
	protected static bool JAT_LOG_SUMMARY = true;

	// Log only when this call is bigger than any seen so far. A plain one-shot fires on the first
	// arsenal, which can be a 1-item weapon rack and tells you nothing about the real one.
	protected static int s_JAT_LargestLogged = 0;

	// prefab -> is weapon?  Prefab inspection is the slow path, so cache it.
	protected static ref map<ResourceName, bool> s_JAT_WeaponCache;

	//! Stable partition - relative order inside each group is preserved.
	//! Only call this on an array you own; never on the catalog's shared/cached array.
	static void Apply(notnull array<SCR_ArsenalItem> items)
	{
		array<SCR_ArsenalItem> weapons = {};
		array<SCR_ArsenalItem> others = {};

		foreach (SCR_ArsenalItem item : items)
		{
			if (IsWeapon(item))
				weapons.Insert(item);
			else
				others.Insert(item);
		}

		if (JAT_LOG_SUMMARY && items.Count() > s_JAT_LargestLogged)
		{
			s_JAT_LargestLogged = items.Count();

			PrintFormat("[JAT_WeaponsFirst] %1 weapon(s) moved to top, %2 other item(s) follow.",
				weapons.Count(), others.Count());
		}

		if (weapons.IsEmpty() || others.IsEmpty())
			return;

		items.Clear();

		foreach (SCR_ArsenalItem weapon : weapons)
		{
			items.Insert(weapon);
		}

		foreach (SCR_ArsenalItem other : others)
		{
			items.Insert(other);
		}

	}

	//------------------------------------------------------------------------------------------------
	static bool IsWeapon(SCR_ArsenalItem item)
	{
		if (!item)
			return false;

		// Mode is safe to trust: its default is DEFAULT(2), which does not overlap WEAPON(4)/
		// WEAPON_VARIANTS(8).
		if ((item.GetItemMode() & (SCR_EArsenalItemMode.WEAPON | SCR_EArsenalItemMode.WEAPON_VARIANTS)) != 0)
			return true;

		// m_eItemType is NOT usable here. It selects which category tab an item appears under, so
		// ammunition carries weapon types too - MG belts are MACHINE_GUN, and this project's US
		// catalog has 31 AMMUNITION entries typed SNIPER_RIFLE and 5 typed ROCKET_LAUNCHER. It also
		// defaults to RIFLE (Attribute("2"), RIFLE == 2) when unauthored. Using it grouped guns with
		// their own ammo and preserved the catalog's interleaving. The prefab check below is the
		// only reliable test.

		// Most entries author neither mode nor type, so fall back to the prefab itself.
		ResourceName prefab = item.GetItemResourceName();

		if (!s_JAT_WeaponCache)
			s_JAT_WeaponCache = new map<ResourceName, bool>();

		bool cached;
		if (s_JAT_WeaponCache.Find(prefab, cached))
			return cached;

		bool isWeapon = PrefabIsWeapon(prefab);
		s_JAT_WeaponCache.Set(prefab, isWeapon);
		return isWeapon;
	}

	//------------------------------------------------------------------------------------------------
	//! A weapon is anything carrying a WeaponComponent, minus thrown items. Grenades and mines also
	//! use WeaponComponent, but their UIInfo is a GrenadeUIInfo - that is what separates them.
	protected static bool PrefabIsWeapon(ResourceName prefab)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return false;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!entitySource)
			return false;

		IEntityComponentSource weaponSource = FindWeaponComponentSource(entitySource);
		if (!weaponSource)
			return false;

		BaseContainer infoSource = weaponSource.GetObject("UIInfo");
		if (infoSource && infoSource.GetClassName() == "GrenadeUIInfo")
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Null-safe replacement for SCR_ComponentHelper.GetWeaponComponentSource, which calls
	//! GetClassName().ToType().IsInherited(...) with no null check. Weapons from mods that ship an
	//! unregistered component (e.g. Pronto_AnimationComponent, BaconSuppressors_*) hit a null typename
	//! there and are never recognised, which left whole weapon lists stranded down with the ammo.
	protected static IEntityComponentSource FindWeaponComponentSource(IEntitySource entitySource)
	{
		// Walks the prefab inheritance chain. GetComponentCount() only reports components declared on
		// that source, so a pure override variant (e.g. MG_M240.et, which contains nothing but an ID
		// and inherits everything from MG_M240_base.et) looks component-less and was never detected.
		IEntitySource source = entitySource;
		int depth = 0;

		while (source && depth < 16)
		{
			int componentsCount = source.GetComponentCount();
			for (int i = 0; i < componentsCount; i++)
			{
				IEntityComponentSource componentSource = source.GetComponent(i);
				if (!componentSource)
					continue;

				typename componentType = componentSource.GetClassName().ToType();
				if (!componentType)
					continue;   // class did not register - skip it, keep looking

				if (componentType.IsInherited(WeaponComponent))
					return componentSource;
			}

			BaseContainer ancestor = source.GetAncestor();
			if (!ancestor)
				break;

			source = ancestor.ToEntitySource();
			depth++;
		}

		return null;
	}
};
