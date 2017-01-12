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
SDName: boss_hurd entry 68001
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
	SPELL_AURA_HURD_DOT	   = 700036,
	SPELL_HURD_DOT_FIELD
};

enum Phases 
{
	PHASE_ONE = 1,
	PHASE_TWO,
};

enum HurdEventsAndAction 
{
	EVENT_CAST_SPELL_HURD_DOT_FIELD = 1,
};

class boss_hurd : public CreatureScript
{
public:
	boss_hurd() : CreatureScript("boss_hurd") { }

	struct boss_hurdAI : public BossAI
	{
		boss_hurdAI(Creature* creature) : BossAI(creature, DATA_HURD)
		{
			Initialize();
			instance = creature->GetInstanceScript();
		}

		void Initialize()
		{
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			me->AddAura(SPELL_AURA_HURD_DOT, me);
		}

		void JustDied(Unit* /*killer*/) override
		{
			_JustDied();
			me->RemoveAura(SPELL_AURA_HURD_DOT);
			instance->DoRemoveAurasDueToSpellOnPlayers(SPELL_AURA_HURD_DOT);
		}

		void Reset() override
		{
			Initialize();
			_Reset();
		}

		void ExecuteEvent(uint32 eventId) override
		{
			switch (eventId)
			{
			case EVENT_CAST_SPELL_HURD_DOT_FIELD:
				DoCast(SelectTarget(SELECT_TARGET_RANDOM, 0), SPELL_HURD_DOT_FIELD);
				events.ScheduleEvent(EVENT_CAST_SPELL_HURD_DOT_FIELD, urand(19000, 22000));
			}
		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage) override
		{
			if (HealthBelowPct(51) && !events.IsInPhase(PHASE_TWO))
			{
				events.SetPhase(PHASE_TWO);
				events.ScheduleEvent(EVENT_CAST_SPELL_HURD_DOT_FIELD, urand(3000, 7000));
			}
		}

		//Переменные
	private:
		InstanceScript* instance;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetPitOfSaronAI<boss_hurdAI>(creature);
	};
};

void AddSC_boss_hurd()
{
	new boss_hurd();
}