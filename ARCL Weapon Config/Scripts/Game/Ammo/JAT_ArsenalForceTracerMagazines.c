// Removes magazines without the TRACER ammo flag from every arsenal.
modded class SCR_ArsenalComponent
{
	protected static bool JAT_AUDIT_ONLY = false;          // true = log only, remove nothing
	protected static EAmmoType JAT_REQUIRED_AMMO = EAmmoType.TRACER;
	protected static bool JAT_EXEMPT_ORDNANCE = true;
	protected static bool JAT_EXEMPT_9X19 = true;          // keep all 9x19 mags regardless of tracer
	protected static bool JAT_KEEP_UNFLAGGED = true;       // unreadable flags -> keep, never delete
	protected static bool JAT_USE_NAME_FALLBACK = true;
	protected static bool JAT_LOG_SUMMARY = true;          // one line, once per session
	protected static bool JAT_LOG_VERBOSE = false;         // per-magazine detail, debugging only

	protected static bool s_JAT_SummaryLogged = false;

	// Rockets/HE/smoke/illum are magazines too, but have no tracer variants - exempt or AT and
	// indirect-fire weapons lose all ammo. INCENDIARY excluded: API/APIT are bullets.
	protected static EAmmoType JAT_ORDNANCE_FLAGS =
		EAmmoType.HE | EAmmoType.HEAT | EAmmoType.FRAG | EAmmoType.SMOKE |
		EAmmoType.ILLUMINATION | EAmmoType.HEDP;

	// Second ordnance net: most catalog entries have no item type authored, so neither test alone
	// catches everything.
	protected static SCR_EArsenalItemType JAT_ORDNANCE_TYPES =
		SCR_EArsenalItemType.ROCKET_LAUNCHER | SCR_EArsenalItemType.MORTARS |
		SCR_EArsenalItemType.EXPLOSIVES | SCR_EArsenalItemType.LETHAL_THROWABLE |
		SCR_EArsenalItemType.NON_LETHAL_THROWABLE;

	protected static ref map<ResourceName, bool> s_JAT_KeepCache;

	override bool GetFilteredArsenalItems(out notnull array<SCR_ArsenalItem> filteredArsenalItems, EArsenalItemDisplayType requiresDisplayType = -1)
	{
		bool result = super.GetFilteredArsenalItems(filteredArsenalItems, requiresDisplayType);
		if (!result)
			return result;

		// Fresh array - the entity catalog caches and shares the one super returned.
		array<SCR_ArsenalItem> kept = {};
		int removed = 0;

		foreach (SCR_ArsenalItem item : filteredArsenalItems)
		{
			if (JAT_ShouldKeep(item))
			{
				kept.Insert(item);
			}
			else
			{
				removed++;
				if (JAT_AUDIT_ONLY)
					kept.Insert(item);
			}
		}

		// Once per session, not per arsenal refresh - this runs on a live server.
		if (JAT_LOG_SUMMARY && !s_JAT_SummaryLogged && removed > 0)
		{
			s_JAT_SummaryLogged = true;

			string mode = "";
			string verb = "removed";
			if (JAT_AUDIT_ONLY)
			{
				mode = " (AUDIT - nothing changed)";
				verb = "flagged";
			}

			PrintFormat("[JAT_ForceTracer]%1 %2 non-tracer magazine(s) %3, %4 item(s) offered.",
				mode, removed, verb, kept.Count());
		}

		// Weapons to the top of the arsenal (see JAT_ArsenalWeaponsFirst).
		JAT_ArsenalWeaponsFirst.Apply(kept);

		filteredArsenalItems = kept;
		return !filteredArsenalItems.IsEmpty();
	}

	protected bool JAT_ShouldKeep(SCR_ArsenalItem item)
	{
		if (!item)
			return true;

		if (JAT_EXEMPT_ORDNANCE && (item.GetItemType() & JAT_ORDNANCE_TYPES) != 0)
			return true;

		ResourceName prefab = item.GetItemResourceName();

		// Lazy init: a null static here would throw and leave the arsenal empty.
		if (!s_JAT_KeepCache)
			s_JAT_KeepCache = new map<ResourceName, bool>();

		bool cached;
		if (s_JAT_KeepCache.Find(prefab, cached))
			return cached;

		bool keep = JAT_EvaluatePrefab(prefab);
		s_JAT_KeepCache.Set(prefab, keep);

		if (JAT_LOG_VERBOSE && !keep)
			PrintFormat("[JAT_ForceTracer]   REMOVE (no tracer): %1", prefab);

		return keep;
	}

	protected bool JAT_EvaluatePrefab(ResourceName prefab)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return true;

		IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(resource);
		if (!entitySource)
			return true;

		// No magazine component = not a magazine (weapon, clothing, thrown grenade, meds).
		IEntityComponentSource magSource = JAT_FindMagazineComponentSource(entitySource);
		if (!magSource)
			return true;

		// MagazineUIInfo lives on the magazine component, NOT on InventoryItemComponent - its
		// "Attributes" collection returns a plain item UIInfo and the cast silently fails.
		BaseContainer infoSource = magSource.GetObject("UIInfo");
		if (!infoSource)
			return JAT_KeepUnknown(prefab, "no MagazineUIInfo");

		int flags = 0;
		string label = string.Empty;
		string calibre = string.Empty;

		MagazineUIInfo magInfo = MagazineUIInfo.Cast(BaseContainerTools.CreateInstanceFromContainer(infoSource));
		if (magInfo)
		{
			flags = magInfo.GetAmmoTypeFlags();
			label = magInfo.GetAmmoType();
			calibre = magInfo.GetAmmoCaliber();
		}

		if (JAT_EXEMPT_9X19 && JAT_Is9x19(prefab, calibre, label))
			return true;

		if (flags == 0)
		{
			int rawFlags;
			if (infoSource.Get("m_eAmmoTypeFlags", rawFlags))
				flags = rawFlags;
		}

		if (JAT_EXEMPT_ORDNANCE && (flags & JAT_ORDNANCE_FLAGS) != 0)
			return true;

		if ((flags & JAT_REQUIRED_AMMO) != 0)
			return true;

		// m_eAmmoTypeFlags is only a UI hint: mixed belts often flag just the primary round.
		if (JAT_USE_NAME_FALLBACK && JAT_NameSuggestsTracer(prefab, label))
		{
			if (JAT_LOG_VERBOSE)
				PrintFormat("[JAT_ForceTracer]   KEEP via name fallback (flags=%1): %2", flags, prefab);
			return true;
		}

		if (flags == 0)
			return JAT_KeepUnknown(prefab, "ammo flags are 0/unauthored");

		return false;
	}

	protected bool JAT_KeepUnknown(ResourceName prefab, string reason)
	{
		if (JAT_LOG_VERBOSE)
		{
			string decision = "REMOVE - unknown";
			if (JAT_KEEP_UNFLAGGED)
				decision = "KEEP - unknown";

			PrintFormat("[JAT_ForceTracer]   %1 (%2): %3", decision, reason, prefab);
		}
		return JAT_KEEP_UNFLAGGED;
	}

	// Checks path, calibre and ammo label, so whichever one carries the calibre will hit. Normalising
	// strips separators, so "9x19", "9x19mm" and "9 x 19" all match. M882 is the US 9x19 NATO ball
	// designation - some mods print that instead of the calibre.
	protected bool JAT_Is9x19(ResourceName prefab, string calibre, string label)
	{
		string haystack = JAT_Normalise(prefab) + JAT_Normalise(calibre) + JAT_Normalise(label);
		return haystack.Contains("9x19") || haystack.Contains("m882");
	}

	protected string JAT_Normalise(string value)
	{
		string text = value;
		text.ToLower();
		text.Replace(".", "");
		text.Replace(" ", "");
		text.Replace("_", "");
		text.Replace("-", "");
		return text;
	}

	protected bool JAT_NameSuggestsTracer(ResourceName prefab, string label)
	{
		string path = prefab;
		path.ToLower();
		if (path.Contains("tracer"))
			return true;

		if (label != string.Empty)
		{
			string lower = label;
			lower.ToLower();
			if (lower.Contains("tracer") || lower.Contains("apit") || lower.Contains("bzt"))
				return true;
		}

		return false;
	}

	// BaseMagazineComponent so MagazineComponent, InventoryMagazineComponent and modded
	// subclasses all match.
	protected IEntityComponentSource JAT_FindMagazineComponentSource(IEntitySource entitySource)
	{
		if (!entitySource)
			return null;

		int componentsCount = entitySource.GetComponentCount();
		for (int i = 0; i < componentsCount; i++)
		{
			IEntityComponentSource componentSource = entitySource.GetComponent(i);
			if (!componentSource)
				continue;

			typename componentType = componentSource.GetClassName().ToType();
			if (componentType && componentType.IsInherited(BaseMagazineComponent))
				return componentSource;
		}

		return null;
	}
};
