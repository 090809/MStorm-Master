
/*
+ *
+ * Copyright (C) 2013 Emu-Devstore <http://emu-devstore.com/>
+ * Written by Teiby <http://www.teiby.de/>
+ *
+ */

//#ifndef ARENA_1V1_H
#define ARENA_1V1_H

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
#include "Guild.h"
#include "ArenaTeam.h"

// TalentTab.dbc -> TalentTabID
const uint32 FORBIDDEN_TALENTS_IN_1V1_ARENA[] =
{
	// Healer
	//201, // PriestDiscipline
	//202, // PriestHoly
	//382, // PaladinHoly
	//262, // ShamanRestoration
	282, // DruidRestoration

	// Tanks
	//383, // PaladinProtection
	//163, // WarriorProtection

	0 // End
};


// Return false, if player have invested more than 35 talentpoints in a forbidden talenttree.
static bool Arena1v1CheckTalents(Player* player)
{
	if (!player)
		return false;

	if (sWorld->getBoolConfig(CONFIG_ARENA_1V1_BLOCK_FORBIDDEN_TALENTS) == false)
		return true;

	uint32 count = 0;
	for (uint32 talentId = 0; talentId < sTalentStore.GetNumRows(); ++talentId)
	{
		TalentEntry const* talentInfo = sTalentStore.LookupEntry(talentId);

		if (!talentInfo)
			continue;

		for (int8 rank = MAX_TALENT_RANK - 1; rank >= 0; --rank)
		{
			if (talentInfo->RankID[rank] == 0)
				continue;

			if (player->HasTalent(talentInfo->RankID[rank], player->GetActiveSpec()))
			{
				for (int8 i = 0; FORBIDDEN_TALENTS_IN_1V1_ARENA[i] != 0; i++)
					if (FORBIDDEN_TALENTS_IN_1V1_ARENA[i] == talentInfo->TalentTab)
						count += rank + 1;
			}
		}
	}

	if (count >= 36)
	{
		ChatHandler(player->GetSession()).SendSysMessage("You can't join, because you have invested too many points in a forbidden talent. Please edit your talents.");
		return false;
	}
	else
		return true;
}

//#endif