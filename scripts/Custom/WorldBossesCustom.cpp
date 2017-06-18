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
SDName: Custom World Bosses
SD%Complete: 50
SDComment: –еализован вестник бури.
SDCategory: World Bosses
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Containers.h"
#include "SpellHistory.h"
#include "GridNotifiersImpl.h"

enum Spells {
	SPELL_AURA_LESS_DAMAGE = 700000,
};

enum SpellsShtorm {
	SPELL_THUNDER = 700020,
	SPELL_SUPERCONDUCTOR,
	SPELL_THUNDER_SHIELD,
};

enum EventsShtorm {
	EVENT_THUNDER = 1,
	EVENT_SUPERCONDUCTOR,
};

enum TextsShtorm {
	TEXT_AGGRO,
	TEXT_THUNDER,
	TEXT_DIED,
};

class WorldBoss_Shtorm : public CreatureScript
{
public:
	WorldBoss_Shtorm() : CreatureScript("WorldBoss_Shtorm") { }

	struct WorldBoss_ShtormAI : public WorldBossAI
	{
		WorldBoss_ShtormAI(Creature* creature) : WorldBossAI(creature)
		{
			Initialize();
		}

		void Initialize()
		{
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		};

		void JustDied(Unit* /*killer*/) override {
			Talk(TEXT_DIED);
		}

		void EnterCombat(Unit* /*who*/) override
		{
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			Talk(TEXT_AGGRO);
			events.ScheduleEvent(EVENT_THUNDER, 3 * IN_MILLISECONDS);
			events.ScheduleEvent(EVENT_SUPERCONDUCTOR, 8 * IN_MILLISECONDS);
		}

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case EVENT_THUNDER:
				Talk(TEXT_THUNDER);
				DoCast(SPELL_THUNDER);
				events.ScheduleEvent(EVENT_THUNDER, (28 + urand(0, 3)) * IN_MILLISECONDS);
				break;
			case EVENT_SUPERCONDUCTOR:
				DoCastAOE(SPELL_SUPERCONDUCTOR);
				events.ScheduleEvent(EVENT_SUPERCONDUCTOR, (2 * 60 + urand(0, 2) - urand(0, 4)) * IN_MILLISECONDS);
				break;
			}
		}

		void DamageTaken(Unit* /*done_by*/, uint32& /*damage*/) override
		{
			if (HealthBelowPct(51) && !me->HasAura(SPELL_THUNDER_SHIELD))
				DoCast(SPELL_THUNDER_SHIELD);
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new WorldBoss_ShtormAI(creature);
	}
};

enum SpiderManActions {
	ACTION_SPIDER_DIED,
	ACTION_NEXT_WAVE
};

enum MinionsSpiderMan {
	MINION_SPIDER_ID    = 556607,
	SPIDER_MAN_ID		= 556605,
};

Position const SpiderPos[3] =
{
	{ 3675.789f, 2058.626709f, 3.0478f, 2.7063f },
	{ 3636.753f, 2075.998047f, 7.131971f, 5.8503f },
	{ 3642.449f, 2034.574463f, 4.276724f, 1.224323f },
};

enum SpiderManEvents {
	EVENT_NEXT_WAVE = 1,

	EVENT_SPIDERMAN_START_BATTLE,
	EVENT_SPIDERMAN_FEAR,
	EVENT_SPIDERMAN_HEALING_ATTACKS,
	EVENT_SPIDERMAN_KISS_OF_PLAGUE,

	EVENT_SPIDERMAN_MINION_SPIDER_POISEN,
};

enum SpiderManSpells {
	SPELL_KISS_OF_PLAGUE = SPELL_THUNDER_SHIELD + 2,
	SPELL_HEALING_ATTACKS,
	SPELL_SPIDERMAN_FEAR,
	SPELL_SPIDERMAN_MINION_SPIDER_POISEN
};

class WorldBoss_SpiderMan : public CreatureScript
{
public:
	WorldBoss_SpiderMan() : CreatureScript("WorldBoss_SpiderMan") { }

	struct WorldBoss_SpiderManAI : public WorldBossAI
	{
		WorldBoss_SpiderManAI(Creature* creature) : WorldBossAI(creature)
		{
			Initialize();
		}

		void Initialize()
		{
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);

			wave = 0;
			SpiderAliveCount = 0;

			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
		};

		void Reset() override
		{
			_Reset();
			Initialize();
		}

		void JustDied(Unit* /*killer*/) override {
			Talk(TEXT_DIED);
		}

		void DoAction(int32 action) override
		{
			switch (action) {
			case ACTION_SPIDER_DIED:
				SpiderAliveCount -= 1;
				if (SpiderAliveCount <= 0)
				{
					DoAction(ACTION_NEXT_WAVE);
					me->DealDamage(me, uint32(me->GetMaxHealth() * 0.1f));
				}
				break;
			case ACTION_NEXT_WAVE:
				if (wave >= 5)
				{
					me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
					me->SetDisplayId(16137);
					me->RemoveAura(39258);
					events.ScheduleEvent(EVENT_SPIDERMAN_START_BATTLE, 1);
					return;
				}
				events.ScheduleEvent(EVENT_NEXT_WAVE, 15000);
			}
		}

		void EnterCombat(Unit* /*who*/) override
		{
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
			me->StopMoving();
			me->AttackStop();

			me->AddAura(39258, me);

			{
			wave++;
			SpiderAliveCount = 3;
			for (int i = 0; i < SpiderAliveCount; i++)
				me->SummonCreature(MINION_SPIDER_ID, SpiderPos[i], TEMPSUMMON_DEAD_DESPAWN)->Attack(me->GetVictim(), true);
			}
		}

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case EVENT_SPIDERMAN_START_BATTLE:
				me->Attack(SelectTarget(SELECT_TARGET_NEAREST, 0), true);
				events.ScheduleEvent(EVENT_SPIDERMAN_FEAR, (28 + urand(0, 4)) * IN_MILLISECONDS);
				events.ScheduleEvent(EVENT_SPIDERMAN_HEALING_ATTACKS, (12 + urand(0, 12)) * IN_MILLISECONDS);
				events.ScheduleEvent(EVENT_SPIDERMAN_KISS_OF_PLAGUE, (27 + urand(0, 6)) * IN_MILLISECONDS);
				break;
			case EVENT_SPIDERMAN_FEAR:
				DoCastAOE(SPELL_SPIDERMAN_FEAR);
				events.ScheduleEvent(EVENT_SPIDERMAN_FEAR, (56 + urand(0, 8)) * IN_MILLISECONDS);
				break;
			case EVENT_SPIDERMAN_HEALING_ATTACKS:
				DoCast(SPELL_HEALING_ATTACKS);
				events.ScheduleEvent(EVENT_SPIDERMAN_HEALING_ATTACKS, (24 + urand(0, 12)) * IN_MILLISECONDS);
				break;
			case EVENT_SPIDERMAN_KISS_OF_PLAGUE:
				DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1), SPELL_KISS_OF_PLAGUE);
				events.ScheduleEvent(EVENT_SPIDERMAN_KISS_OF_PLAGUE, (50 + urand(0, 18)) * IN_MILLISECONDS);
				break;
			case EVENT_NEXT_WAVE:
				wave++;
				SpiderAliveCount = 3;
				for (int i = 0; i < SpiderAliveCount; i++)
					me->SummonCreature(MINION_SPIDER_ID, SpiderPos[i], TEMPSUMMON_DEAD_DESPAWN)->Attack(me->GetVictim(), true);
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

			if (!UpdateVictim())
				return;
			
			if (HealthBelowPct(51))
				DoMeleeAttackIfReady();
		}

	private:
		uint32 SpiderAliveCount, wave;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new WorldBoss_SpiderManAI(creature);
	}
};

class WorldBoss_SpiderMan_Minion_Spider : public CreatureScript
{
public:
	WorldBoss_SpiderMan_Minion_Spider() : CreatureScript("WorldBoss_SpiderMan_Minion_Spider") { }

	struct WorldBoss_SpiderMan_Minion_SpiderAI : public ScriptedAI
	{
		WorldBoss_SpiderMan_Minion_SpiderAI(Creature* creature) : ScriptedAI(creature)
		{
			Initialize();
		}

		void Initialize()
		{
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		};

		void IsSummonedBy(Unit* owner) override 
		{
			_owner = owner;
			
			if (!_owner || _owner == nullptr)
			{
				_owner = me->FindNearestCreature(SPIDER_MAN_ID, 50.0f, true);
				
				if (!_owner || _owner == nullptr)
				{
					me->DespawnOrUnsummon(1);
					return;
				}
			}

			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);

			events.ScheduleEvent(EVENT_SPIDERMAN_MINION_SPIDER_POISEN, 12 * IN_MILLISECONDS);
		}

		void JustDied(Unit* /*killer*/) override
		{
			if (!_owner || _owner == nullptr)
			{
				_owner = me->FindNearestCreature(SPIDER_MAN_ID, 50.0f, true);

				if (!_owner || _owner == nullptr)
				{
					me->DespawnOrUnsummon(1);
					return;
				}
			}
			_owner->ToCreature()->AI()->DoAction(ACTION_SPIDER_DIED);
		}

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);

			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId)
				{
				case EVENT_SPIDERMAN_MINION_SPIDER_POISEN:
					DoCast(SPELL_SPIDERMAN_MINION_SPIDER_POISEN);
					events.ScheduleEvent(EVENT_SPIDERMAN_MINION_SPIDER_POISEN, (28 + urand(0, 4)) * IN_MILLISECONDS);
					break;
				}
			};

			if (!UpdateVictim())
				return;
			DoMeleeAttackIfReady();
		}

		Unit* _owner;
		EventMap events;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new WorldBoss_SpiderMan_Minion_SpiderAI(creature);
	};
};

class spell_world_boss_healing_attacks : public SpellScriptLoader
{
public:
	spell_world_boss_healing_attacks() : SpellScriptLoader("spell_world_boss_healing_attacks") { }

	class spell_world_boss_healing_attacksAuraScript : public AuraScript
	{
		PrepareAuraScript(spell_world_boss_healing_attacksAuraScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_HEALING_ATTACKS))
				return false;
			return true;
		};

		void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo) {
			PreventDefaultAction();

			Unit* caster = eventInfo.GetActor();

			int32 damage = eventInfo.GetDamageInfo()->GetDamage() * 4;

			Unit* ex_tar = GetCaster();

			if (damage > 0 && ex_tar != NULL && ex_tar != nullptr)
				caster->CastCustomSpell(ex_tar, SPELL_HEALING_ATTACKS + 3, &damage, NULL, NULL, true);
		};

		void Register() override
		{
			OnEffectProc += AuraEffectProcFn(spell_world_boss_healing_attacksAuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
		};
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_world_boss_healing_attacksAuraScript();
	};
};

enum SpellsDemEater {
	SPELL_BERSERK_50 = 700096,
	SPELL_DISARM,
};

enum EventsDemEater {
	EVENT_BERSERK = 1,
};

enum TextsDemEater {
	TEXT_BERSERK,
	TEXT_TENTACLES,
};

class WorldBoss_DemEater : public CreatureScript
{
public:
	WorldBoss_DemEater() : CreatureScript("WorldBoss_DemEater") { }

	struct WorldBoss_DemEaterAI : public WorldBossAI
	{
		WorldBoss_DemEaterAI(Creature* creature) : WorldBossAI(creature)
		{
			Initialize();
		}

		void Initialize()
		{
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			health_90 = health_80 = health_70 = health_60 = health_50 = health_40 = health_30 = health_20 = health_10 = false;
		};

		void JustDied(Unit* /*killer*/) override {
			_JustDied();
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.ScheduleEvent(EVENT_BERSERK, 100 * IN_MILLISECONDS);
		}

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case EVENT_BERSERK:
				Talk(TEXT_BERSERK);
				std::list<Player*> targets;
				Trinity::AnyPlayerInObjectRangeCheck u_check(me, 100.0f);
				Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, targets, u_check);
				me->VisitNearbyObject(100.0f, searcher);
				for (std::list<Player*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
					if ((*iter)->IsAlive())
						me->AddAura(SPELL_DISARM, (*iter));
				
				events.ScheduleEvent(EVENT_BERSERK, 100 * IN_MILLISECONDS);
				break;
			}
		}

		void DamageTaken(Unit* /*done_by*/, uint32& damage) override
		{
			if (HealthBelowPct(90) && !health_90) {
				_SummonTentacles(1);
				health_90 = true;
			}
			if (HealthBelowPct(80) && !health_80) {
				_SummonTentacles(1);
				health_80 = true;
			}
			if (HealthBelowPct(70) && !health_70) {
				_SummonTentacles(1);
				health_70 = true;
			}
			if (HealthBelowPct(60) && !health_60) {
				_SummonTentacles(2);
				health_60 = true;
			}
			if (HealthBelowPct(50) && !health_50) {
				_SummonTentacles(2);
				health_50 = true;
			}
			if (HealthBelowPct(40) && !health_40) {
				_SummonTentacles(2);
				health_40 = true;
			}
			if (HealthBelowPct(30) && !health_30) {
				_SummonTentacles(3);
				health_30 = true;
			}
			if (HealthBelowPct(20) && !health_20) {
				_SummonTentacles(3);
				health_20 = true;
			}
			if (HealthBelowPct(10) && !health_10) {
				_SummonTentacles(3);
				health_10 = true;
			}
		}

		void _SummonTentacles(uint8 counter) {
			Talk(TEXT_TENTACLES);
			std::list<Player*> targets;
			Trinity::AnyPlayerInObjectRangeCheck u_check(me, 100.0f);
			Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, targets, u_check);
			me->VisitNearbyObject(100.0f, searcher);

			for (uint8 i = 0; i < counter; i++)
				for (std::list<Player*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
					if ((*iter)->IsAlive())
					{
						Creature* sum = me->SummonCreature(596601, (*iter)->GetPosition(), TEMPSUMMON_DEAD_DESPAWN);
						JustSummoned(sum);
						sum->AI()->AttackStart((*iter));
						me->AddAura(700000, sum);
					}
				
		}

		bool health_90, health_80, health_70, health_60, health_50, health_40, health_30, health_20, health_10;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new WorldBoss_DemEaterAI(creature);
	}
};


enum PyromaniacEnum
{
	EVENT_SPELL_BIGFIREBOLT = 1,
	EVENT_SPELL_SCORGE,
	EVENT_FIRE_CHARGE,
	EVENT_PHASE_2,

	EVENT_SPELL_BIGFIREBOLT_2,
	EVENT_SPELL_SCORGE_2,
	EVENT_FIRE_ARMOR_PROC,
	EVENT_SPELL_FIRE_BOMB,

	EVENT_SPELL_SUN_SEED,
	EVENT_FIRE_UP,
	EVENT_SPELL_BIGFIREBOLT_3,
	EVENT_SPELL_SCORGE_3,

	SPELL_BIGFIREBOLT = 700101,
	SPELL_SCORGE,

	SPELL_FIRE_CHARGE,
	SPELL_FIRE_CHARGE_AURA,

	SPELL_FIRE_AREA_PROC,

	SPELL_FIRE_ARMOR,
	SPELL_FIRE_ARMOR_PROC,
	
	//3-€ фаза
	SPELL_SUN_SEED,
	SPELL_FIRE_UP,
	SPELL_FIRE_UP_ACTIVE,
	SPELL_FIRE_AURA,
	SPELL_FIRE_HARMS = 68161,

	SPELL_AURA_LESS_DAMAGE_2 = 710002
};

class WorldBoss_Pyromaniac : public CreatureScript
{
public:
	WorldBoss_Pyromaniac() : CreatureScript("WorldBoss_Pyromaniac") { }

	struct WorldBoss_PyromaniacAI : public WorldBossAI
	{
		WorldBoss_PyromaniacAI(Creature* creature) : WorldBossAI(creature)
		{
			Initialize();
		}

		void Initialize()
		{
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE_2))
				me->AddAura(SPELL_AURA_LESS_DAMAGE_2, me);
			me->DeMorph();
			me->RemoveAura(SPELL_FIRE_ARMOR);
			me->RemoveAura(SPELL_FIRE_AURA);
			me->RemoveAura(SPELL_FIRE_HARMS);
			DoCast(52993);
			events.SetPhase(0);
		};
		
		void JustReachedHome() override {
			DoCast(52993);
			me->DeMorph();
		}

		void JustDied(Unit* /*killer*/) override {
			_JustDied();
			me->DeMorph();
		}

		void EnterCombat(Unit* /*who*/) override
		{
			me->CastStop();
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE_2))
				me->AddAura(SPELL_AURA_LESS_DAMAGE_2, me);
			events.SetPhase(1);
			events.ScheduleEvent(EVENT_SPELL_BIGFIREBOLT, urand(7000, 10000), 0, 1);
			events.ScheduleEvent(EVENT_SPELL_SCORGE, urand(5500, 7000), 0, 1);
			events.ScheduleEvent(EVENT_FIRE_CHARGE, 18000, 0, 1);
			events.ScheduleEvent(EVENT_PHASE_2, 200000, 0, 1);
		}

		void SetPhase(uint32 phaseId)
		{
			events.SetPhase(phaseId);
			switch (phaseId)
			{
			case 2:
				events.CancelEvent(EVENT_SPELL_BIGFIREBOLT);
				events.CancelEvent(EVENT_SPELL_SCORGE);
				events.CancelEvent(EVENT_FIRE_CHARGE);
				events.CancelEvent(EVENT_PHASE_2);

				events.ScheduleEvent(EVENT_SPELL_BIGFIREBOLT_2, urand(5000, 8000), 0, 2);
				events.ScheduleEvent(EVENT_SPELL_SCORGE_2, urand(3500, 5000), 0, 2);
				events.ScheduleEvent(EVENT_FIRE_ARMOR_PROC, 3000, 0, 2);
				phase_2_health = me->GetHealthPct();
				
				DoCast(SPELL_FIRE_ARMOR); //Visual
				break;
			case 3:
				me->SetDisplayId(1204); //Ёлементаль ќгн€

				DoCast(SPELL_FIRE_AURA);
				DoCast(SPELL_FIRE_HARMS);

				events.CancelEvent(EVENT_SPELL_BIGFIREBOLT_2);
				events.CancelEvent(EVENT_SPELL_SCORGE_2);
				
				events.RescheduleEvent(EVENT_FIRE_ARMOR_PROC, 3000, 0, 3);

				events.ScheduleEvent(EVENT_SPELL_SUN_SEED, urand(8000, 12000), 0, 3);
				events.ScheduleEvent(EVENT_FIRE_UP, urand(0, 1000), 0, 3);
				events.ScheduleEvent(EVENT_SPELL_BIGFIREBOLT_3, urand(3000, 6000), 0, 3);
				events.ScheduleEvent(EVENT_SPELL_SCORGE_3, urand(3500, 4500), 0, 3);

				break;
			}
		}

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case EVENT_SPELL_BIGFIREBOLT:
				DoCast(SPELL_BIGFIREBOLT);
				events.ScheduleEvent(EVENT_SPELL_BIGFIREBOLT, urand(7000, 10000), 0, 1);
				break;
			case EVENT_SPELL_SCORGE:
				DoCast(SPELL_SCORGE);
				events.ScheduleEvent(EVENT_SPELL_SCORGE, urand(5500, 7000), 0, 1);
				break;
			case EVENT_FIRE_CHARGE:
				DoCast(SPELL_FIRE_CHARGE);
				me->AddAura(SPELL_FIRE_CHARGE_AURA, me);
				if (me->GetAuraCount(SPELL_FIRE_CHARGE_AURA) > 9)
					SetPhase(2);
				events.ScheduleEvent(EVENT_FIRE_CHARGE, 18000, 0, 1);
				break;
			case EVENT_PHASE_2:
				SetPhase(2);
				break;
			case EVENT_SPELL_BIGFIREBOLT_2:
			{
				//Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
				DoCastVictim(SPELL_BIGFIREBOLT);
				DoCastVictim(SPELL_FIRE_AREA_PROC, true);
				events.ScheduleEvent(EVENT_SPELL_BIGFIREBOLT_2, urand(6000, 9000), 0, 2);
				break;
			}
			case EVENT_SPELL_SCORGE_2:
			{	
				//Unit* target = 
				//Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
				DoCastVictim(SPELL_SCORGE);
				DoCastVictim(SPELL_FIRE_AREA_PROC, true);
				events.ScheduleEvent(EVENT_SPELL_SCORGE_2, urand(4500, 6000), 0, 2);
				break;
			}
			case EVENT_FIRE_ARMOR_PROC:
				{
					std::list<Player*> targets;
					Trinity::AnyPlayerInObjectRangeCheck u_check(me, 100.0f);
					Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, targets, u_check);
					me->VisitNearbyObject(60.0f, searcher);
					for (std::list<Player*>::iterator iter = targets.begin(); iter != targets.end(); ++iter)
						if ((*iter)->IsAlive() && !(*iter)->IsGameMaster())
						{
							float value = me->GetDistance((*iter)->GetPosition());
							uint32 st_damage;

							if (events.IsInPhase(2))
								st_damage  = 15000000;
							else st_damage = 20000000;

							value = value > 50 ? 50 : value;
							float percent = (value / 50);
							percent = percent < 0.3f ? 0.3f : percent;
							
							const int32 _value = st_damage * percent;
							//const int32 _value = value > 50 ? 0 :
							//	value < 10 ? st_damage :
							//	st_damage / 40 * (50 - value);
							me->CastCustomSpell((*iter), (uint32)SPELL_FIRE_ARMOR_PROC, &_value, NULL, NULL, true);
						}
					if (events.IsInPhase(2))
						events.ScheduleEvent(EVENT_FIRE_ARMOR_PROC, 3000, 0, 2);
					else 
						events.ScheduleEvent(EVENT_FIRE_ARMOR_PROC, 3000, 0, 3);
				}
				break;

			case EVENT_SPELL_BIGFIREBOLT_3:
			{
				//Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
				DoCastVictim(SPELL_BIGFIREBOLT);
				DoCastVictim(SPELL_FIRE_AREA_PROC, true);
				events.ScheduleEvent(EVENT_SPELL_BIGFIREBOLT_3, urand(3000, 6000), 0, 3);
				break;
			}
			case EVENT_SPELL_SCORGE_3:
			{
				//Unit* target = SelectTarget(SELECT_TARGET_TOPAGGRO, 0);
				DoCastVictim(SPELL_SCORGE);
				DoCastVictim(SPELL_FIRE_AREA_PROC, true);
				events.ScheduleEvent(EVENT_SPELL_SCORGE_3, urand(1500, 5000), 0, 3);
				break;
			}
			case EVENT_SPELL_SUN_SEED:
			{
				//Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0);
				DoCastVictim(SPELL_SUN_SEED);
				events.ScheduleEvent(EVENT_SPELL_SUN_SEED, urand(6000, 12000), 0, 3);
				break;
			}
			case EVENT_FIRE_UP:
				DoCast(SPELL_FIRE_UP);
				events.ScheduleEvent(EVENT_FIRE_UP, urand(14000, 17000), 0, 3);
				break;
			}
		}

		void DamageTaken(Unit* /*done_by*/, uint32& damage) override
		{
			if (events.IsInPhase(1) && HealthBelowPct(60))
				SetPhase(2);
			if (events.IsInPhase(2) && HealthBelowPct(phase_2_health / 2))
				SetPhase(3);
		}

		float phase_2_health;

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new WorldBoss_PyromaniacAI(creature);
	}
};

void AddSC_Custom_World_Bosses()
{
	new WorldBoss_Shtorm();
	new WorldBoss_SpiderMan();
	new WorldBoss_SpiderMan_Minion_Spider();
	new spell_world_boss_healing_attacks();
	new WorldBoss_DemEater();
	new WorldBoss_Pyromaniac();
}