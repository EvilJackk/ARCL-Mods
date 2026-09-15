//------------------------------------------------------------------------------------------------
// ARCL Freeze - Vehicle Controller
//
// Covers every vehicle: Car, Car_B, Tracked, Helicopter and Airplane controllers all derive from
// VehicleControllerComponent, so modding it once is enough.
//
// Snapshot state lives on the component itself rather than in a central map. That way a vehicle
// deleted mid-freeze takes its snapshot with it - no dangling handles, no orphaned entries.
//------------------------------------------------------------------------------------------------

modded class VehicleControllerComponent
{
	//! Live registry of every vehicle controller on this machine. Self-maintaining.
	protected static ref array<VehicleControllerComponent> s_aARCLFreezeAll = {};

	protected bool m_bARCLFreezeActive;

	//! True only when this vehicle was actually immobilised via SetCanMove(false).
	//! Airborne helicopters are deliberately NOT immobilised - see ValidateCanMove below.
	protected bool m_bARCLFreezeImmobilised;

	// Pre-freeze snapshot
	protected bool m_bARCLFreezePrevPilotLocked;
	protected bool m_bARCLFreezePrevCanMove;
	protected bool m_bARCLFreezePrevHandBrake;
	protected bool m_bARCLFreezeHasHandBrake;
	protected bool m_bARCLFreezePrevWheelBrake;
	protected bool m_bARCLFreezePrevAutohover;

	//------------------------------------------------------------------------------------------------
	static array<VehicleControllerComponent> ARCLFreeze_GetAll()
	{
		return s_aARCLFreezeAll;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		s_aARCLFreezeAll.Insert(this);

		// Spawned during a freeze - a Game Master placing a vehicle mid-stoppage. Freeze on arrival
		// so it does not roll away the moment it appears.
		if (ARCLFreeze_ManagerComponent.IsFrozen())
			ARCLFreeze_SetFrozen(true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		s_aARCLFreezeAll.RemoveItem(this);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Engine gate, consulted whenever something calls SetCanMove. Holding it false while frozen
	//! stops anything else (a repair completing, a damage update) silently re-enabling movement.
	//!
	//! Gated on m_bARCLFreezeImmobilised, NOT on m_bARCLFreezeActive. For an airborne helicopter
	//! CanMove == false means the powerplant is dead, so forcing it false here would drop a hovering
	//! aircraft out of the sky the moment anything re-evaluated its damage state mid-freeze.
	override bool ValidateCanMove()
	{
		if (m_bARCLFreezeImmobilised)
			return false;

		return super.ValidateCanMove();
	}

	//------------------------------------------------------------------------------------------------
	//! \param frozen Target state
	void ARCLFreeze_SetFrozen(bool frozen)
	{
		if (m_bARCLFreezeActive == frozen)
			return;

		HelicopterControllerComponent heli = HelicopterControllerComponent.Cast(this);

		if (frozen)
			ARCLFreeze_Freeze(heli);
		else
			ARCLFreeze_Unfreeze(heli);
	}

	//------------------------------------------------------------------------------------------------
	// Handbrake lives on two unrelated classes: CarControllerComponent and CarControllerComponent_B
	// both derive straight from VehicleControllerComponent, so _B is NOT a CarControllerComponent and
	// a single cast would silently skip every _B vehicle. Tracked and airplane controllers have no
	// handbrake at all.
	//------------------------------------------------------------------------------------------------
	protected bool ARCLFreeze_GetHandBrake(out bool supported)
	{
		CarControllerComponent car = CarControllerComponent.Cast(this);
		if (car)
		{
			supported = true;
			return car.GetPersistentHandBrake();
		}

		CarControllerComponent_B carB = CarControllerComponent_B.Cast(this);
		if (carB)
		{
			supported = true;
			return carB.GetPersistentHandBrake();
		}

		supported = false;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void ARCLFreeze_SetHandBrake(bool value)
	{
		CarControllerComponent car = CarControllerComponent.Cast(this);
		if (car)
		{
			car.SetPersistentHandBrake(value);
			return;
		}

		CarControllerComponent_B carB = CarControllerComponent_B.Cast(this);
		if (carB)
			carB.SetPersistentHandBrake(value);
	}

	//------------------------------------------------------------------------------------------------
	protected void ARCLFreeze_Freeze(HelicopterControllerComponent heli)
	{
		// Snapshot first, always.
		m_bARCLFreezePrevPilotLocked = ArePilotControlsLocked();
		m_bARCLFreezePrevCanMove = CanMove();
		m_bARCLFreezePrevHandBrake = ARCLFreeze_GetHandBrake(m_bARCLFreezeHasHandBrake);

		if (heli)
		{
			m_bARCLFreezePrevAutohover = heli.GetAutohoverEnabled();
			m_bARCLFreezePrevWheelBrake = heli.GetPersistentWheelBrake();
		}

		m_bARCLFreezeActive = true;

		LockPilotControls(true);

		if (heli)
		{
			if (ARCLFreeze_IsHeliAirborne(heli))
			{
				// Autohover holds station and altitude. Same system as the player-facing autohover
				// toggle (SCR_HelicopterHoverAction).
				//
				// NEVER immobilise an airborne helicopter: the damage system uses CanMove == false
				// to represent a destroyed engine / gearbox / rotors, so it would fall out of the sky.
				heli.SetAutohoverEnabled(true);
			}
			else
			{
				heli.SetPersistentWheelBrake(true);
				ARCLFreeze_Immobilise();
			}

			return;
		}

		ARCLFreeze_Immobilise();

		if (m_bARCLFreezeHasHandBrake)
			ARCLFreeze_SetHandBrake(true);
	}

	//------------------------------------------------------------------------------------------------
	//! Flag must be set before SetCanMove so ValidateCanMove already sees it.
	protected void ARCLFreeze_Immobilise()
	{
		m_bARCLFreezeImmobilised = true;
		SetCanMove(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void ARCLFreeze_Unfreeze(HelicopterControllerComponent heli)
	{
		// Clear both before SetCanMove so ValidateCanMove lets it through.
		m_bARCLFreezeActive = false;
		bool wasImmobilised = m_bARCLFreezeImmobilised;
		m_bARCLFreezeImmobilised = false;

		// Restore only what was recorded. A vehicle that was already immobilised by damage, or
		// already parked with the handbrake on, must stay that way.
		LockPilotControls(m_bARCLFreezePrevPilotLocked);

		// Only push CanMove back if we were the ones who took it away. Touching it otherwise would
		// stomp a damage-driven state that changed during the freeze.
		if (wasImmobilised)
			SetCanMove(m_bARCLFreezePrevCanMove);

		if (m_bARCLFreezeHasHandBrake)
			ARCLFreeze_SetHandBrake(m_bARCLFreezePrevHandBrake);

		if (heli)
		{
			heli.SetPersistentWheelBrake(m_bARCLFreezePrevWheelBrake);

			// Give back exactly the autohover setting the pilot had. Do not gift them an assist.
			heli.SetAutohoverEnabled(m_bARCLFreezePrevAutohover);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return True when the helicopter is flying, or when altitude cannot be determined.
	//! Failing towards "airborne" is deliberate - the airborne path is the safe one.
	protected bool ARCLFreeze_IsHeliAirborne(notnull HelicopterControllerComponent heli)
	{
		VehicleHelicopterSimulation sim = heli.GetSimulation();
		if (!sim)
			return true;

		return sim.GetAltitudeAGL() > ARCLFreeze_Util.AIRBORNE_THRESHOLD_M;
	}
}
