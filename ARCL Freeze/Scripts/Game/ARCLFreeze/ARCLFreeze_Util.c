//------------------------------------------------------------------------------------------------
// ARCL Freeze - Utility
// Shared constants, logging and Game Master / admin exemption checks.
//------------------------------------------------------------------------------------------------

class ARCLFreeze_Util
{
	//! Shown in the boot log - bump it every build so you can confirm the server picked up the new one.
	static const string VERSION = "0.3.0";
	static const string TAG = "[ARCL Freeze]";

	//! A helicopter above this altitude (metres AGL) is treated as airborne and gets autohover
	//! instead of an immobiliser. Below it, it is treated as parked.
	static const float AIRBORNE_THRESHOLD_M = 3.0;

	//------------------------------------------------------------------------------------------------
	static void Log(string msg, LogLevel level = LogLevel.NORMAL)
	{
		Print(TAG + " " + msg, level);
	}

	//------------------------------------------------------------------------------------------------
	//! Local-machine exemption test. Called every frame from the character control hooks, so it is
	//! kept to cheap lookups only.
	//! \return True when the local player should ignore the freeze
	static bool IsLocalExempt(bool exemptGameMasters, bool exemptAdmins)
	{
		if (!GetGame())
			return false;

		// Editor actually open, excluding limited modes such as Photo. Most direct signal there is.
		if (exemptGameMasters && SCR_EditorManagerEntity.IsOpenedInstance(false))
			return true;

		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return false;

		// Vanilla grants GAME_MASTER on editor open (SCR_EditorManagerEntity.UpdateLimited).
		if (exemptGameMasters && pc.HasRole(EPlayerRole.GAME_MASTER))
			return true;

		if (exemptAdmins
			&& (pc.HasRole(EPlayerRole.ADMINISTRATOR) || pc.HasRole(EPlayerRole.SESSION_ADMINISTRATOR)))
			return true;

		return false;
	}
}
