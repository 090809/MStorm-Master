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

	SPELL_SPIKE = 700086,
	SPELL_BERSERK,
};

enum Phases {
	PHASE_ONE = 1,
	PHASE_TWO,
};

struct HittedBy_Time
{
	Unit* hitted_by;
	int16 cooldown;
};

class boss_kraken : public CreatureScript
{
public:
	boss_kraken() : CreatureScript("boss_kraken") { }

	struct boss_krakenAI : public BossAI
	{
		boss_krakenAI(Creature* creature) : BossAI(creature, DATA_KRAKEN)
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
			history.clear();
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
		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage) override
		{
			if (me->HealthBelowPct(21) && events.IsInPhase(PHASE_ONE))
			{
				events.SetPhase(PHASE_TWO);
				DoCast(SPELL_BERSERK);
			}
		}

		void SpellHit(Unit* caster, SpellInfo const* /*spell*/) override
		{
			int8 iter = -1;
			for (uint8 i = 0; i < history.size(); i++)
				if (history[i].hitted_by == caster)
				{
					iter = i;
					break;
				}

			if (iter != -1) {
				if (history[iter].cooldown <= 0 && urand(0, 99) < 30)
				{
					DoCast(caster, SPELL_SPIKE);
					history[iter].cooldown = 5000;
				}
			} else {
				HittedBy_Time attr;

				if (urand(0, 99) < 30)
				{
					DoCast(caster, SPELL_SPIKE);
					attr.cooldown = 5000;
				} else attr.cooldown = 0;

				attr.hitted_by = caster;

				history.push_back(attr);
			}
		}

		void UpdateAI(uint32 diff) override
		{
			if (!UpdateVictim())
				return;

			for (uint8 i = 0; i < history.size(); i++)
				history[i].cooldown -= diff;

			if (me->HasUnitState(UNIT_STATE_CASTING))
				return;

			DoMeleeAttackIfReady();
		}

	private:
		std::vector<HittedBy_Time> history;

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetInstanceAI<boss_krakenAI>(creature);
	}
};

void AddSC_boss_kraken()
{
	new boss_kraken();
}