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

	SPELL_AURA_ZIBUN_CONTOR = 700085,//Аура, за которую нельзя заступать
	SPELL_AURA_ZIBUN_CONTOR_VISUAL = 700088,
	SPELL_AURA_ZIBUN_CONTOR_KILLER = 700081, 

	SPELL_AURA_ZIBUN_SLIMES = 700082,//Аура зыбуна
	SPELL_WATER_NOVA,		//Отталкивающая нова
	SPELL_POISON			//Плевок яда
};

enum Phases {
	PHASE_ONE = 1,
	PHASE_TWO,
};

enum Events
{
	EVENT_SPELL_AURA_ZIBUN_SLIMES = 1,
	EVENT_SPELL_WATER_NOVA,
	EVENT_SPELL_POISON,
};

enum NPCS
{
	NPC_SLIME = 553605,
	DUMMY_CLOUD = 161616,
};

class boss_zibun : public CreatureScript
{
public:
	boss_zibun() : CreatureScript("boss_zibun") { }

	struct boss_zibunAI : public BossAI
	{
		boss_zibunAI(Creature* creature) : BossAI(creature, DATA_ZIBUN)
		{
			Initialize();
		}

		void Initialize()
		{
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			cloud = me->FindNearestCreature(DUMMY_CLOUD, 100);
		}

		void Reset() override
		{
			Initialize();
			_Reset();
			KillAllSlimes();;
			if (cloud)
			{
				cloud->RemoveAura(SPELL_AURA_ZIBUN_CONTOR);
				cloud->RemoveAura(SPELL_AURA_ZIBUN_CONTOR_VISUAL);
			}
		}

		void JustDied(Unit* killer) override
		{
			_JustDied();
			KillAllSlimes();
			if (cloud)
			{
				cloud->RemoveAura(SPELL_AURA_ZIBUN_CONTOR);
				cloud->RemoveAura(SPELL_AURA_ZIBUN_CONTOR_VISUAL);
			}
			instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_AURA_ZIBUN_CONTOR);
			instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_AURA_ZIBUN_CONTOR_KILLER);
		}

		void CallSlimes()
		{
			std::list<Player*> targets;
			Trinity::AnyPlayerInObjectRangeCheck u_check(me, 100.0f);
			Trinity::PlayerListSearcher<Trinity::AnyPlayerInObjectRangeCheck> searcher(me, targets, u_check);
			me->VisitNearbyObject(100.0f, searcher);

			for (std::list<Player*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
				me->AddAura(SPELL_AURA_ZIBUN_SLIMES, (*iter));
		}

		void KillAllSlimes()
		{
			std::list<Unit*> targets;
			Trinity::AnyUnitInObjectRangeCheck u_check(me, 100.0f);
			Trinity::UnitListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(me, targets, u_check);
			me->VisitNearbyObject(100.0f, searcher);

			for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
				if (!(*iter)->IsCharmedOwnedByPlayerOrPlayer() && (*iter)->ToCreature()->GetEntry() == NPC_SLIME)
					me->Kill(*iter);
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.SetPhase(PHASE_ONE);
			events.ScheduleEvent(EVENT_SPELL_AURA_ZIBUN_SLIMES, urand(4000, 23000));
			events.ScheduleEvent(EVENT_SPELL_WATER_NOVA, urand(13000, 24000));
			cloud = me->FindNearestCreature(DUMMY_CLOUD, 100);
			me->AddAura(SPELL_AURA_ZIBUN_CONTOR, cloud);
			me->AddAura(SPELL_AURA_ZIBUN_CONTOR_VISUAL, cloud);
		}

		void ExecuteEvent(uint32 eventid) override
		{
			switch (eventid)
			{
			case EVENT_SPELL_AURA_ZIBUN_SLIMES:
				CallSlimes();
				events.ScheduleEvent(EVENT_SPELL_AURA_ZIBUN_SLIMES, urand(56000, 62000));
				break;
			case EVENT_SPELL_WATER_NOVA:
				DoCast(SPELL_WATER_NOVA);
				events.ScheduleEvent(EVENT_SPELL_WATER_NOVA, urand(27000, 33000));
				break;
			case EVENT_SPELL_POISON:
				DoCast(SPELL_POISON);
				events.ScheduleEvent(EVENT_SPELL_POISON, urand(12000, 17000));
				break;
			}
		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage) override
		{
			if (HealthBelowPct(31) && events.IsInPhase(PHASE_ONE))
			{
				events.SetPhase(PHASE_TWO);
				events.RescheduleEvent(EVENT_SPELL_AURA_ZIBUN_SLIMES, urand(19000, 21000));
				events.CancelEvent(EVENT_SPELL_WATER_NOVA);
				events.ScheduleEvent(EVENT_SPELL_POISON, urand(10000, 30000));
			}
		}
		
		Creature* cloud;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetInstanceAI<boss_zibunAI>(creature);
	}
};

class spell_boss_zibun_aura_slimes : public SpellScriptLoader
{
public:
	spell_boss_zibun_aura_slimes() : SpellScriptLoader("spell_boss_zibun_aura_slimes") { }

	class spell_boss_zibun_aura_slimes_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_boss_zibun_aura_slimes_AuraScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_AURA_ZIBUN_SLIMES))
				return false;
			return true;
		}

		void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
		{
			GetCaster()->SummonCreature(NPC_SLIME, GetTarget()->GetPosition())->Attack(GetTarget(), true);
			GetCaster()->SummonCreature(NPC_SLIME, GetTarget()->GetPosition())->Attack(GetTarget(), true);
			GetCaster()->SummonCreature(NPC_SLIME, GetTarget()->GetPosition())->Attack(GetTarget(), true);
		}

		void Register() override
		{
			AfterEffectRemove += AuraEffectRemoveFn(spell_boss_zibun_aura_slimes_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_boss_zibun_aura_slimes_AuraScript();
	}
};

class spell_boss_zibun_aura_contor : public SpellScriptLoader
{
public:
	spell_boss_zibun_aura_contor() : SpellScriptLoader("spell_boss_zibun_aura_contor") { }

	class spell_boss_zibun_aura_contor_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_boss_zibun_aura_contor_AuraScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_AURA_ZIBUN_CONTOR))
				return false;
			return true;
		}

		void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
		{
			GetCaster()->AddAura(SPELL_AURA_ZIBUN_CONTOR_KILLER, GetTarget());
		}

		void Register() override
		{
			AfterEffectRemove += AuraEffectRemoveFn(spell_boss_zibun_aura_contor_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_boss_zibun_aura_contor_AuraScript();
	}
};

void AddSC_boss_zibun()
{
	new boss_zibun();
	new spell_boss_zibun_aura_contor();
	new spell_boss_zibun_aura_slimes();
}
