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
#include "ScriptedCreature.h"
#include "InstanceScript.h"
#include "drak_tharon_keep.h"

#define HEALTH_AURA 710000
#define HEALTH_DUMMY_ID 710000

DoorData const Doors[] =
{
	{ BOSS_ZARRAK_OPEN_DOOR,		DATA_ZARRAK,	DOOR_TYPE_ROOM,		BOUNDARY_NONE },
	{ BOSS_ZARRAK_CLOSED_DOOR,		DATA_ZARRAK,	DOOR_TYPE_PASSAGE,	BOUNDARY_NONE },
	{ BOSS_KALEON_OPEN_DOOR,		DATA_KALEON,	DOOR_TYPE_ROOM,		BOUNDARY_NONE },
	{ BOSS_KALEON_CLOSED_DOOR,		DATA_KALEON,	DOOR_TYPE_PASSAGE,	BOUNDARY_NONE },
	{ BOSS_DJAGA_CLOSED_DOOR,		DATA_DJAGA,		DOOR_TYPE_PASSAGE,	BOUNDARY_NONE },
	{ BOSS_ZADJIN_OPEN_DOOR,		DATA_ZADJIN,	DOOR_TYPE_ROOM,		BOUNDARY_NONE },
};

class instance_drak_tharon_keep : public InstanceMapScript
{
    public:
        instance_drak_tharon_keep() : InstanceMapScript(DrakTharonKeepScriptName, 600) { }

        struct instance_drak_tharon_keep_InstanceScript : public InstanceScript
        {
            instance_drak_tharon_keep_InstanceScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(EncounterCount);
				LoadDoorData(Doors);
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
					case DATA_ZARRAK:
						ZarrakGuid = creature->GetGUID();
						break;
					case DATA_KALEON:
						KaleonGuid = creature->GetGUID();
						break;
					case DATA_DJAGA:
						DjagaGuid = creature->GetGUID();
						break;
					case DATA_ZADJIN:
						ZadjinGuid = creature->GetGUID();
						break;
					case HEALTH_DUMMY_ID:
						healtcreature = creature;
					default:
						break;
					}
            }

			void OnGameObjectCreate(GameObject* go) override
			{
				switch (go->GetEntry())
				{
				case BOSS_ZARRAK_OPEN_DOOR:
				case BOSS_ZARRAK_CLOSED_DOOR:
				case BOSS_KALEON_OPEN_DOOR:
				case BOSS_KALEON_CLOSED_DOOR:
				case BOSS_DJAGA_CLOSED_DOOR:
				case BOSS_ZADJIN_OPEN_DOOR:
					AddDoor(go, true);
					break;
				}
			}

			void OnGameObjectRemove(GameObject* go) override
			{
				switch (go->GetEntry())
				{
				case BOSS_ZARRAK_OPEN_DOOR:
				case BOSS_ZARRAK_CLOSED_DOOR:
				case BOSS_KALEON_OPEN_DOOR:
				case BOSS_KALEON_CLOSED_DOOR:
				case BOSS_DJAGA_CLOSED_DOOR:
				case BOSS_ZADJIN_OPEN_DOOR:
					AddDoor(go, false);
					break;
				}
			}

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_ZARRAK:
                        return ZarrakGuid;
                    case DATA_KALEON:
                        return KaleonGuid;
                    case DATA_DJAGA:
                        return DjagaGuid;
                    case DATA_ZADJIN:
                        return ZadjinGuid;
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
			ObjectGuid ZadjinGuid, DjagaGuid, KaleonGuid, ZarrakGuid;
			uint8 player_count;
			Creature* healtcreature;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_drak_tharon_keep_InstanceScript(map);
        }
};

void AddSC_instance_drak_tharon_keep()
{
    new instance_drak_tharon_keep();
}
