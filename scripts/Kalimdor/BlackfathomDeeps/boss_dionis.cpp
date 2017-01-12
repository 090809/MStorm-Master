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
#include "blackfathom_deeps.h"
#include "GridNotifiersImpl.h"

#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum Spells
{
	SPELL_AURA_LESS_DAMAGE = 700000,

	SPELL_LITH = 700075,
	SPELL_FROST_AURA = 700073
};

enum Phases {
	PHASE_ONE = 1,
	PHASE_TWO,
};

enum Events
{
	EVENT_SPAWN_MINIONS = 1,
	EVENT_CAST_LITH,
	EVENT_SUMMON_FROST_SPHERE,
};

enum NPCS
{
	NPC_PORTAL = 30679,
	NPC_MINION = 68017,
	NPC_FROST_SPHERE = 161619,
};

class boss_dionis : public CreatureScript
{
public:
	boss_dionis() : CreatureScript("boss_dionis") { }

	struct boss_dionisAI : public BossAI
	{
		boss_dionisAI(Creature* creature) : BossAI(creature, DATA_DIONIS)
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

		void JustDied(Unit* killer) override
		{
			_JustDied();
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.SetPhase(PHASE_ONE);
			events.ScheduleEvent(EVENT_SPAWN_MINIONS, urand(40000, 80000));
			events.ScheduleEvent(EVENT_CAST_LITH, urand(4000, 8000));
			events.ScheduleEvent(EVENT_SUMMON_FROST_SPHERE, urand(8000, 12000));
			TakePortals();
		}

		void TakePortals() {
			std::list<Unit*> targets;
			Trinity::AnyUnitInObjectRangeCheck u_check(me, 100.0f);
			Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(me, targets, u_check);
			me->VisitNearbyObject(100.0f, searcher);

			for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
				if (!(*iter)->IsCharmedOwnedByPlayerOrPlayer() && (*iter)->ToCreature()->GetEntry() == NPC_PORTAL)
					portals.push_back(*iter);
		}

		void ExecuteEvent(uint32 eventid) override
		{
			switch (eventid)
			{
			case EVENT_SPAWN_MINIONS:
				for (uint8 i = 0; i < portals.size(); i++)
					me->SummonCreature(NPC_MINION, portals[i]->GetPosition())->Attack(SelectTarget(SELECT_TARGET_TOPAGGRO, 1), true);
				events.ScheduleEvent(EVENT_SPAWN_MINIONS, urand(60000, 100000));
				break;
			case EVENT_CAST_LITH:
				DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1), SPELL_LITH);
				events.ScheduleEvent(EVENT_CAST_LITH, urand(8000, 12000));
				break;
			case EVENT_SUMMON_FROST_SPHERE:
				Creature* Sphere = me->SummonCreature(NPC_FROST_SPHERE, SelectTarget(SELECT_TARGET_RANDOM, 0)->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 10000);
				Sphere->GetMotionMaster()->MoveIdle();
				Sphere->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
				me->AddAura(SPELL_FROST_AURA, Sphere);
				events.ScheduleEvent(EVENT_SUMMON_FROST_SPHERE, urand(8000, 12000));
				break;
			}
		}

		std::vector<Unit*> portals;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetInstanceAI<boss_dionisAI>(creature);
	}
};

void AddSC_boss_dionis()
{
	new boss_dionis();
}