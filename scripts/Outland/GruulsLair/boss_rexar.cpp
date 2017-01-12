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
SDName: Boss_rexar entry 68000
SD%Complete: 90
SDComment: Странная хня с торнадо. Еще ни один нормально не заспаунился. 
Нужно будет во время теста > 5 человек посмотреть на поведение, т.е. появиться ли торнадо?
SDCategory: Gruul's Lair
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "gruuls_lair.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Containers.h"
#include "SpellHistory.h"

#define HEALER_SPELLS 7
static uint32 healer_spells[] =
{
	20473,
	47788,
	61295,
	974,
	48438,
	65139,
	47540
};

enum Spells {
	SPELL_AURA_LESS_DAMAGE = 700000,
	SPELL_TORNADO = 700010, //SpellVisual - 25160
	SPELL_TORNADO_REAL,

	SPELL_BERSERK,
	SPELL_BERSERK_UNLIM,
	
	SPELL_BOAR_SHOT,
	SPELL_BOAR_POISENED_SHOT,

	SPELL_BEAR_5DAMAGE,
	SPELL_BEAR_STUN,

	SPELL_HAWK_STUN,
	SPELL_HAWK_MISSLE,
};

enum RexarSummons {
	MINION_BOAR_ID = 61010,
	MINION_BEAR_ID,
	MINION_HAWK_ID,
	MINION_TORNADE_ID,
};

enum Events {
	EVENT_TORNADO = 0,
	EVENT_BERSERK,
	EVENT_SUMMON_BEAR,
	EVENT_SUMMON_BEAR_IN_RAGE,

	EVENT_BEAR_STUN,
	EVENT_BEAR_5DAMAGE,

	EVENT_BOAR_CHANGE_TARGET,
	EVENT_BOAR_SHOT,
	EVENT_BOAR_POISENED_SHOT,

	EVENT_HAWK_STUN,
	EVENT_HAWK_MISSLE
};

enum Phases {
	PHASE_START = 0,
	PHASE_50,
	PHASE_15
};

enum Actions {
	ACTION_BEAR_DEAD = 0,
	ACTION_START_BATTLE,
};

enum RexarTexts {
	YELL_AGGRO = 0,
	YELL_BEAR_SUMMON,
	YELL_HAWK_SUMMON,
};

class boss_rexar : public CreatureScript
{
public:
	boss_rexar() : CreatureScript("boss_rexar") { }

	struct boss_rexarAI : public BossAI
	{

		boss_rexarAI(Creature* creature) : BossAI(creature, DATA_GRUUL)
		{
			instance = creature->GetInstanceScript();
			Initialize();
		}

		void Initialize()
		{
			boar = hawk = bear = nullptr;
			phase = PHASE_START;
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		}

		void EnterCombat(Unit* /*who*/) override
		{
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			boar = me->SummonCreature(MINION_BOAR_ID, me->GetPosition());
			boar->AI()->DoAction(ACTION_START_BATTLE);

			events.ScheduleEvent(EVENT_TORNADO, 30 * IN_MILLISECONDS);
			events.ScheduleEvent(EVENT_BERSERK, 2 * 60 * IN_MILLISECONDS);
			Talk(YELL_AGGRO);
			_EnterCombat();
		}

		void JustDied(Unit* /*killer*/) override
		{
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
			_Reset();
			Initialize();
		}

		void DoAction(int32 action) override
		{
			switch (action) {
			case ACTION_BEAR_DEAD:
				me->DealDamage(me, uint32(me->GetMaxHealth() / 10));
				if (phase == PHASE_50)
					events.ScheduleEvent(EVENT_SUMMON_BEAR, 45 * IN_MILLISECONDS);
				else if (phase == PHASE_15)
					events.ScheduleEvent(EVENT_SUMMON_BEAR_IN_RAGE, 0);
				break;
			}
		}

		void DamageTaken(Unit* done_by, uint32 &damage) override
		{
			if (HealthBelowPct(51) && phase == PHASE_START) {
				phase = PHASE_50;
				events.ScheduleEvent(EVENT_SUMMON_BEAR, 1);
			}
			if (HealthBelowPct(16) && phase == PHASE_50) {
				phase = PHASE_15;
				if (!bear || bear->isDead())
					events.ScheduleEvent(EVENT_SUMMON_BEAR_IN_RAGE, 1);
				hawk = me->SummonCreature(MINION_HAWK_ID, me->GetPosition());

				if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
					hawk->Attack(target, true);
				else hawk->Attack(me->GetVictim(), true);

				hawk->AI()->DoAction(ACTION_START_BATTLE);

				Talk(YELL_HAWK_SUMMON);
			}
		}

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case EVENT_BERSERK:
				DoCast(me, SPELL_BERSERK);
				events.ScheduleEvent(EVENT_BERSERK, 2 * 60 * IN_MILLISECONDS);
				break;
			case EVENT_TORNADO:
				i = 1;
				//Создает не больше 3 торнадо, но минимальное кол-во торнадо - кол-во игроков - 1.
				while (i <= 3) {
					if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, i))
						if (TempSummon* Tornado = me->SummonCreature(MINION_TORNADE_ID, target->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 30 * IN_MILLISECONDS)) {
							Tornado->CastSpell(Tornado, SPELL_TORNADO);
						}
					i++;
				}
				events.ScheduleEvent(EVENT_TORNADO, 60 * IN_MILLISECONDS);
				break;
			case EVENT_SUMMON_BEAR:
				if (bear != nullptr && bear->isDead())
					bear->DespawnOrUnsummon();
				events.CancelEvent(EVENT_SUMMON_BEAR);
				bear = me->SummonCreature(MINION_BEAR_ID, me->GetPosition());
				bear->AI()->DoAction(ACTION_START_BATTLE);
				Talk(YELL_BEAR_SUMMON);
				break;
			case EVENT_SUMMON_BEAR_IN_RAGE:
				if (bear != nullptr && bear->isDead())
					bear->DespawnOrUnsummon();
				events.CancelEvent(EVENT_SUMMON_BEAR);
				bear = me->SummonCreature(MINION_BEAR_ID, me->GetPosition());
				bear->AI()->DoAction(ACTION_START_BATTLE);
				me->AddAura(SPELL_BERSERK_UNLIM, bear);
				break;
			}
		}

	private:
		int i;
		uint8 phase;
		InstanceScript* instance;
		Creature* bear, *boar, *hawk;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetGruulsLairAI<boss_rexarAI>(creature);
	};
};

class boss_rexar_minion_boar : public CreatureScript
{
public:
	boss_rexar_minion_boar() : CreatureScript("boss_rexar_minion_boar") { }

	struct boss_rexar_minion_boarAI : public ScriptedAI //npc 32353
	{
		boss_rexar_minion_boarAI(Creature* creature) : ScriptedAI(creature)
		{
			instance = creature->GetInstanceScript();
			Initialize();
		}

		void Initialize()
		{
			owner = ObjectAccessor::GetUnit(*me, instance->GetGuidData(DATA_GRUUL));
			if (!owner)
				me->DespawnOrUnsummon();
		};

		void DoAction(int32 action) override
		{
			if (action != ACTION_START_BATTLE) return;

			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);

			events.ScheduleEvent(EVENT_BOAR_SHOT, 2 * IN_MILLISECONDS);
			events.ScheduleEvent(EVENT_BOAR_POISENED_SHOT, 3 * IN_MILLISECONDS);
			events.ScheduleEvent(EVENT_BOAR_CHANGE_TARGET, 20 * IN_MILLISECONDS);
			me->GetMotionMaster()->MoveChase(me->GetVictim(), 20);
		}

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId)
				{
				case EVENT_BOAR_SHOT:
					DoCast(SPELL_BOAR_SHOT);
					events.ScheduleEvent(EVENT_BOAR_SHOT, 2 * IN_MILLISECONDS);
					break;
				case EVENT_BOAR_POISENED_SHOT:
					DoCast(SPELL_BOAR_POISENED_SHOT);
					events.ScheduleEvent(EVENT_BOAR_POISENED_SHOT, 35 * IN_MILLISECONDS);
					break;
				case EVENT_BOAR_CHANGE_TARGET:
					me->Attack(SelectTarget(SELECT_TARGET_RANDOM, 1), false);
					me->GetMotionMaster()->MoveChase(me->GetVictim(), 20);
					events.ScheduleEvent(EVENT_BOAR_CHANGE_TARGET, 20 * IN_MILLISECONDS);
					break;
				}
			};
		}

		Unit* owner;
		InstanceScript* instance;
		EventMap events;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetGruulsLairAI<boss_rexar_minion_boarAI>(creature);
	};
};

class boss_rexar_minion_bear : public CreatureScript
{
public:
	boss_rexar_minion_bear() : CreatureScript("boss_rexar_minion_bear") { }

	struct boss_rexar_minion_bearAI : public ScriptedAI //npc 32353
	{
		boss_rexar_minion_bearAI(Creature* creature) : ScriptedAI(creature)
		{
			instance = creature->GetInstanceScript();
			Initialize();
		}

		void Initialize() 
		{
			owner = ObjectAccessor::GetUnit(*me, instance->GetGuidData(DATA_GRUUL));
			if (!owner)
				me->DespawnOrUnsummon();
		};

		void DoAction(int32 action) override
		{
			if (action != ACTION_START_BATTLE) return;
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			me->Attack(GetHealerOrRandomTarget(), true);
			events.ScheduleEvent(EVENT_BEAR_STUN, 30 * IN_MILLISECONDS);
			events.ScheduleEvent(EVENT_BEAR_5DAMAGE, 8 * IN_MILLISECONDS);
		}

		void JustDied(Unit*) override {
			owner->GetAI()->DoAction(ACTION_BEAR_DEAD);
		}

		Unit* GetRandomTarget() {
			Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1);

			//if there aren't other units, cast on the tank
			if (!target)
				target = me->GetVictim();
			return target;
		}

		bool IsHealClass(uint8 Class) {
			switch (Class) {
			case CLASS_PALADIN:
			case CLASS_PRIEST:
			case CLASS_DRUID:
			case CLASS_SHAMAN:
				return true;
			}
			return false;
		}

		Unit* GetHealerOrRandomTarget() {
			bool healer_found = false;
			Unit* target;
			uint32 counter = 0;
			Map::PlayerList const& Players = me->GetMap()->GetPlayers();
			for (Map::PlayerList::const_iterator itr = Players.begin(); itr != Players.end(); ++itr)
			{
				if (Player* player = itr->GetSource())
					if (IsHealClass(player->getClass()))
						for (int i = 0; i < HEALER_SPELLS; i++) {
							if (player->HasSpell(healer_spells[i]))
								return player;
						}
			}
			return GetRandomTarget();

		}

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId)
				{
				case EVENT_BEAR_STUN:
					DoCast(SPELL_BEAR_STUN);
					events.ScheduleEvent(EVENT_BEAR_STUN, 30 * IN_MILLISECONDS);
					break;
				case EVENT_BEAR_5DAMAGE:
					DoCast(SPELL_BEAR_5DAMAGE);
					events.ScheduleEvent(EVENT_BEAR_5DAMAGE, 8 * IN_MILLISECONDS);
					break;
				}
			};

			if (!UpdateVictim())
				return;
			DoMeleeAttackIfReady();
		}

	private:
		Unit* owner;
		InstanceScript* instance;
		EventMap events;
	};



	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetGruulsLairAI<boss_rexar_minion_bearAI>(creature);
	};
};

class boss_rexar_minion_hawk : public CreatureScript
{
public:
	boss_rexar_minion_hawk() : CreatureScript("boss_rexar_minion_hawk") { }

	struct boss_rexar_minion_hawkAI : public ScriptedAI //npc 32353
	{
		boss_rexar_minion_hawkAI(Creature* creature) : ScriptedAI(creature)
		{
			instance = creature->GetInstanceScript();
			Initialize();
		}

		void Initialize()
		{
			owner = ObjectAccessor::GetUnit(*me, instance->GetGuidData(DATA_GRUUL));
			if (!owner)
				me->DespawnOrUnsummon();
		};

		void DoAction(int32 action) override
		{
			if (action != ACTION_START_BATTLE) return;
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.ScheduleEvent(EVENT_HAWK_STUN, 8 * IN_MILLISECONDS);
			events.ScheduleEvent(EVENT_HAWK_MISSLE, 8 * IN_MILLISECONDS);
		}

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId)
				{
				case EVENT_HAWK_STUN:
					DoCast(SPELL_HAWK_STUN);
					events.ScheduleEvent(EVENT_HAWK_STUN, 12 * IN_MILLISECONDS);
					break;
				case EVENT_HAWK_MISSLE:
					DoCast(SPELL_HAWK_MISSLE);
					events.ScheduleEvent(SPELL_HAWK_MISSLE, 20 * IN_MILLISECONDS);
					break;
				}
			};
			if (!UpdateVictim())
				return;
			DoMeleeAttackIfReady();
		}

		Unit* owner;
		InstanceScript* instance;
		EventMap events;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetGruulsLairAI<boss_rexar_minion_hawkAI>(creature);
	};
};

void AddSC_boss_rexar()
{
	new boss_rexar();
	new boss_rexar_minion_hawk();
	new boss_rexar_minion_bear();
	new boss_rexar_minion_boar();
}