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

#include "MoveSplineInit.h"

enum Spells
{
	SPELL_AURA_LESS_DAMAGE = 700000,

	SPELL_IMPALED = 700078,
	SPELL_POISON ,
	
	SPELL_AURA_DEMONIC = 60449,
	SPELL_BERSERK = 700077,
	SPELL_TORTURE = 700076,

	SPELL_RIDE_VEHICLE = 46598,

};

enum Phases {
	PHASE_ONE = 1,
	PHASE_TWO,
	PHASE_THREE
};

enum Events
{
	EVENT_IMPALE = 1,
	EVENT_TORTURE
};

enum NPCS
{
	NPC_IMPALE = 161618,
};

class boss_asgur : public CreatureScript
{
public:
	boss_asgur() : CreatureScript("boss_asgur") { }

	struct boss_asgurAI : public BossAI
	{
		boss_asgurAI(Creature* creature) : BossAI(creature, DATA_ASGUR)
		{
			Initialize();
		}

		void Initialize()
		{
			me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			me->AddAura(SPELL_POISON, me);
		}

		void Reset() override
		{
			Initialize();
			_Reset();
		}

		void JustDied(Unit* killer) override
		{
			_JustDied();
			if (instance->instance->GetPlayersCountExceptGMs() >= 10)
			{
				Map::PlayerList const& Players = me->GetMap()->GetPlayers();
				for (Map::PlayerList::const_iterator itr = Players.begin(); itr != Players.end(); ++itr)
					if (Player* player = itr->GetSource())
						player->AddItem(70003, 1);
			}
			GiveRewardPoints();
		}

		void EnterCombat(Unit* /*who*/) override
		{
			_EnterCombat();
			if (!me->HasAura(SPELL_AURA_LESS_DAMAGE))
				me->AddAura(SPELL_AURA_LESS_DAMAGE, me);
			events.SetPhase(PHASE_ONE);
			events.ScheduleEvent(EVENT_IMPALE, 60000);
			me->AddAura(SPELL_POISON, me);
		}

		void SummonedCreatureDespawn(Creature* creature) override
		{
			if (creature == impale_unit)
				impale_target->RemoveAura(SPELL_IMPALED);
		}

		void ExecuteEvent(uint32 eventid) override
		{
			switch (eventid)
			{
			case EVENT_IMPALE:
				impale_target = SelectTarget(SELECT_TARGET_RANDOM, 1);
				if (impale_target)
					impale_unit = impale_target->SummonCreature(NPC_IMPALE, impale_target->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 60000)->ToCreature();
				events.ScheduleEvent(EVENT_IMPALE, 60000);
				break;
			case EVENT_TORTURE:
				DoCast(SPELL_TORTURE);
				events.ScheduleEvent(EVENT_TORTURE, urand(28000, 31000));
				break;
			}
		}

		void DamageTaken(Unit* /*attacker*/, uint32& damage) override
		{
			if (me->HealthBelowPct(71) && events.IsInPhase(PHASE_ONE))
			{
				me->AddAura(SPELL_AURA_DEMONIC, me);
				events.ScheduleEvent(EVENT_TORTURE, urand(18000, 25000));
				events.SetPhase(PHASE_TWO);
			} else if (me->HealthBelowPct(11) && events.IsInPhase(PHASE_TWO))
			{
				me->RemoveAura(SPELL_AURA_DEMONIC);
				events.SetPhase(PHASE_THREE);
				DoCast(SPELL_BERSERK);
			}
		}

		Unit* impale_target;
		Creature *impale_unit;
	};



	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetInstanceAI<boss_asgurAI>(creature);
	}
};

class npc_spike : public CreatureScript
{
public:
	npc_spike() : CreatureScript("npc_spike") { }

	struct npc_spikeAI : public ScriptedAI
	{
		npc_spikeAI(Creature* creature) : ScriptedAI(creature), _hasTrappedUnit(false)
		{
			ASSERT(creature->GetVehicleKit());

			SetCombatMovement(false);
		}

		void JustDied(Unit* /*killer*/) override
		{
			if (TempSummon* summ = me->ToTempSummon())
				if (Unit* trapped = summ->GetSummoner())
					trapped->RemoveAurasDueToSpell(SPELL_IMPALED);

			me->DespawnOrUnsummon();
		}

		void KilledUnit(Unit* victim) override
		{
			me->DespawnOrUnsummon();
			victim->RemoveAurasDueToSpell(SPELL_IMPALED);
		}

		void IsSummonedBy(Unit* summoner) override
		{
			me->AddAura(SPELL_IMPALED, summoner);
			summoner->CastSpell(me, SPELL_RIDE_VEHICLE, true);
			_hasTrappedUnit = true;
		}

		void PassengerBoarded(Unit* passenger, int8 /*seat*/, bool apply) override
		{
			if (!apply)
				return;

			/// @HACK - Change passenger offset to the one taken directly from sniffs
			/// Remove this when proper calculations are implemented.
			/// This fixes healing spiked people
			Movement::MoveSplineInit init(passenger);
			init.DisableTransportPathTransformations();
			init.MoveTo(-0.02206125f, -0.02132235f, 5.514783f, false);
			init.Launch();
		}

		void UpdateAI(uint32 diff) override
		{
			if (!_hasTrappedUnit)
				return;
		}

	private:
		EventMap _events;
		bool _hasTrappedUnit;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return GetInstanceAI<npc_spikeAI>(creature);
	}
};



void AddSC_boss_asgur()
{
	new npc_spike();
	new boss_asgur();
}