//------------------------------------------------------------------------------------------------
// ARCL Freeze - Game Mode
//
// Drives the per-frame re-assert, mirroring how vanilla SCR_BaseGameMode calls SetLocalControls()
// every frame from EOnFrame. Reasserting rather than firing once on the transition is what makes
// the lockout survive menu open/close, vehicle entry/exit, respawn and JIP.
//------------------------------------------------------------------------------------------------

modded class SCR_BaseGameMode
{
	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		ARCLFreeze_ManagerComponent mgr = ARCLFreeze_ManagerComponent.GetInstance();
		if (mgr)
			mgr.Tick_ARCLFreeze();
	}
}
