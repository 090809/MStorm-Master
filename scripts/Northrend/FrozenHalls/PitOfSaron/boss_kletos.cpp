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
SDName: Boss_Kletos
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
#include "Group.h"
#include "GridNotifiersImpl.h"

enum Spells
{
	SPELL_AURA_LESS_DAMAGE = 700000,

	SPELL_FULL_HEAL = 700038,
	SPELL_PLAGUE,
	SPELL_CHAOTIC_ARROW,
	SPELL_DARK_BOLTS,
	SPELL_TELEPORT_VISUAL = 51347,
	SPELL_BREAK_CRYSTAL_DEFEND_VISUAL = 70509,
	SPELL_NECROPOLIS_DIE = 41277,
	SPELL_AURA_DISABLE_KLETOS = 700042,
	SPELL_CONTROL = 700043,
	SPELL_AURA_VISUAL_BLOCK,
	SPELL_ACTIVATE_NECROPOLIS = 33535,
};

enum Phases
{
	PHASE_NECROPOLIS_ONE,
	PHASE_NECROPOLIS_TWO,

	KLETOS__PHASE_LEAVE_BATTLE,
	KLETOS__PHASE_RETURN_BATTLE
};

enum KletosEvents
{
	KLETOS__EVENT_PLAGUE = 1,
	KLETOS__EVENT_CHAOTIC_ARROW,
	KLETOS__EVENT_CONTROL,
};

enum KletosActions
{
	KLETOS__ACTION_NECROPOLIS_START_HEAL_KLETOS = 1,
	KLETOS__ACTION_LEAVE_BATTLE,
	KLETOS__ACTION_BACK_TO_FIGHT,
	KLETOS__SAY,
};

enum NecropolisEventsAndActions
{
	NECROPOLIS__ACTION_NECROPOLIS_START_HEAL_KLETOS = KLETOS__ACTION_NECROPOLIS_START_HEAL_KLETOS,
	NECROPOLIS__EVENT_HEAL_KLETOS,
	NECROPOLIS__ACTION_START_EVENT_CRYSTALS,
	NECROPOLIS__ACTION_JAINA_BREAK_CRYSTALS,
	NECROPOLIS__EVENT_DARK_BOLTS,
};

enum Texts
{
	TEXT_KLETOS = 0,
	TEXT_KLETOS_NEXT_PHASE,
	TEXT_KLETOS_LOSE,

	TEXT_JAINA_1 = 0,
	TEXT_JAINA_2
};

enum Summons
{
	CREATURE_CRYSTAL = 553603,
};

class boss_kletos : public CreatureScript
{
public:
	boss_kletos() : CreatureScript("boss_kletos") { }

	struct boss_kletosAI : public BossAI
	{
		boss_kletosAI(Creature* creature) : BossAI(creature, DATA_KLETOS)
		{
			Initialize();
			instance = creature->GetInstanceScript();
			Necropolis = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_NECROPOLIS_GUID));
			Jaina = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_JAINA_GUID));
		}

		void Initialize()
		{
			me->SetReactState(REACT_AGGRESSIVE);
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		}

		void DoAction(int32 action) override
		{
			switch (action)
			{
			case KLETOS__ACTION_LEAVE_BATTLE:
				me->SetReactState(REACT_PASSIVE);
				me->AttackStop();
				me->AddAura(SPELL_AURA_DISABLE_KLETOS, me);
				events.SetPhase(KLETOS__PHASE_LEAVE_BATTLE);
				events.CancelEvent(KLETOS__EVENT_PLAGUE);
				events.CancelEvent(KLETOS__EVENT_CHAOTIC_ARROW);
				Talk(TEXT_KLETOS_NEXT_PHASE);
				break;
			case KLETOS__SAY:
				Talk(TEXT_KLETOS);
				break;
			case KLETOS__ACTION_BACK_TO_FIGHT:
				me->SetReactState(REACT_AGGRESSIVE);
				me->RemoveAura(SPELL_AURA_DISABLE_KLETOS);

				events.SetPhase(KLETOS__PHASE_RETURN_BATTLE);
				
				events.ScheduleEvent(KLETOS__EVENT_CONTROL, 5000);
				events.ScheduleEvent(KLETOS__EVENT_PLAGUE, urand(3000, 7000));
				events.ScheduleEvent(KLETOS__EVENT_CHAOTIC_ARROW, urand(1000, 10000));
				break;
			}
		}

		void EnterCombat(Unit* who) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);

			if (!Necropolis)
				Necropolis = me->FindNearestCreature(NECROPOLIS_ID, 100);
			if (!Jaina)
				Jaina = me->FindNearestCreature(JAINA_ID, 100);
			
			Necropolis->SetSpeed(MOVE_RUN, 0, true);
			Necropolis->SetInCombatWith(who);
			Necropolis->CombatStart(who, true);
			Necropolis->Attack(who, false);

			Necropolis->AI()->DoAction(KLETOS__ACTION_NECROPOLIS_START_HEAL_KLETOS);
			events.ScheduleEvent(KLETOS__EVENT_PLAGUE, urand(3000, 7000));
			events.ScheduleEvent(KLETOS__EVENT_CHAOTIC_ARROW, urand(1000, 10000));
		}

		void JustReachedHome() override
		{
			_JustReachedHome();
			instance->SetBossState(DATA_KLETOS, NOT_STARTED);
			if (!Necropolis->IsAlive())
				Necropolis->Respawn(true);
		}

		void EnterEvadeMode() override
		{
			_EnterEvadeMode();
			me->GetMotionMaster()->MoveTargetedHome();
			Talk(TEXT_KLETOS_LOSE);
		}

		void JustDied(Unit* /*killer*/) override
		{
			_JustDied();
			if (instance->instance->GetPlayersCountExceptGMs() >= 10)
			{
				Map::PlayerList const& Players = me->GetMap()->GetPlayers();
				for (Map::PlayerList::const_iterator itr = Players.begin(); itr != Players.end(); ++itr)
					if (Player* player = itr->GetSource())
						player->AddItem(70001, 1);
			}
			GiveRewardPoints();
		}

		void Reset() override
		{
			_Reset();
			me->RemoveAura(SPELL_AURA_DISABLE_KLETOS);
			Initialize();
		}

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case KLETOS__EVENT_PLAGUE:
				DoCast(SelectTarget(SELECT_TARGET_RANDOM), SPELL_PLAGUE);
				events.ScheduleEvent(KLETOS__EVENT_PLAGUE, urand(43000, 46000));
				break;
			case KLETOS__EVENT_CHAOTIC_ARROW:
				DoCast(SelectTarget(SELECT_TARGET_RANDOM), SPELL_CHAOTIC_ARROW);
				events.ScheduleEvent(KLETOS__EVENT_CHAOTIC_ARROW, urand(10000, 20000));
				break;
			case KLETOS__EVENT_CONTROL:
				DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1), SPELL_CONTROL);
				events.ScheduleEvent(KLETOS__EVENT_CONTROL, 120000);
				break;
			}
		}

		//Переменные
	private:
		InstanceScript* instance;
		Creature* Necropolis, *Jaina;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetPitOfSaronAI<boss_kletosAI>(creature);
	};
};

class boss_necropolis : public CreatureScript
{
public:
	boss_necropolis() : CreatureScript("boss_necropolis") { }

	struct boss_necropolisAI : public ScriptedAI
	{
		boss_necropolisAI(Creature* creature) : ScriptedAI(creature)
		{
			Initialize();
			instance = creature->GetInstanceScript();
			Kletos = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KLETOS));
			Jaina = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_JAINA_GUID));
		}

		void Initialize()
		{
			me->SetSpeed(MOVE_RUN, 0, true);
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		}

		void DoAction(int32 action) override
		{
			switch (action)
			{
			case NECROPOLIS__ACTION_NECROPOLIS_START_HEAL_KLETOS:
				events.ScheduleEvent(NECROPOLIS__EVENT_HEAL_KLETOS, 20000);
				break;
			}
		}

		void EnterCombat(Unit* who) override
		{
			if (!Kletos)
				Kletos = me->FindNearestCreature(KLETOS_ID, 100);
			if (!Jaina)
				Jaina = me->FindNearestCreature(JAINA_ID, 100);

			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);

			Kletos->SetInCombatWith(who);
			Kletos->CombatStart(who, true);
			Kletos->Attack(who, false);

		}

		void EnterEvadeMode() override
		{
			me->NearTeleportTo(me->GetHomePosition().m_positionX, 
							   me->GetHomePosition().m_positionY, 
							   me->GetHomePosition().m_positionZ, 
							   me->GetHomePosition().GetOrientation());
			_EnterEvadeMode();
			Talk(TEXT_KLETOS_LOSE);
		}

		void JustDied(Unit* /*killer*/) override
		{
			events.Reset();
			Kletos->AI()->DoAction(KLETOS__ACTION_BACK_TO_FIGHT);
		}

		void Reset() override
		{
			me->RemoveAllAuras();
			me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
			me->SetDisableGravity(false);
			Initialize();
			events.Reset();
		}

		void ExecuteEvent(uint32 eventId)
		{
			switch (eventId)
			{
			case NECROPOLIS__EVENT_HEAL_KLETOS:
				DoCast(Kletos, SPELL_FULL_HEAL);
				events.ScheduleEvent(NECROPOLIS__EVENT_HEAL_KLETOS, 20000);
				break;
			case NECROPOLIS__EVENT_DARK_BOLTS:
				DoCast(SPELL_DARK_BOLTS);
				events.RescheduleEvent(NECROPOLIS__EVENT_DARK_BOLTS, urand(2000, 4000));
				break;
			}
		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage) override
		{
			if (me->GetHealthPct() <= 20 && events.IsInPhase(PHASE_NECROPOLIS_ONE))
			{
				events.SetPhase(PHASE_NECROPOLIS_TWO);

				events.CancelEvent(NECROPOLIS__EVENT_HEAL_KLETOS);
				events.ScheduleEvent(NECROPOLIS__EVENT_DARK_BOLTS, urand(1000, 4000));

				//Скрываем некрополиса
				me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
				//Летаем
				me->SetDisableGravity(true);
				me->GetMotionMaster()->MoveJump(1017.44167f, 166.864182f, 661.126221, 30.0f, 30.0f);

				Jaina->AI()->DoAction(NECROPOLIS__ACTION_START_EVENT_CRYSTALS);
				Kletos->AI()->DoAction(KLETOS__ACTION_LEAVE_BATTLE);
				ActivateCrystalls();
			}
		}

		void ActivateCrystalls()
		{
			std::list<Unit*> targets;
			Trinity::AnyUnitInObjectRangeCheck u_check(me, 200.0f);
			Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(me, targets, u_check);
			me->VisitNearbyObject(200.0f, searcher);

			for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
				if (!(*iter)->IsCharmedOwnedByPlayerOrPlayer() && (*iter)->ToCreature()->GetEntry() == CREATURE_CRYSTAL)
				{
					(*iter)->CastSpell(me, SPELL_ACTIVATE_NECROPOLIS);
					(*iter)->AddAura(SPELL_AURA_VISUAL_BLOCK, (*iter));
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
		Creature* Kletos, *Jaina;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetPitOfSaronAI<boss_necropolisAI>(creature);
	};
};

enum JainaActions
{
	JAINA__ACTION_START_EVENT_CRYSTALS = NECROPOLIS__ACTION_START_EVENT_CRYSTALS,
	JAINA__ACTION_BREAK_CRYSTALS,
};

enum JainaEvents
{
	EVENT_JAINA_SAY_1,
	EVENT_KLETOS_SAY_1,
	EVENT_JAINA_SAY_2,
	EVENT_TELEPORT_OUT,
};

enum JainaPhases
{
	PHASE_JAINA_ACTIVE = 1,
};

class npc_helper_jaina : public CreatureScript
{
public:
	npc_helper_jaina() : CreatureScript("npc_helper_jaina") { }

	struct npc_helper_jainaAI : public ScriptedAI
	{
		npc_helper_jainaAI(Creature* creature) : ScriptedAI(creature)
		{
			Initialize();
			instance = creature->GetInstanceScript();
			Kletos = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_KLETOS));
			Necropolis = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_NECROPOLIS_GUID));
		}

		void Initialize()
		{
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
			me->SetVisible(false);
		}

		void DoAction(int32 action) override
		{
			if (!Necropolis)
				Necropolis = me->FindNearestCreature(NECROPOLIS_ID, 200);
			if (!Kletos)
				Kletos = me->FindNearestCreature(KLETOS_ID, 200);
			switch (action)
			{
			case JAINA__ACTION_START_EVENT_CRYSTALS:
				me->setActive(true);
				me->SetVisible(true);
				DoCast(me, SPELL_TELEPORT_VISUAL);
				events.ScheduleEvent(EVENT_JAINA_SAY_1, 1000);
				events.SetPhase(PHASE_JAINA_ACTIVE);
				break;

			case JAINA__ACTION_BREAK_CRYSTALS:
				if (Creature* crystal = me->FindNearestCreature(CREATURE_CRYSTAL, 70.0f, true))
				{
					crystal->AddAura(SPELL_AURA_LESS_DAMAGE, crystal);
					me->GetMotionMaster()->MoveChase(crystal);
					crystal->CastSpell(crystal, SPELL_BREAK_CRYSTAL_DEFEND_VISUAL);
					me->Attack(crystal, true);
					crystal->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
					crystal->RemoveAura(SPELL_AURA_VISUAL_BLOCK);
				} 
				else {
					DoCast(Necropolis, SPELL_NECROPOLIS_DIE, true);
					me->DealDamage(Necropolis, Necropolis->GetHealth());
					events.SetPhase(PHASE_JAINA_ACTIVE + 1);
					events.ScheduleEvent(EVENT_TELEPORT_OUT, 500);
				}
				break;
			}
		}

		void JustDied(Unit* /*killer*/) override
		{
			events.Reset();
		}

		void Reset() override
		{
			Initialize();
			events.Reset();
		}

		void ExecuteEvent(uint32 eventId)
		{
			switch (eventId)
			{
			case EVENT_JAINA_SAY_1:
				Talk(TEXT_JAINA_1);
				events.ScheduleEvent(EVENT_KLETOS_SAY_1, 4000);
				break;
			case EVENT_KLETOS_SAY_1:
				Kletos->AI()->DoAction(KLETOS__SAY);
				events.ScheduleEvent(EVENT_JAINA_SAY_2, 4000);
				break;
			case EVENT_JAINA_SAY_2:
				Talk(TEXT_JAINA_2);
				DoAction(JAINA__ACTION_BREAK_CRYSTALS);
				break;
			case EVENT_TELEPORT_OUT:
				Kletos->AI()->DoAction(KLETOS__ACTION_BACK_TO_FIGHT);
				DoCast(SPELL_TELEPORT_VISUAL);
				me->SetVisible(false);
				me->setActive(false);
				break;
			}
		}

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			while (uint32 eventId = events.ExecuteEvent())
				ExecuteEvent(eventId);

			if (events.IsInPhase(PHASE_JAINA_ACTIVE))
			{
				Unit* target = me->GetVictim(); 
				if (target)
				{
					Creature* cr = target->ToCreature();
					if (cr->GetEntry() != CREATURE_CRYSTAL)
						DoAction(JAINA__ACTION_BREAK_CRYSTALS);
				}
				else DoAction(JAINA__ACTION_BREAK_CRYSTALS);
			}
		}

		//Переменные
	private:
		InstanceScript* instance;
		EventMap events;
		Creature* Kletos, *Necropolis;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetPitOfSaronAI<npc_helper_jainaAI>(creature);
	};
};

void AddSC_boss_kletos()
{
	new boss_kletos();
	new boss_necropolis();
	new npc_helper_jaina();
}