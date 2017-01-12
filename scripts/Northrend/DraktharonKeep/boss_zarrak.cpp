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

/*
* Comment: MAYBE need more improve the "Raptor Call".
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "drak_tharon_keep.h"

#define SUMMON_COPYES_ID 68060

enum Spells
{
	SPELL_AURA_LESS_DAMAGE = 700000,

	SPELL_WEAPON_DIST = 700050,
	SPELL_AMANI,
	SPELL_NONMAGIC,
	SPELL_NONPHISIC,

};

enum Phases {
	PHASE_ONE = 1,
	PHASE_TWO,
	PHASE_THREE,
	PHASE_FOUR,
};

enum Events
{
	EVENT_CAST_WEAPON_DIST,
	EVENT_AMANI,
};

class boss_zarrak : public CreatureScript
{
public:
	boss_zarrak() : CreatureScript("boss_zarrak") { }

	struct boss_zarrakAI : public BossAI
	{
		boss_zarrakAI(Creature* creature) : BossAI(creature, DATA_ZARRAK)
		{
			Initialize();
		}

		void Initialize()
		{
			counter = 0;
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		}

		void Reset() override
		{
			Initialize();
			_Reset();
		}

		void DoAction(int32 action) override
		{
			counter += action;
			if (counter >= 2)
			{
				me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
				me->DealDamage(me, me->GetMaxHealth());
			}
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.ScheduleEvent(EVENT_CAST_WEAPON_DIST, urand(2000, 6000));
			events.ScheduleEvent(EVENT_AMANI, urand(7000, 10000));
			events.SetPhase(PHASE_ONE);
		}

		void SummonedCreatureDespawn(Creature* creature) override
		{
			counter++;
			if (counter >= 2)
			{
				events.SetPhase(PHASE_FOUR + 1);
				me->Kill(me, false);
				me->DealDamage(me, me->GetMaxHealth());
			}
		}

		void ExecuteEvent(uint32 eventid) override
		{
			switch (eventid)
			{
			case EVENT_CAST_WEAPON_DIST:
				DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1), SPELL_WEAPON_DIST);
				events.ScheduleEvent(EVENT_CAST_WEAPON_DIST, urand(24000, 32000));
				break;
			case EVENT_AMANI:
				DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1), SPELL_AMANI);
				events.ScheduleEvent(EVENT_AMANI, urand(50000, 64000));
				break;
			}
		}

		void DamageTaken(Unit* attacker, uint32& damage) override
		{
			if (HealthBelowPct(51) && events.IsInPhase(PHASE_ONE))
			{
				me->AddAura(SPELL_NONMAGIC, me);
				events.SetPhase(PHASE_TWO);
			}
			if (HealthBelowPct(31) && events.IsInPhase(PHASE_TWO))
			{
				me->AddAura(SPELL_NONPHISIC, me);
				me->RemoveAura(SPELL_NONMAGIC);
				events.SetPhase(PHASE_THREE);
			}
			if (HealthBelowPct(11) && events.IsInPhase(PHASE_THREE))
			{
				for (int i = 0; i < 2; i++)
				{
					Creature* copy = me->SummonCreature(SUMMON_COPYES_ID, me->GetPosition(), TEMPSUMMON_CORPSE_DESPAWN);
					me->AddAura(SPELL_NONMAGIC + i, copy);
					copy->Attack(me->GetVictim(), true);
				}
				events.Reset();
				events.SetPhase(PHASE_FOUR);
				me->AttackStop();
				me->SetReactState(REACT_PASSIVE);
				me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
			}
			if (events.IsInPhase(PHASE_FOUR) && attacker != me)
				damage = 0;
		}


	private:
		uint8 counter;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetDrakTharonKeepAI<boss_zarrakAI>(creature);
	}
};

class npc_zarrak_copyes : public CreatureScript
{
public:
	npc_zarrak_copyes() : CreatureScript("npc_zarrak_copyes") { }

	struct npc_zarrak_copyesAI : public ScriptedAI
	{
		npc_zarrak_copyesAI(Creature* creature) : ScriptedAI(creature)
		{
			Initialize();
		}

		void Initialize()
		{
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		}

		void EnterCombat(Unit* /*who*/) override
		{
			me->setActive(true);
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.ScheduleEvent(EVENT_CAST_WEAPON_DIST, urand(2000, 6000));
			events.ScheduleEvent(EVENT_AMANI, urand(7000, 10000));
		}

		void JustDied(Unit* /*killer*/) override
		{
			if (me->GetOwner())
				me->GetOwner()->GetAI()->DoAction(1);
			else
				me->FindNearestCreature(68011, 100)->AI()->DoAction(1);
		}

		void ExecuteEvent(uint32 eventid)
		{
			switch (eventid)
			{
			case EVENT_CAST_WEAPON_DIST:
				DoCast(SPELL_WEAPON_DIST);
				events.ScheduleEvent(EVENT_CAST_WEAPON_DIST, urand(24000, 32000));
				break;
			case EVENT_AMANI:
				DoCast(EVENT_AMANI);
				events.ScheduleEvent(EVENT_AMANI, urand(50000, 64000));
				break;
			}
		}

		void UpdateAI(uint32 diff) override
		{
			if (!UpdateVictim())
				return;

			events.Update(diff);

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			while (uint32 eventId = events.ExecuteEvent())
				ExecuteEvent(eventId);
		}

		//Переменные
	private:
		InstanceScript* instance;
		EventMap events;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetDrakTharonKeepAI<npc_zarrak_copyesAI>(creature);
	};
};

void AddSC_boss_zarrak()
{
	new boss_zarrak();
}
