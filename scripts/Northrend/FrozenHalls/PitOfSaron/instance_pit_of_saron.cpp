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
#include "pit_of_saron.h"
#include "Player.h"

#define HEALTH_AURA 710000
#define HEALTH_DUMMY_ID 710000
DoorData const Doors[] =
{
    { GO_SHAN_DOOR,   DATA_SHAN,   DOOR_TYPE_ROOM,		BOUNDARY_NONE },
	{ GO_HURD_DOOR,   DATA_HURD,   DOOR_TYPE_ROOM,		BOUNDARY_NONE },
    //{ GO_ICE_WALL,    DATA_HURD,   DOOR_TYPE_PASSAGE,	BOUNDARY_NONE },
	{ GO_KLETOS_DOOR, DATA_KLETOS, DOOR_TYPE_ROOM,		BOUNDARY_NONE },
};

class instance_pit_of_saron : public InstanceMapScript
{
    public:
        instance_pit_of_saron() : InstanceMapScript(PoSScriptName, 658) { }

        struct instance_pit_of_saron_InstanceScript : public InstanceScript
        {
            instance_pit_of_saron_InstanceScript(Map* map) : InstanceScript(map)
            {
                SetHeaders(DataHeader);
                SetBossNumber(EncounterCount);
                LoadDoorData(Doors);
				player_count = 0;
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

            void OnCreatureCreate(Creature* creature) override
            {
				switch (creature->GetEntry())
                {
					case JAINA_ID:
						_jainaGUID = creature->GetGUID();
						break;
					case KLETOS_ID:
						_kletosGUID = creature->GetGUID();
						break;
					case NECROPOLIS_ID:
						_necropolisGUID = creature->GetGUID();
						break;
					case HEALTH_DUMMY_ID:
						healtcreature = creature;
						break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_ICE_WALL:
						_ice_wallGUID = go->GetGUID();
					case GO_SHAN_DOOR:
					case GO_HURD_DOOR:
					case GO_KLETOS_DOOR:
                        AddDoor(go, true);
                        break;
                }
            }

            void OnGameObjectRemove(GameObject* go) override
            {
                switch (go->GetEntry())
                {
					case GO_ICE_WALL:
					case GO_SHAN_DOOR:
					case GO_HURD_DOOR:
					case GO_KLETOS_DOOR:
                        AddDoor(go, false);
                        break;
                }
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
					case DATA_SHAN:
						if (state == DONE && GetBossState(DATA_HURD) == DONE) DoUseDoorOrButton(GetGuidData(DATA_ICE_WALL));
						break;
					case DATA_HURD:
						if (state == DONE && GetBossState(DATA_SHAN) == DONE) DoUseDoorOrButton(GetGuidData(DATA_ICE_WALL)); 
						break;
                    default:
                        break;
                }

                return true;
            }

            uint32 GetData(uint32 type) const override
            {
                switch (type)
                {
                    default:
                        break;
                }

                return 0;
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
					case DATA_ICE_WALL:
						return _ice_wallGUID;
					case DATA_KLETOS:
						return _kletosGUID;
					case DATA_NECROPOLIS_GUID:
						return _necropolisGUID;
                    case DATA_JAINA_GUID:
                        return _jainaGUID;
                    default:
                        break;
                }

                return ObjectGuid::Empty;
            }

        private:
            ObjectGuid _jainaGUID, _necropolisGUID, _kletosGUID, _ice_wallGUID;
			uint8 player_count;
			std::vector<Creature*> Creatures;
			Creature* healtcreature;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_pit_of_saron_InstanceScript(map);
        }
};

void AddSC_instance_pit_of_saron()
{
    new instance_pit_of_saron();
}
