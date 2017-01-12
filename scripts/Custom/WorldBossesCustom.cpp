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
SDComment: Реализован вестник бури.
SDCategory: World Bosses
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Containers.h"
#include "SpellHistory.h"

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

void AddSC_Custom_World_Bosses()
{
	new WorldBoss_Shtorm();
	new WorldBoss_SpiderMan();
	new WorldBoss_SpiderMan_Minion_Spider();
	new spell_world_boss_healing_attacks();
}