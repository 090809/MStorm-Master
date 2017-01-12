/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
SDName: Instance_Blackfathom_Deeps
SD%Complete: 50
SDComment:
SDCategory: Blackfathom Deeps
EndScriptData */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "blackfathom_deeps.h"

#define HEALTH_AURA 710000
#define HEALTH_DUMMY_ID 710000

DoorData const Doors[] =
{
	{ GO_ZIBUN_DOOR,   DATA_ZIBUN,    DOOR_TYPE_PASSAGE,	BOUNDARY_NONE },
	{ GO_KRAKEN_DOOR,  DATA_KRAKEN,   DOOR_TYPE_PASSAGE,	BOUNDARY_NONE },
	{ GO_DIONIS_DOOR,  DATA_DIONIS,   DOOR_TYPE_PASSAGE,	BOUNDARY_NONE },
	{ GO_DIONIS_DOOR,  DATA_ASGUR,    DOOR_TYPE_ROOM,		BOUNDARY_NONE },
};

class instance_blackfathom_deeps : public InstanceMapScript
{
public:
    instance_blackfathom_deeps() : InstanceMapScript("instance_blackfathom_deeps", 48) { }

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_blackfathom_deeps_InstanceMapScript(map);
    }

    struct instance_blackfathom_deeps_InstanceMapScript : public InstanceScript
    {
        instance_blackfathom_deeps_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            SetHeaders(DataHeader);
            SetBossNumber(EncounterCount);
			LoadDoorData(Doors);
        }

		void OnPlayerEnter(Player* player) override
		{
			player_count = instance->GetPlayersCountExceptGMs();
			if (player_count >= 6 && healtcreature)
			{
				healtcreature->AddAura(HEALTH_AURA, healtcreature);
			}
		}

		void OnPlayerLeave(Player* player) override
		{
			player_count = instance->GetPlayersCountExceptGMs();
			if (player_count > 1 && healtcreature)
			{
				healtcreature->RemoveAuraFromStack(HEALTH_AURA);
			}
		}

		void OnGameObjectCreate(GameObject* go) override
		{
			switch (go->GetEntry())
			{
			case GO_ZIBUN_DOOR:
			case GO_KRAKEN_DOOR:
			case GO_DIONIS_DOOR:
				AddDoor(go, true);
				break;
			}
		}

		void OnCreatureCreate(Creature* creature) override
		{
			switch (creature->GetEntry())
			{
			case HEALTH_DUMMY_ID:
				healtcreature = creature;
				break;
			default:
				break;
			}
		}

		uint8 EncounterCount = 4;
		uint8 player_count;
		Creature* healtcreature;
    };
};

void AddSC_instance_blackfathom_deeps()
{
    new instance_blackfathom_deeps();
}
