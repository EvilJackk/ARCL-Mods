//------------------------------------------------------------------------------------------------
//! Arsenal To Private
//!
//! Flattens the rank requirement on every arsenal item (weapons, gear, attachments, consumables -
//! anything with SCR_ArsenalItem catalog data) down to Private, so all players can pull the same
//! equipment from the arsenal box regardless of rank.
//!
//! Vanilla already defaults m_eRequiredRank to PRIVATE; anything locked higher was overridden in a
//! faction config. This forces those overrides back to the default without touching any config.
//!
//! Every consumer of arsenal item rank reads it through GetRequiredRank(), so this single override
//! covers all of them:
//!   - SCR_ArsenalInventorySlotUI.SetItemRank()      (client: greys out the slot)
//!   - SCR_InventoryMenuUI                           (client: "requires rank X" tooltip hint)
//!   - SCR_ArsenalManagerComponent.CanSaveLoadout()  (server: blocks saving the loadout)
//!   - SCR_PlayerArsenalLoadout                      (accumulates loadout rank from item ranks)
//!
//! NOTE: rank locking is only active when the game mode enables it
//! (SCR_ArsenalManagerComponent.m_bRankLockedItems - "Conflict functionality only"). Outside
//! Conflict there is nothing to unlock, so this mod will correctly appear to do nothing.
//------------------------------------------------------------------------------------------------
//! The decorator MUST be repeated from the vanilla declaration. SCR_ArsenalItem is config-
//! serialized; a modded class without it loses its container registration and every catalog entry
//! fails to load with "Unknown class 'SCR_ArsenalItem'", emptying every arsenal in the game.
[BaseContainerProps(configRoot: true), BaseContainerCustomCheckIntTitleField("m_bEnabled", "Arsenal Data", "DISABLED - Arsenal Data", 1)]
modded class SCR_ArsenalItem
{
	//------------------------------------------------------------------------------------------------
	//! \return Always Private, so any player at Private or above can take the item.
	override SCR_ECharacterRank GetRequiredRank()
	{
		return SCR_ECharacterRank.PRIVATE;
	}
}
