modded class SCR_VideoSettingsSubMenu : SCR_SettingsSubMenuBase
{
	protected static const int ARCL_MIN_GRASS_DISTANCE = 150;
	protected bool m_bARCL_SyncQueued;

	protected SCR_BaseGameMode ARCL_GetGameMode()
	{
		return SCR_BaseGameMode.Cast(GetGame().GetGameMode());
	}

	protected bool ARCL_ForceVideoSettings()
	{
		SCR_BaseGameMode gameMode = ARCL_GetGameMode();
		if (!gameMode)
			return false;

		return gameMode.ARCL_IsForceVideoSettings();
	}

	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);

		SCR_BaseGameMode gameMode = ARCL_GetGameMode();
		if (!gameMode || !gameMode.ARCL_IsForceVideoSettings())
			return;

		gameMode.ARCL_ApplyVideoMinimums();
		ARCL_SyncWidgets();
	}

	override void OnMenuItemChanged(SCR_SettingsBindingBase binding)
	{
		super.OnMenuItemChanged(binding);

		if (m_bLoadingSettings || !binding)
			return;

		string widgetName = binding.GetWidgetName();
		if (widgetName != "DistantShadows" && widgetName != "SSDO" && widgetName != "GrassDistance" && widgetName != "GrassLOD")
			return;

		SCR_BaseGameMode gameMode = ARCL_GetGameMode();
		if (!gameMode || !gameMode.ARCL_IsForceVideoSettings())
			return;

		if (gameMode.ARCL_ApplyVideoMinimums())
			ARCL_QueueWidgetSync();
	}

	override protected void OnQualityPresetChanged(SCR_ComboBoxComponent combobox, int itemIndex)
	{
		super.OnQualityPresetChanged(combobox, itemIndex);

		SCR_BaseGameMode gameMode = ARCL_GetGameMode();
		if (!gameMode || !gameMode.ARCL_IsForceVideoSettings())
			return;

		if (gameMode.ARCL_ApplyVideoMinimums())
			ARCL_QueueWidgetSync();
	}

	protected void ARCL_QueueWidgetSync()
	{
		if (m_bARCL_SyncQueued)
			return;

		m_bARCL_SyncQueued = true;
		GetGame().GetCallqueue().CallLater(ARCL_DeferredWidgetSync, 0, false);
	}

	protected void ARCL_DeferredWidgetSync()
	{
		m_bARCL_SyncQueued = false;
		ARCL_SyncWidgets();
	}

	protected void ARCL_SyncWidgets()
	{
		if (!m_wRoot)
			return;

		SCR_BaseGameMode gameMode = ARCL_GetGameMode();
		if (!gameMode)
			return;

		UserSettings engineUserSettings = GetGame().GetEngineUserSettings();
		if (!engineUserSettings)
			return;

		m_bLoadingSettings = true;

		if (gameMode.ARCL_IsForceDistantShadows())
		{
			BaseContainer videoSettings = engineUserSettings.GetModule("VideoUserSettings");
			int distantShadowsQuality;
			if (videoSettings && videoSettings.Get("DistantShadowsQuality", distantShadowsQuality))
			{
				SCR_SpinBoxComponent distantShadowsWidget = SCR_SpinBoxComponent.GetSpinBoxComponent("DistantShadows", m_wRoot);
				if (distantShadowsWidget)
				{
					int clampedItem = Math.ClampInt(distantShadowsQuality, 0, distantShadowsWidget.GetNumItems() - 1);
					if (distantShadowsWidget.GetCurrentIndex() != clampedItem)
						distantShadowsWidget.SetCurrentItem(clampedItem);
				}
			}
		}

		if (gameMode.ARCL_IsForceContactShadows())
		{
			BaseContainer displaySettings = engineUserSettings.GetModule("DisplayUserSettings");
			int ssdoQuality;
			if (displaySettings && displaySettings.Get("SSDO", ssdoQuality))
			{
				SCR_SpinBoxComponent ssdoWidget = SCR_SpinBoxComponent.GetSpinBoxComponent("SSDO", m_wRoot);
				if (ssdoWidget)
				{
					int clampedItem = Math.ClampInt(ssdoQuality, 0, ssdoWidget.GetNumItems() - 1);
					if (ssdoWidget.GetCurrentIndex() != clampedItem)
						ssdoWidget.SetCurrentItem(clampedItem);
				}
			}
		}

		if (gameMode.ARCL_IsForceGrassSettings())
		{
			BaseContainer grassSettings = engineUserSettings.GetModule("GrassMaterialSettings");
			int grassLod;
			if (grassSettings && grassSettings.Get("Lod", grassLod))
			{
				SCR_SpinBoxComponent grassLodWidget = SCR_SpinBoxComponent.GetSpinBoxComponent("GrassLOD", m_wRoot);
				if (grassLodWidget)
				{
					int clampedItem = Math.ClampInt(grassLod, 0, grassLodWidget.GetNumItems() - 1);
					if (grassLodWidget.GetCurrentIndex() != clampedItem)
						grassLodWidget.SetCurrentItem(clampedItem);
				}
			}

			SCR_SliderComponent grassDistanceWidget = SCR_SliderComponent.GetSliderComponent("GrassDistance", m_wRoot);
			if (grassDistanceWidget)
			{
				grassDistanceWidget.SetMin(ARCL_MIN_GRASS_DISTANCE);
				float currentGrassDistance = GetGame().GetGrassDistance();
				if (grassDistanceWidget.GetValue() != currentGrassDistance)
					grassDistanceWidget.SetValue(currentGrassDistance);
			}
		}

		m_bLoadingSettings = false;
	}
}
