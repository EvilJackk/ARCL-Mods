//------------------------------------------------------------------------------------------------
//! JAT_RenegadeGuard - engine hooks. Policy lives in JAT_RenegadeGuard.c.
//!
//! Layer 1 (rank) is the actual fix and intercepts at SetCharacterRank, which is the single funnel
//! every rank change passes through - including SCR_PlayerXPHandlerComponent.UpdatePlayerRank.
//!
//! Hooking OnRankChanged instead does NOT work, and this is worth spelling out. In 1.7 the stock
//! flow is:
//!
//!     RpcDoSetCharacterRank(newRank, prevRank, silent)
//!         m_iRank = newRank;
//!         OnRankChanged(oldRank, newRank, silent);
//!         SpecialRankHandling(newRank, prevRank);   // <-- still sees the ORIGINAL newRank
//!
//! SpecialRankHandling runs after OnRankChanged and re-reads the rank it was called with, so a
//! reset applied from inside OnRankChanged comes too late: the player is still exiled to the hidden
//! RNGD faction via AttemptSwitchFaction. Substituting the rank before it enters this flow at all
//! avoids the exile, needs no re-entrancy guard, and leaves clients consistent because the server
//! broadcasts the already-substituted value.
//!
//! Layer 2 (XP) then lifts the underlying XP clear of the renegade threshold, so the player is not
//! left pinned at the rank floor by a debt they can never climb out of.
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
//! Layer 1 - no renegade rank ever reaches the stock rank-change flow.
//------------------------------------------------------------------------------------------------
modded class SCR_CharacterRankComponent
{
	//------------------------------------------------------------------------------------------------
	override void SetCharacterRank(SCR_ECharacterRank rank, bool silent = false)
	{
		// GetCharacterFaction and m_Owner are both protected on the base class, so this resolves the
		// character's own faction rather than guessing at a global rank table.
		SCR_Faction faction = GetCharacterFaction(m_Owner);
		if (faction)
		{
			SCR_RankContainer ranks = faction.GetRanks();

			if (JAT_RenegadeGuard.Sanitize(ranks, rank))
				JAT_RenegadeGuard.Log("Blocked a renegade rank; applying Private instead.");
		}

		// Stock SetCharacterRank already no-ops when rank matches the current one, so a substitution
		// that lands on the rank the character already holds costs nothing and sends no RPC.
		super.SetCharacterRank(rank, silent);
	}
}

//------------------------------------------------------------------------------------------------
//! Layer 2 - keep stored XP clear of the renegade threshold.
//------------------------------------------------------------------------------------------------
modded class SCR_PlayerXPHandlerComponent
{
	//------------------------------------------------------------------------------------------------
	override void UpdatePlayerRank(bool notify = true)
	{
		// Raise XP first: stock UpdatePlayerRank derives the new rank straight from m_iPlayerXP.
		JAT_RaiseXPClearOfRenegade();
		super.UpdatePlayerRank(notify);
	}

	//------------------------------------------------------------------------------------------------
	override void AddPlayerXP(SCR_EXPRewards rewardID, float multiplier = 1.0, bool volunteer = false, int addDirectly = 0)
	{
		super.AddPlayerXP(rewardID, multiplier, volunteer, addDirectly);

		// super() normally routes through UpdatePlayerRank above, which already handled this. This
		// covers the paths where it does not; on the common path the check is all it costs.
		if (JAT_RaiseXPClearOfRenegade())
			UpdatePlayerRank(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Raises stored XP to the target rank's threshold when the current total maps to a renegade
	//! rank. Only ever raises.
	//! \return true when XP was changed.
	protected bool JAT_RaiseXPClearOfRenegade()
	{
		if (IsProxy())
			return false;

		SCR_PlayerController playerController = SCR_PlayerController.Cast(GetOwner());
		if (!playerController)
			return false;

		SCR_RankContainer ranks = JAT_RenegadeGuard.GetPlayerRanks(playerController.GetPlayerId());
		if (!ranks)
			return false;

		if (!JAT_RenegadeGuard.IsRenegade(ranks, ranks.GetRankByXP(m_iPlayerXP)))
			return false;

		int targetXP;
		if (!JAT_RenegadeGuard.GetTargetRankXP(ranks, targetXP))
			return false;

		// A renegade result with XP already at or above the threshold has some other cause. Writing
		// XP down to match would quietly cost the player earned progress, so leave it alone.
		if (m_iPlayerXP >= targetXP)
			return false;

		JAT_RenegadeGuard.Log(string.Format("Raising player XP from %1 to %2 to clear the renegade threshold.", m_iPlayerXP, targetXP));

		m_iPlayerXP = targetXP;

		if (m_iPlayerXPSinceLastSpawn < 0)
			m_iPlayerXPSinceLastSpawn = 0;

		// m_iPlayerXP is replicated and was written directly, so push it to the owning client.
		Replication.BumpMe();

		return true;
	}
}
