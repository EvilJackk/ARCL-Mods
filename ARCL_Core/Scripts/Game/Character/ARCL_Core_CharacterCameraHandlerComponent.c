modded class SCR_CharacterCameraHandlerComponent
{
	protected SCR_CompartmentAccessComponent m_ARCL_CompartmentAccess;
	protected bool m_bARCL_Hooked;

	protected bool ARCL_FirstPersonOnFoot()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return true;

		return gameMode.ARCL_IsFirstPersonOnFoot();
	}

	protected bool ARCL_ThirdPersonInVehicles()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return true;

		return gameMode.ARCL_IsThirdPersonInVehicles();
	}

	protected bool ARCL_IsMounted()
	{
		return m_OwnerCharacter && m_OwnerCharacter.IsInVehicle();
	}

	protected void ARCL_Hook()
	{
		if (m_bARCL_Hooked || !m_OwnerCharacter)
			return;

		m_ARCL_CompartmentAccess = SCR_CompartmentAccessComponent.Cast(m_OwnerCharacter.GetCompartmentAccessComponent());
		if (!m_ARCL_CompartmentAccess)
			return;

		m_ARCL_CompartmentAccess.GetOnCompartmentEntered().Insert(ARCL_OnCompartmentEntered);
		m_ARCL_CompartmentAccess.GetOnCompartmentLeft().Insert(ARCL_OnCompartmentLeft);
		m_bARCL_Hooked = true;
	}

	protected void ARCL_OnCompartmentEntered(IEntity targetEntity, BaseCompartmentManagerComponent manager, int mgrID, int slotID, bool move)
	{
		SetThirdPerson(ARCL_ThirdPersonInVehicles());
	}

	protected void ARCL_OnCompartmentLeft(IEntity targetEntity, BaseCompartmentManagerComponent manager, int mgrID, int slotID, bool move)
	{
		if (move)
			return;

		if (ARCL_FirstPersonOnFoot())
			SetThirdPerson(false);
	}

	override void OnCameraActivate()
	{
		super.OnCameraActivate();

		ARCL_Hook();

		if (!IsInThirdPerson())
			return;

		if (ARCL_IsMounted())
		{
			if (!ARCL_ThirdPersonInVehicles())
				SetThirdPerson(false);
		}
		else if (ARCL_FirstPersonOnFoot())
		{
			SetThirdPerson(false);
		}
	}

	override void OnCameraSwitchPressed()
	{
		if (ARCL_IsMounted())
		{
			if (!ARCL_ThirdPersonInVehicles())
			{
				if (IsInThirdPerson())
					SetThirdPerson(false);
				return;
			}
		}
		else if (ARCL_FirstPersonOnFoot())
		{
			if (IsInThirdPerson())
				SetThirdPerson(false);
			return;
		}

		super.OnCameraSwitchPressed();
	}
}
