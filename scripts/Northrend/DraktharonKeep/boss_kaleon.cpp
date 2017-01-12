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
#include "drak_tharon_keep.h"

enum Spells
{
	SPELL_AURA_LESS_DAMAGE = 700000,

	SPELL_WATER_PRISON = 700054,
	SPELL_BLOOD_DRINK = 700056,
	SPELL_WATER_ARROW,
};

enum Phases {
	PHASE_ONE = 1,
	PHASE_TWO,
};

enum Events
{
	EVENT_CAST_WATER_PRISON,
	EVENT_CAST_BLOOD_DRINK,
	EVENT_CAST_WATER_ARROW,
};

class boss_kaleon : public CreatureScript
{
public:
	boss_kaleon() : CreatureScript("boss_kaleon") { }

	struct boss_kaleonAI : public BossAI
	{
		boss_kaleonAI(Creature* creature) : BossAI(creature, DATA_KALEON)
		{
			Initialize();
		}

		void Initialize()
		{
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		}

		void Reset() override
		{
			Initialize();
			_Reset();
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.SetPhase(PHASE_ONE);
			events.ScheduleEvent(EVENT_CAST_WATER_PRISON, urand(3000, 17000));
			events.ScheduleEvent(EVENT_CAST_BLOOD_DRINK, urand(8000, 18000));
			
		}

		void ExecuteEvent(uint32 eventid) override
		{
			switch (eventid)
			{
			case EVENT_CAST_WATER_PRISON:
				DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1), SPELL_WATER_PRISON);
				events.ScheduleEvent(EVENT_CAST_WATER_PRISON, urand(25000, 31000));
				break;
			case EVENT_CAST_BLOOD_DRINK:
				DoCastAOE(SPELL_BLOOD_DRINK);
				events.ScheduleEvent(EVENT_CAST_BLOOD_DRINK, urand(55000, 61000));
				break;
			case EVENT_CAST_WATER_ARROW:
				DoCast(SPELL_WATER_ARROW);
				events.ScheduleEvent(EVENT_CAST_WATER_ARROW, 2000);
				break;
			}
		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage) override
		{
			if (HealthBelowPct(21) && events.IsInPhase(PHASE_ONE))
			{
				events.SetPhase(PHASE_TWO);
				me->Attack(me->GetVictim(), false);
				events.ScheduleEvent(EVENT_CAST_WATER_ARROW, 200);
				events.RescheduleEvent(EVENT_CAST_BLOOD_DRINK, urand(10000, 17000));
				events.RescheduleEvent(EVENT_CAST_WATER_PRISON, urand(5000, 9000));
			}
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetDrakTharonKeepAI<boss_kaleonAI>(creature);
	}
};

void AddSC_boss_kaleon()
{
	new boss_kaleon();
}
