/*
*
* Copyright (C) 2015-2016 <Spirit-wow.ru>
* Written by Pallam
*
*/
#include "Guild.h"
#include "ScriptMgr.h"
#include "ArenaTeamMgr.h"
#include "Common.h"
#include "DisableMgr.h"
#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "Language.h"
#include "GroupMgr.h"
#include "ScriptMgr.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Unit.h"
#include "GameObject.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "InstanceScript.h"
#include "CombatAI.h"
#include "PassiveAI.h"
#include "Chat.h"
#include "DBCStructure.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "World.h"
#include "Language.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "GuildMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include <sstream>
#include <string>

struct Changer
{
	uint32 ItemId, XP;

	//Construct
	Changer(uint32 item, uint32 _xp)
	{
		ItemId = item;
		XP = _xp;
	}
};

enum _Gossip
{
	START_MENU = 0,
	CHANGER_MENU,
	TREE_MENU,
};

enum perems {
	TREE = 10000,
	//////////////////
	SHOP = 1*100*100000,
	SHOP_ENTER = 1,
	SHOP_PURCHASE_1,
	SHOP_PURCHASE_10,
	SHOP_PURCHASE_100,
	SHOP_PURCHASE_1000,
	SHOP_PURCHASE_ALL,
};

class npc_guild_level_master : public CreatureScript
{
public:
	npc_guild_level_master() : CreatureScript("npc_guild_level_master") {
		Init();
	}

	void Init()
	{
		QueryResult result = CharacterDatabase.PQuery("SELECT item_id, item_xp FROM guild_items_to_xp");

		if (result)
		{
			do
			{
				Field* fields = result->Fetch();
				ChangerVector.push_back(Changer(fields[0].GetUInt32(), fields[1].GetUInt32()));
			} while (result->NextRow());
		}
		GuildTalents = sWorld->GetGuildTalentsSystem()->GetAllGuildTreeLevels();
	}

	bool OnGossipHello(Player* player, Creature* me)
	{
		if (!player || !me)
			return true;

		if (player->GetGuild()) {
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Уровень гильдии: " + std::to_string(player->GetGuild()->GetLevel()), GOSSIP_SENDER_MAIN, START_MENU);
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Добавить Очки Опыта моей гильдии", GOSSIP_SENDER_MAIN, CHANGER_MENU);
			
			//Офицеры гильдии могут менять таланты.
			if (player->GetRank() <= 1)
				player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, "Показать древо развития моей гильдии", GOSSIP_SENDER_MAIN, TREE_MENU);
		}
		else 
			ChatHandler(player->GetSession()).SendSysMessage("Вы не состоите в гильдии!");

		player->SEND_GOSSIP_MENU(player->GetGossipTextId(me), me->GetGUID());
		return true;
	};

	bool OnGossipSelect(Player* player, Creature* me, uint32 uiSender, uint32 uiAction)
	{
		if (!player || !me)
			return true;

		player->PlayerTalkClass->ClearMenus();
		WorldSession* session = player->GetSession();
		Guild* guild = player->GetGuild();
		switch (uiAction)
		{
			case START_MENU:
				OnGossipHello(player, me);
				break;
			case CHANGER_MENU: // Меню обмена предметов на очки опыта
			{
				uint16 ItemsCounts = 0;
				for (uint8 i = 0; i < ChangerVector.size(); i++) {
					uint32 actionID = ChangerVector[i].ItemId + SHOP;
					if (player->HasItemCount(ChangerVector[i].ItemId))
					{
						player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Обменять " + ReturnIconAndName(ChangerVector[i].ItemId, session) + " на очки опыта гильдии.", i, actionID);
						ItemsCounts++;
					}
				}

				//Если у игрока нет предметов ИЛИ в базе нет значений для обмена
				if (ItemsCounts == 0)
				{
					ChatHandler(session).SendSysMessage("Вам нечего обменять на Опыт Гильдии.");
					OnGossipHello(player, me);
				} else 
					player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Назад...", GOSSIP_SENDER_MAIN, START_MENU);
				
				player->SEND_GOSSIP_MENU(player->GetGossipTextId(me), me->GetGUID());
				break;
			}
			break;
			case TREE_MENU:
			{
				for (uint8 i = 0; i < GuildTalents.size(); i++)
				{
					//Уровень гильдийского дерева
					uint32 actionID = GuildTalents[i].tree_level + TREE;
					if (guild->GetMaxGuildTreeLevel() < GuildTalents[i].tree_level)
						break;
					uint8 talent_id = guild->GetTalentID(GuildTalents[i].tree_level);
					if (talent_id == 0)
					{
						player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Выбрать бонус", i, actionID);
						continue;
					}
					//Если у гильдии выбран спелл за этот уровень - отобразить спелл
					if (const SpellInfo* spell = sSpellMgr->GetSpellInfo(GuildTalents[i].spells[talent_id - 1])) {
						player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, GetSpellIcon(spell->SpellIconID) + ' ' + spell->SpellName[session->GetSessionDbcLocale()], i, actionID);
					}
					else
						player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, "Выбрать бонус", i, actionID);
				}
				player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Назад...", GOSSIP_SENDER_MAIN, START_MENU);
				player->SEND_GOSSIP_MENU(player->GetGossipTextId(me), me->GetGUID());
				break;
			}
			break;
			default:
			{
				//Вообще, достаточно кривая и сомнительная проверка...
				//Однако, очень сильно напрягает отсуствие других переменных для работы с меню.
				//Имеет ли смысл кастомной дописи, или нет?
				if (uiAction >= SHOP)
				{
					uint32 action = uiAction / SHOP;
					uint32 SItemID = uiAction - action * SHOP;
					uint32 ItemCount = player->GetItemCount(SItemID);
					switch (action)
					{
						case SHOP_ENTER:
						{
							if (ItemCount > 0)
							{
								player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_VENDOR, "Обменять все" + ReturnIconAndName(SItemID, session), uiSender, SHOP_PURCHASE_ALL * SHOP + SItemID, "Вы уверены что хотите обменять \n\n" + std::to_string(ItemCount) + " " + ReturnIconAndName(SItemID, session) + " на " + std::to_string(ChangerVector[uiSender].XP * ItemCount) + " очка(ов) опыта гильдии?", 0, false);
								player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_VENDOR, ConvertToTextTitle(SItemID, session, 1), uiSender, SHOP_PURCHASE_1 * SHOP + SItemID, ConvertToTextBody(SItemID, session, uiSender, 1), 0, false);
							}
							else { ChatHandler(session).SendSysMessage("У вас нет данной валюты..."); OnGossipHello(player, me); }

							if (ItemCount > 9)
								player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_VENDOR, ConvertToTextTitle(SItemID, session, 10), uiSender, SHOP_PURCHASE_10 * SHOP + SItemID, ConvertToTextBody(SItemID, session, uiSender, 10), 0, false);
							if (ItemCount > 99)
								player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_VENDOR, ConvertToTextTitle(SItemID, session, 100), uiSender, SHOP_PURCHASE_100 * SHOP + SItemID, ConvertToTextBody(SItemID, session, uiSender, 100), 0, false);
							if (ItemCount > 999)
								player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_VENDOR, ConvertToTextTitle(SItemID, session, 1000), uiSender, SHOP_PURCHASE_1000 * SHOP + SItemID, ConvertToTextBody(SItemID, session, uiSender, 1), 0, false);

							player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Назад...", GOSSIP_SENDER_MAIN, CHANGER_MENU);
							player->SEND_GOSSIP_MENU(player->GetGossipTextId(me), me->GetGUID());
						}
						return true;
						case SHOP_PURCHASE_1:
							if (player->HasItemCount(SItemID), 1)
							{
								player->DestroyItemCount(SItemID, 1, true);
								player->GetGuild()->GiveXp(1 * ChangerVector[uiSender].XP);
								GuildLevelLogs(player, SItemID, 1, ChangerVector[uiSender].XP);
							}
							break;
						case SHOP_PURCHASE_10:
							if (player->HasItemCount(SItemID), 10)
							{
								player->DestroyItemCount(SItemID, 10, true);
								player->GetGuild()->GiveXp(10 * ChangerVector[uiSender].XP);
								GuildLevelLogs(player, SItemID, 10, ChangerVector[uiSender].XP * 10);
							}
							break;
						case SHOP_PURCHASE_100:
							if (player->HasItemCount(SItemID), 100)
							{
								player->DestroyItemCount(SItemID, 100, true);
								player->GetGuild()->GiveXp(100 * ChangerVector[uiSender].XP);
								GuildLevelLogs(player, SItemID, 100, ChangerVector[uiSender].XP * 100);
							}
							break;
						case SHOP_PURCHASE_1000:
							if (player->HasItemCount(SItemID), 1000)
							{
								player->DestroyItemCount(SItemID, 1000, true);
								player->GetGuild()->GiveXp(1000 * ChangerVector[uiSender].XP);
								GuildLevelLogs(player, SItemID, 1000, ChangerVector[uiSender].XP * 1000);
							}
							break;
						case SHOP_PURCHASE_ALL:
							player->DestroyItemCount(SItemID, ItemCount, true);
							player->GetGuild()->GiveXp(ItemCount * ChangerVector[uiSender].XP);
							GuildLevelLogs(player, SItemID, ItemCount, ChangerVector[uiSender].XP * ItemCount);
							break;
						default:
							ChatHandler(session).SendSysMessage("Что-то пошло не так...");
							OnGossipHello(player, me);
							return true;
							break;
					}
					std::string ss;
					if (player->GetGuild()->GetXpForNextLevel() > 0)
						ss = "Опыт гильдии был повышен. Теперь он равен " + std::to_string(player->GetGuild()->GetCurrentXP()) + " ОО из " + std::to_string(player->GetGuild()->GetXpForNextLevel()) + " ОО";
					else
						ss = "Опыт гильдии был повышен. Теперь он равен " + std::to_string(player->GetGuild()->GetCurrentXP()) + " ОО";
					//ss.str();
					me->Whisper(ss, LANG_UNIVERSAL, player);
					OnGossipHello(player, me);
				}
				else if (uiAction >= TREE)
				{
					uint32 action = uiAction / TREE;
					switch (action)
					{
						case 1:
						{	
							uint32 tree_level = uiAction - TREE;
							for (uint8 i = 0; i < 3; i++)
							{
								uint32 spell_id = 0;
								for (uint8 k = 0; k <= GuildTalents.size(); k++)
									if (GuildTalents[k].tree_level == tree_level)
										spell_id = GuildTalents[k].spells[i];
								uint32 actionId = TREE * (i + 2) + tree_level;
								if (const SpellInfo* spell = sSpellMgr->GetSpellInfo(spell_id))
									player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, GetSpellIcon(spell->SpellIconID) + ' ' + spell->SpellName[session->GetSessionDbcLocale()], i, actionId);
							}
							player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Назад...", GOSSIP_SENDER_MAIN, TREE_MENU);
							player->SEND_GOSSIP_MENU(player->GetGossipTextId(me), me->GetGUID());
						}
						break;
						default:
						{	
							uint32 talent_id = action - 1;
							uint32 tree_level = uiAction - TREE * action;
							guild->SetGuildTalent(tree_level, talent_id);
							if (player->IsGameMaster())
								me->Whisper("DEBUG INFO (GM ONLY): tree_level: " + std::to_string(tree_level) + ", talent_id: " + std::to_string(talent_id), Language(0), player);
							OnGossipHello(player, me);
						}
						break;
					}
				}
				break;
			}
		}
		return true;
	};

	std::vector<Changer> ChangerVector;
	std::vector<GuildTalentsSystem::TreeLevel> GuildTalents;

	std::string ConvertToTextTitle(uint32 SItemID, WorldSession* session, uint32 counter)
	{
		std::string str = "Обменять " + std::to_string(counter) + ' ' +ReturnIconAndName(SItemID, session);
		return str;
	}

	std::string ConvertToTextBody(uint32 SItemID, WorldSession* session, uint32 uiSender, uint32 counter)
	{
		if (counter == 1)
			return "Вы уверены что хотите обменять \n" + ReturnIconAndName(SItemID, session) + " на " + std::to_string(ChangerVector[uiSender].XP * counter) + " очка(ов) опыта гильдии?";
		return "Вы уверены что хотите обменять \n" + std::to_string(counter) + ' ' + ReturnIconAndName(SItemID, session) + " на " + std::to_string(ChangerVector[uiSender].XP * counter) + " очка(ов) опыта гильдии?";
	}

	std::string ReturnIconAndName(uint32 entry, WorldSession* session)
	{
		return sTransmogrification->GetItemIcon(entry, 16, 16, 0, 0) + sTransmogrification->GetItemLink(entry, session);
	};

	std::string GetSpellIcon(uint32 entry, uint32 width = 16, uint32 height = 16, int x = 0, int y = 0) const
	{
		TC_LOG_DEBUG("custom.transmog", "Transmogrification::GetItemIcon");

		std::ostringstream ss;
		ss << "|T";
		const SpellIconPath* IconPath = sSpellIconPathStore.LookupEntry(entry);
		if (IconPath)
			ss << IconPath->path;
		else
			ss << "INTERFACE/InventoryItems/WoWUnknownItem01";
		ss << ":" << width << ":" << height << ":" << x << ":" << y << "|t";
		return ss.str();
	}

	void GuildLevelLogs(Player* player, int32 ItemID, uint32 Count, int64 XP)
	{
		CharacterDatabase.PExecute("INSERT INTO guild_level_logs(`PlayerGUID`, `PlayerPlayedTime`, `PlayerName`, `GuildName`, `ItemID`, `ItemCount`, `XP`) VALUES (%u, %u, \"%s\", \"%s\", %i, %u, %i)", 
			player->GetGUIDLow(), player->m_Played_time[PLAYED_TIME_TOTAL], player->GetName(), player->GetGuildName(), ItemID, Count, XP);
	}
};

void AddSC_npc_guild_level_master()
{
	new npc_guild_level_master();
};