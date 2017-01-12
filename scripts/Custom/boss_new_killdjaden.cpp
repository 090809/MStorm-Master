#include "GroupMgr.h"
#include "ScriptMgr.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GameEventMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Unit.h"
#include "GameObject.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "InstanceScript.h"
#include "CombatAI.h"
#include "PassiveAI.h"
#include "Chat.h"
#include "DBCStructure.h"
#include "DBCStores.h"
#include "ObjectMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum SPELLS {
	SPELL_START = 49119,
	SPELL_METIOR = 26789,
	SPELL_HELL_BALL = 66965,
	SPELL_SHADOW_FIRE_BALT = 41078,

	SPELL_BLOOD_BOLT = 71478,
	SPELL_BLOOD_HEAL = 41068,
	SPELL_AURA_VISUAL = 71596,

	SPELL_DARK_HEAL = 72324,
};

enum EVENTS {
	EVENT_FIRE_COMBO = 0,
	EVENT_HELL_BALL,
	EVENT_METIOR,
	EVENT_START_FIRES,
	EVENT_CHANGE_STATE,
	EVENT_SUMMON_BLOOD,
	EVENT_SHADOW_FIRE_BALT,

	EVENT_START_FIRES_DEATH,
	EVENT_START_FIRES_SAVE,
	EVENT_CHECK_TO_KILL,

	EVENT_DEAL_MELEE_DAMAGE,
	EVENT_CAST_BLOOD_BOLT,
	EVENT_CAST_BLOOD_HEAL,
};

enum ACTIONS {
	ACTION_FIRE_DEATH = 0,
	ACTION_FIRE_SAVE,
	ACTION_RESET
};

class KilDjaden : public CreatureScript
{
public:
	KilDjaden() : CreatureScript("boss_new_KilDjaden") { }

	struct KilDjadenAI : public BossAI //npc 418000
	{
		KilDjadenAI(Creature* creature) : BossAI(creature, 1)
		{
			init();
		}

		void init() {
			me->SetMaxHealth(516326741);
			me->SetHealth(me->GetMaxHealth());
			me->setFaction(14);
			me->SetSpeed(MOVE_WALK, 0);
			me->SetSpeed(MOVE_RUN, 0);
			DoCast(me, 420000, true);
			phase = 1;
			state = 1;
		}

		void Reset() override {
			_Reset();
			init();
			me->GetCreatureListWithEntryInGrid(Fires, 418001, 50.0f);
			for (std::list<Creature*>::const_iterator iter = Fires.begin(); iter != Fires.end(); ++iter)
			{
				(*iter)->RemoveAura(74803);
				(*iter)->RemoveAura(74629);
				(*iter)->AI()->DoAction(ACTION_RESET);
			}
		
			me->GetCreatureListWithEntryInGrid(Blood, 418002, 50.0f);
			for (std::list<Creature*>::const_iterator iter = Blood.begin(); iter != Blood.end(); ++iter)
				if ((*iter) && (*iter)->IsAlive())
					(*iter)->DealDamage((*iter), (*iter)->GetMaxHealth() * 2);
		}

		void EnterCombat(Unit* /*who*/) override {
			_EnterCombat();
			events.ScheduleEvent(EVENT_HELL_BALL, urand(7000, 7000));
			events.ScheduleEvent(EVENT_METIOR, urand(1500, 4500));
			SwitchFires(phase);
			me->GetCreatureListWithEntryInGrid(Fires, 418001, 50.0f);
			for (std::list<Creature*>::const_iterator iter = Fires.begin(); iter != Fires.end(); ++iter)
				(*iter)->AddAura(74803, (*iter));
		}

		void JustDied(Unit* /*killer*/) override
		{
			_JustDied();
			me->GetCreatureListWithEntryInGrid(Fires, 418001, 50.0f);
			for (std::list<Creature*>::const_iterator iter = Fires.begin(); iter != Fires.end(); ++iter)
			{
				(*iter)->RemoveAura(74803);
				(*iter)->RemoveAura(74629);
				(*iter)->AI()->DoAction(ACTION_RESET);
			}

			me->GetCreatureListWithEntryInGrid(Blood, 418002, 50.0f);
			for (std::list<Creature*>::const_iterator iter = Blood.begin(); iter != Blood.end(); ++iter)
				if ((*iter) && (*iter)->IsAlive())
					(*iter)->DespawnOrUnsummon();
		}

		void FF(uint8 phase) {
			switch (phase) {
			case 1:
				fire = me->FindNearestCreature(418001, 50.0f);
				fire->AddAura(74629, fire);
				fire->AI()->DoAction(ACTION_FIRE_SAVE);
				break;
			case 2:
			case 3:
				me->GetCreatureListWithEntryInGrid(Fires, 418001, 50.0f);
				int i = urand(0, 2);
				int j = 0;
				for (std::list<Creature*>::const_iterator iter = Fires.begin(); iter != Fires.end(); ++iter)
				{
					if (j == i)
					{
						(*iter)->AddAura(74629, (*iter));
						(*iter)->AI()->DoAction(ACTION_FIRE_SAVE);
						break;
					}
					j++;
				}
				break;
			}
		}

		void SwitchFires(uint8 phase) {
			switch (state) {
			case 1: //Тьма горит, заходить нельзя
				events.ScheduleEvent(EVENT_START_FIRES, 20000);
				break;
			case 2: //Тьма тухнет, загорается свет, 6 секунд
				me->GetCreatureListWithEntryInGrid(Fires, 418001, 50.0f);
				for (std::list<Creature*>::const_iterator iter = Fires.begin(); iter != Fires.end(); ++iter)
				{
					(*iter)->RemoveAura(74803);
				}
				FF(phase);
				events.ScheduleEvent(EVENT_START_FIRES, 10000);
				break;
			case 3: //Свет горит, выходить нельзя
				events.ScheduleEvent(EVENT_START_FIRES, 20000);
				break;
			case 4: //Свет тухнет, загорается тьма, 6 секунд
				me->GetCreatureListWithEntryInGrid(Fires, 418001, 50.0f);
				for (std::list<Creature*>::const_iterator iter = Fires.begin(); iter != Fires.end(); ++iter)
				{
					(*iter)->RemoveAura(74629);
					(*iter)->AddAura(74803, (*iter));
					(*iter)->AI()->DoAction(ACTION_FIRE_DEATH);
				}
				events.ScheduleEvent(EVENT_START_FIRES, 6000);
				break;
			}
		}

		void DamageTaken(Unit* /*done_by*/, uint32& /*damage*/) override
		{
			if (phase < 2 && me->GetHealthPct() < 60) {
				phase = 2;
				events.ScheduleEvent(EVENT_SHADOW_FIRE_BALT, 500);
			}
			else if (phase < 3 && me->GetHealthPct() < 20) {
				phase = 3;
				events.ScheduleEvent(EVENT_SUMMON_BLOOD, 500);
			}
		}

		void UpdateAI(uint32 diff) override
		{
			if (!UpdateVictim())
				return;
			events.Update(diff);
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_HELL_BALL:
					DoCast(me->GetVictim(), SPELL_HELL_BALL, true);
					events.ScheduleEvent(EVENT_HELL_BALL, urand(3000, 7000));
					break;
				case EVENT_METIOR:
					DoCast(SPELL_METIOR);
					events.ScheduleEvent(EVENT_METIOR, urand(1500, 4500));
					break;
				case EVENT_START_FIRES:
					state++;
					if (state > 4) state = 1;
					SwitchFires(phase);
					break;
				case EVENT_SUMMON_BLOOD:
					me->SummonCreature(418002, me->GetVictim()->GetPosition())->CastSpell(me->GetVictim(), 63620); //Телепорт за спину после спауна.
					events.ScheduleEvent(EVENT_SUMMON_BLOOD, 24000);
					break;
				case EVENT_SHADOW_FIRE_BALT:
					DoCast(me->GetVictim(), SPELL_SHADOW_FIRE_BALT, true);
					events.ScheduleEvent(EVENT_SHADOW_FIRE_BALT, urand(5000, 9000));
					break;
				}
			}
			DoMeleeAttackIfReady();
		}

	public:
		Creature* fire;
		uint8 phase, state;
		std::list<Creature*> Fires, Blood;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new KilDjadenAI(creature);
	}
};


class npc_fire_kildjaden : public CreatureScript
{
public:
	npc_fire_kildjaden() : CreatureScript("npc_fire_kildjaden") { }

	struct npc_fire_kildjadenAI : public ScriptedAI //npc 418001
	{
		npc_fire_kildjadenAI(Creature* creature) : ScriptedAI(creature)
		{
			init();
		}

		void DoAction(int32 action) override {
			switch (action)
			{
			case ACTION_FIRE_DEATH:
				phase = 3;
				events.ScheduleEvent(EVENT_START_FIRES_DEATH, 6000);
				break;
			case ACTION_FIRE_SAVE:
				phase = 3;
				events.ScheduleEvent(EVENT_START_FIRES_SAVE, 10000);
				break;
			case ACTION_RESET:
				phase = 0;
				events.Reset();
				break;
			}
		}

		void init() {
			me->SetMaxHealth(1000000);
			me->SetHealth(1000000);
			phase = 0;
			events.ScheduleEvent(EVENT_CHECK_TO_KILL, 500);
		}

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);
			me->GetPlayerListInGrid(PlayersList, 150.0f);
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_START_FIRES_DEATH:
					DoCast(48582); //Визуальный эффект перехода
					phase = 1;
					events.RescheduleEvent(EVENT_CHECK_TO_KILL, 500);
					break;
				case EVENT_START_FIRES_SAVE:
					DoCast(33002); //Визуальный эффект перехода
					phase = 2;
					events.RescheduleEvent(EVENT_CHECK_TO_KILL, 500);
					break;
				case EVENT_CHECK_TO_KILL:
					for (std::list<Player*>::const_iterator iter = PlayersList.begin(); iter != PlayersList.end(); ++iter)
					{
						Player* who = (*iter);
						switch (phase) {
						case 1: //Убить всех внутри
							if (who && !who->IsGameMaster() &&
								me->IsWithinDistInMap(who, 6.0f) &&
								who->IsAlive() &&
								!who->HasAura(27827) &&
								me->CanCreatureAttack(who))
							{
								me->DealDamage(who, who->GetMaxHealth() * 2);
							}
							break;
						case 2:
							if (who && !who->IsGameMaster() &&
								!me->IsWithinDistInMap(who, 6.0f) &&
								who->IsAlive() &&
								!who->HasAura(27827) &&
								me->CanCreatureAttack(who))
							{
								me->DealDamage(who, who->GetMaxHealth() * 2);
							}
							break;
						}
					}
					events.ScheduleEvent(EVENT_CHECK_TO_KILL, 500);
					break;
				}
			}
		}

	public:
		int phase;
		EventMap events;
		std::list<Player*> PlayersList;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_fire_kildjadenAI(creature);
	}
};

class Blood_of_Kildfaden : public CreatureScript
{
public:
	Blood_of_Kildfaden() : CreatureScript("npc_Blood_of_Kildjaden") { }

	struct Blood_of_KildfadenAI : public ScriptedAI //npc 418002
	{
		Blood_of_KildfadenAI(Creature* creature) : ScriptedAI(creature)
		{
			init();
		}

		void init()
		{
			me->setFaction(14); //NOT FRIENDLY
			me->SetMaxHealth(2341739);
			me->SetHealth(2107565);
			DoCast(SPELL_AURA_VISUAL);
			DoCast(me, 420000, true);
			events.ScheduleEvent(EVENT_CAST_BLOOD_BOLT, urand(2500, 5000));
			events.ScheduleEvent(EVENT_DEAL_MELEE_DAMAGE, 100);
			events.ScheduleEvent(EVENT_CAST_BLOOD_HEAL, urand(1000, 3500));
		}

		void DoMeleeAttack()
		{
			CalcDamageInfo Damage;
			me->ToUnit()->CalculateMeleeDamage(me->GetVictim(), 0, &Damage);
			Damage.attacker = me;
			me->DealDamageMods(me->GetVictim(), Damage.damage, &Damage.absorb);
			me->SendAttackStateUpdate(&Damage);
			me->ProcDamageAndSpell(Damage.target, Damage.procAttacker, Damage.procVictim, Damage.procEx, Damage.damage, Damage.attackType);
			me->DealMeleeDamage(&Damage, true);

			const int32 heal = Damage.damage * 38;
			const int32 heal_me = Damage.damage * 2;
			if (Summoner && me->GetHealth() == me->GetMaxHealth())
				me->CastCustomSpell(Summoner, SPELL_DARK_HEAL, &heal, NULL, NULL, true);
			me->CastCustomSpell(me, SPELL_DARK_HEAL, &heal_me, NULL, NULL, true);
		}

		void IsSummonedBy(Unit* summoner) override { Summoner = summoner; }

		void HealReceived(Unit* healer, uint32& heal) override
		{
			const int32 heal_val = heal * 3;
			if (Summoner && me->GetHealth() == me->GetMaxHealth())
				me->CastCustomSpell(Summoner, SPELL_DARK_HEAL, &heal_val, NULL, NULL, true);
		}

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_DEAL_MELEE_DAMAGE:
					if (me->IsWithinMeleeRange(me->GetVictim())) {
						DoMeleeAttack();
						events.ScheduleEvent(EVENT_DEAL_MELEE_DAMAGE, urand(1000, 1400));
					}
					else events.ScheduleEvent(EVENT_DEAL_MELEE_DAMAGE, 100);
					break;
				case EVENT_CAST_BLOOD_BOLT:
					DoCast(SPELL_BLOOD_BOLT);
					events.ScheduleEvent(EVENT_CAST_BLOOD_BOLT, urand(6000, 8000));
					break;
				case EVENT_CAST_BLOOD_HEAL:
					DoCast(SPELL_BLOOD_HEAL);
					events.ScheduleEvent(EVENT_CAST_BLOOD_HEAL, urand(12000, 17000));
					break;
				}
			}
		}

	public:
		EventMap events;
		Unit* Summoner;
		std::list<Player*> PlayersList;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new Blood_of_KildfadenAI(creature);
	}
};

void AddSC_new_KilDjaden()
{
	new Blood_of_Kildfaden();
	new npc_fire_kildjaden();
	new KilDjaden();
}