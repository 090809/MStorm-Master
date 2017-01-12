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

	SPELL_BERSERK = 700070,
	SPELL_STONESKIN,
	SPELL_INDESTRUCTION,
};

enum Phases {
	PHASE_ONE = 1,
	PHASE_TWO,
};

enum Events
{
	EVENT_CAST_BERSERK,
	EVENT_CAST_STONESKIN,
	EVENT_CAST_INDESTRUCTION,
	EVENT_CAST_RING_ATTACK
};

class boss_jaga : public CreatureScript
{
public:
	boss_jaga() : CreatureScript("boss_jaga") { }

	struct boss_jagaAI : public BossAI
	{
		boss_jagaAI(Creature* creature) : BossAI(creature, DATA_DJAGA)
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
			events.ScheduleEvent(EVENT_CAST_BERSERK, urand(10000, 22000));
			events.ScheduleEvent(EVENT_CAST_STONESKIN, urand(15000, 25000));
			events.ScheduleEvent(EVENT_CAST_INDESTRUCTION, urand(4000, 6000));
		}

		void ExecuteEvent(uint32 eventid) override
		{
			switch (eventid)
			{
			case EVENT_CAST_BERSERK:
				me->AddAura(SPELL_BERSERK, me);
				events.ScheduleEvent(EVENT_CAST_BERSERK, 60000);
				break;
			case EVENT_CAST_STONESKIN:
				DoCast(SPELL_STONESKIN);
				events.ScheduleEvent(EVENT_CAST_STONESKIN, urand(115000, 125000));
				break;
			case EVENT_CAST_INDESTRUCTION:
				DoCast(SelectTarget(SELECT_TARGET_TOPAGGRO, 0),SPELL_INDESTRUCTION);
				events.ScheduleEvent(EVENT_CAST_INDESTRUCTION, (urand(3000, 8000)));
				break;
			}
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetDrakTharonKeepAI<boss_jagaAI>(creature);
	}
};

void AddSC_boss_jaga()
{
	new boss_jaga();
}
