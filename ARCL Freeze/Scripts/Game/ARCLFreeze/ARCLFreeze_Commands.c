//------------------------------------------------------------------------------------------------
// ARCL Freeze - Server Commands
//
//   #freeze          always freezes
//   #unfreeze        always unfreezes
//
// Both are idempotent on purpose. A toggle would be a hazard with multiple staff online: two
// admins both reacting to the same incident would freeze, then immediately unfreeze, the match.
// The Game Master toolbar button is still a toggle, because that one shows its state.
//
// Requires ADMINISTRATOR for chat, admin permission for RCON. The RCON path means the league's
// Discord bot or admin panel can trigger a freeze with nobody in-game.
//
// NOTE: ScrServerCommand requires OnUpdate() to exist or registration fails outright with
// "Missing api definition: OnUpdate". It is the poll used by asynchronous commands (see vanilla
// UploadSaveCommand / BanCommands, which wait on backend callbacks). Ours complete synchronously,
// so the result object is already terminal by the time anything could poll it - but the override
// still has to be present.
//------------------------------------------------------------------------------------------------

class ARCLFreeze_FreezeCommand : ScrServerCommand
{
	//! Single reusable result object, mutated in place. Same pattern as vanilla UploadSaveCommand.
	protected ref ScrServerCmdResult m_Result_ARCLFreeze = new ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);

	//------------------------------------------------------------------------------------------------
	override string GetKeyword()
	{
		return "freeze";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsServerSide()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override int RequiredChatPermission()
	{
		return EPlayerRole.ADMINISTRATOR;
	}

	//------------------------------------------------------------------------------------------------
	override int RequiredRCONPermission()
	{
		return ERCONPermissions.PERMISSIONS_ADMIN;
	}

	//------------------------------------------------------------------------------------------------
	//! First chat stage, always client-side. Returning OK passes the command through to the server
	//! because IsServerSide() is true. Same trivial implementation as vanilla BanCommands.
	override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
	{
		return ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);
	}

	//------------------------------------------------------------------------------------------------
	override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
	{
		return ARCLFreeze_Run(argv);
	}

	//------------------------------------------------------------------------------------------------
	override ref ScrServerCmdResult OnRCONExecution(array<string> argv)
	{
		return ARCLFreeze_Run(argv);
	}

	//------------------------------------------------------------------------------------------------
	//! Required by the command API. The work is synchronous, so this only ever hands back the
	//! already-terminal result.
	override ref ScrServerCmdResult OnUpdate()
	{
		return m_Result_ARCLFreeze;
	}

	//------------------------------------------------------------------------------------------------
	//! Always freezes. Takes no arguments. Safe to run twice.
	protected ref ScrServerCmdResult ARCLFreeze_Run(array<string> argv)
	{
		ARCLFreeze_ManagerComponent mgr = ARCLFreeze_ManagerComponent.GetInstance();
		if (!mgr)
		{
			m_Result_ARCLFreeze.m_sResponse = "ARCL Freeze: manager component not found. Is ARCLFreeze_ManagerComponent attached to the game mode?";
			m_Result_ARCLFreeze.m_eResultType = EServerCmdResultType.ERR;
			return m_Result_ARCLFreeze;
		}

		if (mgr.IsFrozenState())
		{
			m_Result_ARCLFreeze.m_sResponse = "ARCL Freeze: world is already frozen.";
			m_Result_ARCLFreeze.m_eResultType = EServerCmdResultType.OK;
			return m_Result_ARCLFreeze;
		}

		mgr.SetFrozen(true);

		m_Result_ARCLFreeze.m_sResponse = "ARCL Freeze: world FROZEN.";
		m_Result_ARCLFreeze.m_eResultType = EServerCmdResultType.OK;
		return m_Result_ARCLFreeze;
	}
}

//------------------------------------------------------------------------------------------------
//! Convenience alias so staff never have to think about arguments under pressure.
//------------------------------------------------------------------------------------------------
class ARCLFreeze_UnfreezeCommand : ScrServerCommand
{
	protected ref ScrServerCmdResult m_Result_ARCLFreeze = new ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);

	//------------------------------------------------------------------------------------------------
	override string GetKeyword()
	{
		return "unfreeze";
	}

	//------------------------------------------------------------------------------------------------
	override bool IsServerSide()
	{
		return true;
	}

	//------------------------------------------------------------------------------------------------
	override int RequiredChatPermission()
	{
		return EPlayerRole.ADMINISTRATOR;
	}

	//------------------------------------------------------------------------------------------------
	override int RequiredRCONPermission()
	{
		return ERCONPermissions.PERMISSIONS_ADMIN;
	}

	//------------------------------------------------------------------------------------------------
	//! See the note on the same method in ARCLFreeze_FreezeCommand.
	override ref ScrServerCmdResult OnChatClientExecution(array<string> argv, int playerId)
	{
		return ScrServerCmdResult(string.Empty, EServerCmdResultType.OK);
	}

	//------------------------------------------------------------------------------------------------
	override ref ScrServerCmdResult OnChatServerExecution(array<string> argv, int playerId)
	{
		return ARCLFreeze_Release();
	}

	//------------------------------------------------------------------------------------------------
	override ref ScrServerCmdResult OnRCONExecution(array<string> argv)
	{
		return ARCLFreeze_Release();
	}

	//------------------------------------------------------------------------------------------------
	//! Required by the command API - see the note at the top of this file.
	override ref ScrServerCmdResult OnUpdate()
	{
		return m_Result_ARCLFreeze;
	}

	//------------------------------------------------------------------------------------------------
	//! Always unfreezes. Takes no arguments. Safe to run twice.
	protected ref ScrServerCmdResult ARCLFreeze_Release()
	{
		ARCLFreeze_ManagerComponent mgr = ARCLFreeze_ManagerComponent.GetInstance();
		if (!mgr)
		{
			m_Result_ARCLFreeze.m_sResponse = "ARCL Freeze: manager component not found.";
			m_Result_ARCLFreeze.m_eResultType = EServerCmdResultType.ERR;
			return m_Result_ARCLFreeze;
		}

		if (!mgr.IsFrozenState())
		{
			m_Result_ARCLFreeze.m_sResponse = "ARCL Freeze: world is not frozen.";
			m_Result_ARCLFreeze.m_eResultType = EServerCmdResultType.OK;
			return m_Result_ARCLFreeze;
		}

		mgr.SetFrozen(false);

		m_Result_ARCLFreeze.m_sResponse = "ARCL Freeze: world released.";
		m_Result_ARCLFreeze.m_eResultType = EServerCmdResultType.OK;
		return m_Result_ARCLFreeze;
	}
}
