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

#ifndef DRAK_THARON_KEEP_H_
#define DRAK_THARON_KEEP_H_

#define DrakTharonKeepScriptName "instance_drak_tharon_keep"
#define DataHeader               "DTK"

uint32 const EncounterCount = 4;

enum DataTypes
{
    // Encounter States/Boss GUIDs
	DATA_ZARRAK = 0,
	DATA_KALEON,
	DATA_DJAGA,
	DATA_ZADJIN,
};

enum BossEntryes
{
	BOSS_ZADJIN = 68008,
	BOSS_DJAGA = 68009,
	BOSS_KALEON = 68010,
	BOSS_ZARRAK = 68011,
};

enum DoorEntryes
{
	BOSS_ZARRAK_OPEN_DOOR = 186858,
	BOSS_ZARRAK_CLOSED_DOOR = 104600,
	BOSS_KALEON_OPEN_DOOR = 124372,
	BOSS_KALEON_CLOSED_DOOR = 183972,
	BOSS_DJAGA_CLOSED_DOOR = 146086,
	BOSS_ZADJIN_OPEN_DOOR = 186859
};

template<class AI>
AI* GetDrakTharonKeepAI(Creature* creature)
{
    return GetInstanceAI<AI>(creature, DrakTharonKeepScriptName);
}

#endif // DRAK_THARON_KEEP_H_
