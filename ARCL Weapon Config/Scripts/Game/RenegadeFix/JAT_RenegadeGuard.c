//------------------------------------------------------------------------------------------------
//! JAT_RenegadeGuard - policy for the "players never hold a renegade rank" rule.
//!
//! Verified against the Arma Reforger 1.7.0.54 script source:
//!   scripts/Game/Ranks/SCR_RankContainer.c
//!   scripts/Game/GameMode/FactionManager/SCR_FactionManager.c
//!   scripts/Game/Components/SCR_CharacterRankComponent.c
//!
//! Renegade status is data-driven and per-faction: each SCR_Faction owns an SCR_RankContainer, and
//! each SCR_RankInfo inside it carries its own renegade flag and XP threshold. The container must
//! therefore always be asked, and asked for the right faction - the answer is not global and is not
//! inferable from the SCR_ECharacterRank value or from the sign of a player's XP.
//!
//! Everything here is a query. Nothing in this class mutates game state.
//------------------------------------------------------------------------------------------------
class JAT_RenegadeGuard
{
	protected static const string LOG_PREFIX = "[JAT_RenegadeGuard]";

	//! Latched so a broken rank config warns once instead of once per XP award.
	protected static bool s_bConfigWarningShown;

	//------------------------------------------------------------------------------------------------
	//! \return the rank renegades are pulled back to.
	static SCR_ECharacterRank GetTargetRank()
	{
		return SCR_ECharacterRank.PRIVATE;
	}

	//------------------------------------------------------------------------------------------------
	//! Rank table of the faction a player is affiliated with.
	//! \param[in] playerId PlayerManager/PlayerController player id
	//! \return the container, or null when it cannot be resolved.
	static SCR_RankContainer GetPlayerRanks(int playerId)
	{
		if (playerId <= 0)
			return null;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return null;

		return factionManager.GetFactionRanks(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] ranks rank table to consult
	//! \param[in] rank rank to test
	//! \return true only when the table positively reports the rank as renegade. An unresolved table
	//! answers false, so a missing config can never trigger a demotion.
	static bool IsRenegade(SCR_RankContainer ranks, SCR_ECharacterRank rank)
	{
		if (!ranks || rank == SCR_ECharacterRank.INVALID)
			return false;

		return ranks.IsRankRenegade(rank);
	}

	//------------------------------------------------------------------------------------------------
	//! Whether the guard may act against this rank table.
	//!
	//! If the rank we substitute in is itself flagged renegade, every substitution would re-trigger
	//! the guard. In that case it stands down rather than loop.
	//! \param[in] ranks rank table to consult
	//! \return true when it is safe to act.
	static bool IsUsable(SCR_RankContainer ranks)
	{
		if (!ranks)
			return false;

		if (!ranks.IsRankRenegade(GetTargetRank()))
			return true;

		if (!s_bConfigWarningShown)
		{
			s_bConfigWarningShown = true;
			Log("Target rank is itself flagged renegade in this rank config - standing down to avoid a substitution loop.", LogLevel.WARNING);
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Replaces a renegade rank with the target rank in place.
	//! \param[in] ranks rank table to consult
	//! \param[inout] rank rank to sanitise; left untouched unless this returns true
	//! \return true when the rank was replaced.
	static bool Sanitize(SCR_RankContainer ranks, inout SCR_ECharacterRank rank)
	{
		if (!IsRenegade(ranks, rank))
			return false;

		if (!IsUsable(ranks))
			return false;

		rank = GetTargetRank();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Lowest XP total that maps to the target rank.
	//! \param[in] ranks rank table to consult
	//! \param[out] xp required XP; left untouched unless this returns true
	//! \return true when a usable threshold was resolved.
	static bool GetTargetRankXP(SCR_RankContainer ranks, out int xp)
	{
		if (!IsUsable(ranks))
			return false;

		int required = ranks.GetRequiredRankXP(GetTargetRank());

		// SCR_RankContainer reports int.MAX when the rank is absent from the table. Writing that
		// into a player's XP would be far worse than leaving them renegade.
		if (required == int.MAX)
			return false;

		xp = required;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Authority test, matching the idiom SCR_CharacterRankComponent.AttemptSwitchFaction uses.
	//! \return true when this machine owns the game state.
	static bool IsServer()
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		return (gameMode && gameMode.IsMaster()) || (!gameMode && Replication.IsServer());
	}

	//------------------------------------------------------------------------------------------------
	//! \param[in] message text to log
	//! \param[in] level severity
	static void Log(string message, LogLevel level = LogLevel.NORMAL)
	{
		Print(string.Format("%1 %2", LOG_PREFIX, message), level);
	}
}
