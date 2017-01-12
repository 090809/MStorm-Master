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
#include "GridNotifiersImpl.h"

enum Spells
{
	SPELL_AURA_LESS_DAMAGE = 700000,
	SPELL_TANK_DEBUFF = 700060,
	SPELL_MASS_DEBUFF,
	SPELL_AURA_BLADE_DANCE,
	SPELL_DUMMY_FIRE,

	SPELL_AURA_STOP_ATTACK = 700065,
};

enum Phases {
	PHASE_ONE = 1,
	PHASE_TWO,
};

enum Events
{
	EVENT_TANK_DEBUFF = 1,
	EVENT_CAST_MASS_DEBUFF,
	EVENT_BACK_TO_FIGHT,
	EVENT_CHANGE_TARGET,
};

enum Actions
{
	ACTION_FIRE = 1,
};

enum Creatures
{
	DUMMY_FIRE = 28015,
};

enum Texts
{
	TEXT_ZADJIN_AGGRO,
	TEXT_ZADJIN_THIRD_PHASE
};

class boss_zadjin : public CreatureScript
{
public:
	boss_zadjin() : CreatureScript("boss_zadjin") { }

	struct boss_zadjinAI : public BossAI
	{
		boss_zadjinAI(Creature* creature) : BossAI(creature, DATA_ZADJIN)
		{
			Initialize();
			counter = 9;
		}

		void DoAction(int32 action) override
		{
			switch (action)
			{
			case ACTION_FIRE:
				me->AttackStop();
				me->AddAura(SPELL_AURA_STOP_ATTACK, me);
				me->GetMotionMaster()->MoveJump(-237.390228f, -676.355164f, 131.866165f, 10, 30); //”казать координаты центра.
				events.ScheduleEvent(EVENT_BACK_TO_FIGHT, 15000);
				ActivateFire();
				break;
			default:
				break;
			}
		}

		void ActivateFire()
		{
			std::list<Unit*> targets;
			Trinity::AnyUnitInObjectRangeCheck u_check(me, 100.0f);
			Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(me, targets, u_check);
			me->VisitNearbyObject(100.0f, searcher);

			for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
				if (!(*iter)->IsCharmedOwnedByPlayerOrPlayer() && (*iter)->ToCreature()->GetEntry() == DUMMY_FIRE)
				{
					(*iter)->ToCreature()->SetInCombatWithZone();
					(*iter)->AddAura(SPELL_DUMMY_FIRE, (*iter));
				}
		}

		void Initialize()
		{
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			counter = 9;
		}

		void JustDied(Unit* killer) override
		{
			std::list<Unit*> targets;
			Trinity::AnyUnitInObjectRangeCheck u_check(me, 100.0f);
			Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(me, targets, u_check);
			me->VisitNearbyObject(100.0f, searcher);

			for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
				if (!(*iter)->IsCharmedOwnedByPlayerOrPlayer() && (*iter)->ToCreature()->GetEntry() == DUMMY_FIRE)
					me->Kill(*iter);

			_JustDied();

			if (instance->instance->GetPlayersCountExceptGMs() >= 10)
			{
				Map::PlayerList const& Players = me->GetMap()->GetPlayers();
				for (Map::PlayerList::const_iterator itr = Players.begin(); itr != Players.end(); ++itr)
					if (Player* player = itr->GetSource())
						player->AddItem(70000, 1);
			}
			GiveRewardPoints();
		}

		void Reset() override
		{
			Initialize();
			me->RemoveAura(SPELL_AURA_STOP_ATTACK);
			_Reset();
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.SetPhase(PHASE_ONE);
			events.ScheduleEvent(EVENT_CAST_MASS_DEBUFF, 12000);
			Talk(TEXT_ZADJIN_AGGRO);
		}

		void ExecuteEvent(uint32 eventid) override
		{
			switch (eventid)
			{
			case EVENT_BACK_TO_FIGHT:
				me->RemoveAura(SPELL_AURA_STOP_ATTACK);
				me->Attack(me->GetVictim(), true);
				break;
			case EVENT_CAST_MASS_DEBUFF:
				DoCast(SPELL_MASS_DEBUFF);
				events.ScheduleEvent(EVENT_CAST_MASS_DEBUFF, 60000);
				break;
			case EVENT_TANK_DEBUFF:
				DoCast(SelectTarget(SELECT_TARGET_TOPAGGRO, 0), SPELL_TANK_DEBUFF);
				events.ScheduleEvent(EVENT_TANK_DEBUFF, urand(18000, 20000));
				break;
			case EVENT_CHANGE_TARGET:
				Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0);
				me->Attack(target, true);
				me->getThreatManager().doAddThreat(target, me->getThreatManager().getOnlineContainer().getMostHated()->getThreat()); //ѕереключаем цель.
				events.ScheduleEvent(EVENT_CHANGE_TARGET, urand(14000, 16000));
				break;
			}

		}

		void DamageTaken(Unit* attacker, uint32& damage) override
		{
			if (me->HealthBelowPct(counter * 10))
			{
				--counter;
				if (counter == 0)
				{
					Talk(TEXT_ZADJIN_THIRD_PHASE);
					me->AddAura(SPELL_AURA_BLADE_DANCE, me);
					events.ScheduleEvent(EVENT_CHANGE_TARGET, urand(1000, 6000));
				} else
					DoAction(ACTION_FIRE);
			}
			if (me->HealthBelowPct(51) && events.IsInPhase(PHASE_ONE))
			{
				events.SetPhase(PHASE_TWO);
				events.RescheduleEvent(EVENT_CAST_MASS_DEBUFF, 10000);
				events.ScheduleEvent(EVENT_TANK_DEBUFF, urand(8000, 12000));
			}
			if (attacker && attacker->ToCreature() && attacker->ToCreature()->GetEntry() == DUMMY_FIRE)
				damage = 0;
		}
	private:
		uint8 counter;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetDrakTharonKeepAI<boss_zadjinAI>(creature);
	}
};

void AddSC_boss_zadjin()
{
	new boss_zadjin();
}
