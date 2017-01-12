/*
* CreateDate: 24.07.2016 by Pallam, <http://MagicStorm.ru/>
* Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
* Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Boss_Orri entry 68001
SD%Complete: 100
SDComment: N-tested
SDCategory: Saron Pit
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "pit_of_saron.h"
#include "Map.h"
#include "Player.h"

enum Spells 
{
	SPELL_AURA_LESS_DAMAGE = 700000,

	SPELL_FROST_BREATH = 700030,
	SPELL_SUMMON = 57604,
	SPELL_FREEZE = 700031,
	SPELL_EXPLOSEN = 700032,
	SPELL_SHAN_RAGE = 700034,
	SPELL_SHAN_CHARGE = 700035,
};

enum Phases 
{
	PHASE_ONE = 1,
	PHASE_TWO,
	PHASE_BETWEEN,
};

enum ShanEventsAndAction 
{
	EVENT_SPAWN_SPIRIT = 1,
	EVENT_SUMMON_AND_FREEZE_PLAYERS,
	EVENT_DELETE_SPIRITS,
	EVENT_SPELL_FROST_BREATH,
	EVENT_SHAN_MINDLESS,

	ACTION_HELL,
	ACTION_BACK_TO_FIGHT,
};

enum ShanSummons
{
	FROST_SPIRIT = 123456,
};

Position const CenterOfLake = { 885.182983f, 272.056000f, 531.174988, 3.055970f };

class boss_shan : public CreatureScript
{
public:
	boss_shan() : CreatureScript("Boss_shan") { }

	struct boss_shanAI : public BossAI
	{
		boss_shanAI(Creature* creature) : BossAI(creature, DATA_SHAN)
		{
			Initialize();
			instance = creature->GetInstanceScript();
		}

		void Initialize()
		{
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			__spiritcounter = 0;
			FrozenSpirits.clear();
			events.ScheduleEvent(EVENT_SPELL_FROST_BREATH, urand(25000, 32000), 0U, PHASE_ONE);
		}

		void DoAction(int32 action) override
		{
			switch (action)
			{
			case ACTION_HELL:
				__spiritcounter = 0;
				FrozenSpirits.clear();
				me->GetMotionMaster()->MoveJump(CenterOfLake, 45.0f, 15.0f);
				speed_run = me->GetSpeed(MOVE_RUN) / 7.0f;
				me->SetSpeed(MOVE_RUN, 0, true);
				events.ScheduleEvent(EVENT_SUMMON_AND_FREEZE_PLAYERS, 3 * IN_MILLISECONDS, 0U, PHASE_BETWEEN);
				events.ScheduleEvent(EVENT_SPAWN_SPIRIT, 9 * IN_MILLISECONDS, 0U, PHASE_BETWEEN);
				events.ScheduleEvent(EVENT_DELETE_SPIRITS, 35 * IN_MILLISECONDS, 0U, PHASE_BETWEEN);
				break;
			case ACTION_BACK_TO_FIGHT:
				me->SetSpeed(MOVE_RUN, speed_run, true);
				me->Attack(SelectTarget(SELECT_TARGET_RANDOM, 0), true);
				break;
			}
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			events.SetPhase(PHASE_ONE);
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		}

		void JustDied(Unit* /*killer*/) override
		{
			_JustDied();
		}

		void Reset() override
		{
			Initialize();
			_Reset();
		}

		void ExecuteEvent(uint32 eventId) override
		{
			Map::PlayerList const& Players = me->GetMap()->GetPlayers();
			Unit* target;

			switch (eventId)
			{
			case EVENT_SUMMON_AND_FREEZE_PLAYERS:
				for (Map::PlayerList::const_iterator _itr = Players.begin(); _itr != Players.end(); ++_itr)
				{
					if (Player* player = _itr->GetSource())
					{
						player->CastSpell(me, SPELL_SUMMON, TRIGGERED_FULL_MASK); 
						me->AddAura(SPELL_FREEZE, player);
					}
				}
				break;
			case EVENT_SPAWN_SPIRIT:
				if (__spiritcounter++ < 8) {
					for (Map::PlayerList::const_iterator _itr = Players.begin(); _itr != Players.end(); ++_itr)
						if (Player* player = _itr->GetSource())
						{
							Creature* spirit = me->SummonCreature(FROST_SPIRIT, me->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 30 * IN_MILLISECONDS);
							me->AddAura(SPELL_EXPLOSEN, spirit);
							spirit->Attack(player, true);
							FrozenSpirits.push_back(spirit);
						}
					events.ScheduleEvent(EVENT_SPAWN_SPIRIT, 3 * IN_MILLISECONDS, 0U, PHASE_BETWEEN);
				}
				
				break;
			case EVENT_DELETE_SPIRITS:
				for (std::vector<Creature*>::const_iterator iter = FrozenSpirits.begin(); iter != FrozenSpirits.end(); ++iter) 
					if ((*iter)->IsAlive())
						(*iter)->DespawnOrUnsummon();
				FrozenSpirits.clear();
				events.SetPhase(PHASE_ONE);
				DoAction(ACTION_BACK_TO_FIGHT);
				break;
			case EVENT_SHAN_MINDLESS:
				target = SelectTarget(SELECT_TARGET_RANDOM, 0);

				me->AddThreat(target, 1000.0f);
				me->Attack(target, true);
				DoCast(target, SPELL_SHAN_CHARGE);
				events.ScheduleEvent(EVENT_SHAN_MINDLESS, urand(17000, 24000), 0U, PHASE_TWO);
				break;
			case EVENT_SPELL_FROST_BREATH:
				DoCastAOE(SPELL_FROST_BREATH);
				events.ScheduleEvent(EVENT_SPELL_FROST_BREATH, urand(25000, 32000), 0U, PHASE_ONE);
				break;
			}
		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage) override
		{
			if (( me->HealthAbovePct(84) && me->HealthBelowPct(86)
			   || me->HealthAbovePct(59) && me->HealthBelowPct(61)
			   || me->HealthAbovePct(34) && me->HealthBelowPct(36)) 
					&& events.IsInPhase(PHASE_ONE))
			{
				events.SetPhase(PHASE_BETWEEN);
				me->AttackStop();
				DoAction(ACTION_HELL);
			}

			if (HealthBelowPct(31) && !events.IsInPhase(PHASE_TWO))
			{
				for (std::vector<Creature*>::const_iterator iter = FrozenSpirits.begin(); iter != FrozenSpirits.end(); ++iter)
					(*iter)->DespawnOrUnsummon();
				FrozenSpirits.clear();
				DoAction(ACTION_BACK_TO_FIGHT);

				events.SetPhase(PHASE_TWO);
				DoCast(SPELL_SHAN_RAGE);
				events.ScheduleEvent(EVENT_SHAN_MINDLESS, urand(17000, 24000), 0U, PHASE_TWO);
			}
		}

		//Переменные
	private:
		uint8 __spiritcounter;
		InstanceScript* instance;
		std::vector<Creature*> FrozenSpirits;
		float speed_run;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetPitOfSaronAI<boss_shanAI>(creature);
	};
};

void AddSC_boss_shan()
{
	new boss_shan();
}