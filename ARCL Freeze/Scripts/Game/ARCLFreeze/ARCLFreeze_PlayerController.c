//------------------------------------------------------------------------------------------------
// ARCL Freeze - Player Controller
//
// SCR_PlayerController registers a handful of InputManager action listeners that never consult the
// character control disable flags. Inventory is the one that matters for league integrity - without
// this a frozen player could re-kit mid-stoppage.
//------------------------------------------------------------------------------------------------

modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	//! Registered via AddActionListener("Inventory", ...), entirely independent of
	//! SetDisableWeaponControls - so it has to be closed here.
	override void ActionOpenInventory()
	{
		if (ARCLFreeze_ManagerComponent.IsLocalFrozen())
			return;

		super.ActionOpenInventory();
	}

	//------------------------------------------------------------------------------------------------
	//! Same class of bypass. Cosmetic rather than integrity-relevant, but a frozen player pinging
	//! the map looks wrong. Remove this override if you would rather leave pinging available.
	override void ActionGesturePing(float value = 0.0, EActionTrigger reason = 0)
	{
		if (ARCLFreeze_ManagerComponent.IsLocalFrozen())
			return;

		super.ActionGesturePing(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	override void ActionGesturePingHold(float value = 0.0, EActionTrigger reason = 0)
	{
		if (ARCLFreeze_ManagerComponent.IsLocalFrozen())
			return;

		super.ActionGesturePingHold(value, reason);
	}

	//------------------------------------------------------------------------------------------------
	//! Optic magnification and sight switching are their own listeners too, so SetDisableWeaponControls
	//! does not stop them. Weapon manipulation during a stoppage should be off in a league context.
	override protected void ChangeMagnification(float value)
	{
		if (ARCLFreeze_ManagerComponent.IsLocalFrozen())
			return;

		super.ChangeMagnification(value);
	}

	//------------------------------------------------------------------------------------------------
	override protected void ChangeWeaponOptics()
	{
		if (ARCLFreeze_ManagerComponent.IsLocalFrozen())
			return;

		super.ChangeWeaponOptics();
	}
}

// Deliberately NOT blocked:
//   OnWalk / OnEndWalk        - harmless, movement is already dead
//   ActionFocusToggle*        - cosmetic zoom, blocking it just feels broken
//   RequestRespawn            - a player who genuinely died during a freeze must still be able to
//                               respawn. Forcing a respawn to escape a freeze is an edge case not
//                               worth the risk of soft-locking someone.
