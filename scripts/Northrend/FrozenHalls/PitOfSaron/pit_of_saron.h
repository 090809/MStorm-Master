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

#ifndef PIT_OF_SARON_H_
#define PIT_OF_SARON_H_

#define PoSScriptName "instance_pit_of_saron"
#define DataHeader "POS"

uint32 const EncounterCount = 3;

enum Creatures
{
	JAINA_ID = 553604,
	KLETOS_ID = 68003,
	NECROPOLIS_ID,
};

enum DataTypes
{
    // Encounter states and GUIDs
    DATA_SHAN				= 0,
    DATA_HURD               = 1,
    DATA_KLETOS             = 2,

    // GUIDs
    DATA_JAINA_GUID			= 3,
	DATA_NECROPOLIS_GUID	= 4,
	DATA_ICE_WALL			= 5,
};

enum GameObjectIds
{
	GO_SHAN_DOOR				= 181228,
	GO_HURD_DOOR				= 181199,
	GO_ICE_WALL					= 201885,
	GO_KLETOS_DOOR				= 183847,
};

template<class AI>
AI* GetPitOfSaronAI(Creature* creature)
{
    return GetInstanceAI<AI>(creature, PoSScriptName);
}

#endif // PIT_OF_SARON_H_
