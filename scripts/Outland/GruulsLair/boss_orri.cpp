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

enum Spells {
	SPELL_AURA_LESS_DAMAGE = 700000,
	SPELL_LOSS_ARMOR,
	SPELL_DEATH_PLAGUE_AURA,
	SPELL_DEATH_PLAGUE_DAMAGE,
	SPELL_METEOR,
	SPELL_SILENCE,
};

enum Phases {
	PHASE_START = 1,
	PHASE_50,

};

enum OrriEvents {
	EVENT_SPELL_LOSS_ARMOR = 1,
	EVENT_SPELL_DEATH_PLAGUE,
	EVENT_SPELL_METEOR,
	EVENT_SPELL_SILENCE,
	EVENT_TEXT_METEOR
};

enum OrriTexts {
	TEXT_DEATH_PLAGUE = 0,
	TEXT_METEOR
};

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

class boss_orri : public CreatureScript
{
public:
	boss_orri() : CreatureScript("boss_orri") { }

	struct boss_orriAI : public BossAI
	{
		boss_orriAI(Creature* creature) : BossAI(creature, DATA_MAULGAR)
		{
			Initialize();
			instance = creature->GetInstanceScript();
		}

		void Initialize()
		{
			phase = PHASE_START;
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.ScheduleEvent(EVENT_SPELL_LOSS_ARMOR, 7 * IN_MILLISECONDS);
		}

		void JustDied(Unit* /*killer*/) override
		{
			_JustDied();
			instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_DEATH_PLAGUE_AURA);
		}

		void Reset() override
		{
			Initialize();
			_Reset();
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

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case EVENT_SPELL_LOSS_ARMOR:
				DoCastVictim(SPELL_LOSS_ARMOR);
				events.ScheduleEvent(EVENT_SPELL_LOSS_ARMOR, 30 * IN_MILLISECONDS);
				break;
			case EVENT_SPELL_DEATH_PLAGUE:
				Talk(TEXT_DEATH_PLAGUE);
				DoCast(GetRandomTarget(), SPELL_DEATH_PLAGUE_AURA);
				events.ScheduleEvent(EVENT_SPELL_DEATH_PLAGUE, 60 * IN_MILLISECONDS);
				break;
			case EVENT_SPELL_METEOR:
				events.ScheduleEvent(EVENT_TEXT_METEOR, 2 * IN_MILLISECONDS);
				DoCastAOE(SPELL_METEOR);
				events.ScheduleEvent(EVENT_SPELL_METEOR, 2 * 60 * IN_MILLISECONDS);
				break;
			case EVENT_TEXT_METEOR:
				Talk(TEXT_METEOR);
				break;
			case EVENT_SPELL_SILENCE:
				if (Unit* target = GetHealerOrRandomTarget())
				{
					DoCast(target, SPELL_SILENCE);
					events.ScheduleEvent(EVENT_SPELL_SILENCE, 30 * IN_MILLISECONDS);
				}
				break;
			}
		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage) override
		{
			if (HealthBelowPct(50) && phase == PHASE_START)
			{
				phase = PHASE_50;
				events.ScheduleEvent(EVENT_SPELL_DEATH_PLAGUE, 60 * IN_MILLISECONDS);
				events.ScheduleEvent(EVENT_SPELL_METEOR, 2 * 60 * IN_MILLISECONDS);
				events.ScheduleEvent(EVENT_SPELL_SILENCE, 30 * IN_MILLISECONDS);
			}
		}

	//Переменные
	private:
		uint8 phase;
		InstanceScript* instance;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetGruulsLairAI<boss_orriAI>(creature);
	};
};

class spell_orri_death_plague : public SpellScriptLoader
{
public:
	spell_orri_death_plague() : SpellScriptLoader("spell_orri_death_plague") { }

	class spell_orri_death_plague_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_orri_death_plague_AuraScript);

		void RemoveEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
		{
			if (GetCaster() && GetTarget() && GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
				GetCaster()->CastSpell(GetTarget(), SPELL_DEATH_PLAGUE_DAMAGE, TRIGGERED_FULL_MASK);
			GetTarget()->RemoveAura(SPELL_DEATH_PLAGUE_AURA);
		}

		void Register() override
		{
			OnEffectRemove += AuraEffectRemoveFn(spell_orri_death_plague_AuraScript::RemoveEffect, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_orri_death_plague_AuraScript();
	}
};

void AddSC_orri_spells_scripts()
{
	new spell_orri_death_plague();
}

void AddSC_boss_orri()
{
	new boss_orri();
}