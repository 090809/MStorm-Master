/*
 * Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
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

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "gruuls_lair.h"

enum Bosses {
	REXAR_ID = 68000,
	ORRI_ID,
};

#define HEALTH_AURA 710000
#define HEALTH_DUMMY_ID 710000

DoorData const doorData[] =
{
    { GO_MAULGAR_DOOR,  DATA_MAULGAR,   DOOR_TYPE_PASSAGE,  BOUNDARY_NONE }, //Дверь орри
    { GO_GRUUL_DOOR,    DATA_GRUUL,     DOOR_TYPE_ROOM,     BOUNDARY_NONE }, //Дверь Рексара
    { 0,                0,              DOOR_TYPE_ROOM,     BOUNDARY_NONE } // END
};

class instance_gruuls_lair : public InstanceMapScript
{
    public:
        instance_gruuls_lair() : InstanceMapScript(GLScriptName, 565) { }

        struct instance_gruuls_lair_InstanceMapScript : public InstanceScript
        {
            instance_gruuls_lair_InstanceMapScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(EncounterCount);
                LoadDoorData(doorData);
            }

            void OnCreatureCreate(Creature* creature) override
            {
				switch (creature->GetEntry())
				{
				case REXAR_ID:
					RexarGUID = creature->GetGUID();
					break;
				case ORRI_ID:
					OrriGUID = creature->GetGUID();
					break;
				case HEALTH_DUMMY_ID:
					healtcreature = creature;
				default:
					break;
				}
            }

            void OnCreatureRemove(Creature* creature) override
            {
				switch (creature->GetEntry())
				{
				default:
					break;
				}
            }

            void OnGameObjectCreate(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_MAULGAR_DOOR:
                    case GO_GRUUL_DOOR:
                        AddDoor(go, true);
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectRemove(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_MAULGAR_DOOR:
                    case GO_GRUUL_DOOR:
                        AddDoor(go, false);
                        break;
                    default:
                        break;
                }
            }

			void Update(uint32 /*diff*/) override {
				if (!IsEncounterInProgress())
				{
				}
			}

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
					case DATA_GRUUL:
						return RexarGUID;
					case DATA_MAULGAR:
						return OrriGUID;
                    default:
                        break;
                }
                return ObjectGuid::Empty;
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

        protected:
            ObjectGuid RexarGUID, OrriGUID;
			uint8 player_count;
			Creature* healtcreature;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_gruuls_lair_InstanceMapScript(map);
        }
};

void AddSC_instance_gruuls_lair()
{
    new instance_gruuls_lair();
}
