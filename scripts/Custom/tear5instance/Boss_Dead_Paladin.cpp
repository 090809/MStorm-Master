/*class test : public CreatureScript
{
public:
	test() : CreatureScript("test") { }

	struct npc_valeera_sanguinarAI : public ScriptedAI //npc 32353
	{
		testAI(Creature* creature) : ScriptedAI(creature)
		{
		}

		void Reset() override
		{
		}

		void DoAction(int32 action) override {
			if (action = 1)
			{
			}
		}

		void EnterCombat(Unit* ) override
		{
		}

		void UpdateAI(uint32 diff) override
		{
		}

		void DamageTaken(Unit* done_by, uint32 &damage) override
		{
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new testAI(creature);
	}
};

void AddSC_start_event_scr()
{
	new test();
} 
*/

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

enum Paladin_event_structure {
	DEAD_PALADIN_ENTRY = 211810,
	SPIRIT_ENTRY = 211811,

	//Events
	EVENT_DAMAGE_OVERFLOW = 0,
	EVENT_CAN_BUBBLE_NOW = 1,
	EVENT_DARK_JUSTICE = 2,
	EVENT_DARK_BUFF = 3,
	EVENT_DARK_DEBUFF = 4,
	EVENT_STUN = 5,
	EVENT_DARK = 6,
	SWITCH_TARGET,

	//Spells
	SPELL_STUN = 58154,
	SPELL_BUBBLE = 64505,
	SPELL_DARK = 33914,
	SPELL_DARK_BUFF = 69391,
	SPELL_DARK_DEBUFF = 39153,
	SPELL_DARK_JUSTICE = 34112,
	SPELL_DARK_ENRAGE = 48292,

	TEXT_SPIRIT_FREE = 100010, //Свободу темный душам!
	TEXT_ENTER_COMBAT		//Я подарю вам освобождение!
};

class boss_dead_paladin : public CreatureScript
{
public:
	boss_dead_paladin() : CreatureScript("boss_dead_paladin") { }

	struct boss_dead_paladinAI : public BossAI //npc 32353
	{
		boss_dead_paladinAI(Creature* creature) : BossAI(creature, 1)
		{
			Initialize();
		}

		void Initialize() {
			//me->SetInt32Value(UNIT_FIELD_ATTACK_POWER_MULTIPLIER, 40);
			//me->SetInt32Value(UNIT_FIELD_MINDAMAGE, 74000);
			//me->SetInt32Value(UNIT_FIELD_MAXDAMAGE, 140000);
			//me->UpdateAttackPowerAndDamage();

			me->SetMaxHealth(84685824 * 4);
			me->SetHealth(84685824 * 4);
			me->SetAttackTime(BASE_ATTACK, 800);
			me->SetMaxPower(POWER_MANA, 93 * 681);
			me->SetPower(POWER_MANA, me->GetMaxPower(POWER_MANA));
			me->setFaction(14);
			SpiritCreated = false;
			DamageOveflow = false;
			DamageDoneByPlayer = 0;
			DamageDoneByHeal = 0;
		}

		void Reset() override
		{
			_Reset();
			Initialize();
			me->SetPosition(me->GetHomePosition());
		}

		void EnterEvadeMode() override {
			Reset();
		}

		/*void DoAction(int32 action) override {
			if (action = 1)
			{
			}
		}*/

		void EnterCombat(Unit*) override
		{
			me->Yell(TEXT_ENTER_COMBAT);
			me->SetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER, 100);
			//me->SetInt32Value(UNIT_FIELD_MINDAMAGE, 74000);
			//me->SetInt32Value(UNIT_FIELD_MAXDAMAGE, 140000);

			events.ScheduleEvent(EVENT_DARK_JUSTICE, urand(3000, 8000));
			events.ScheduleEvent(EVENT_DARK_DEBUFF, urand(2000, 4000));
			events.ScheduleEvent(EVENT_STUN, urand(7000, 9000));
			events.ScheduleEvent(EVENT_DARK, urand(2000, 6000));
			events.ScheduleEvent(SWITCH_TARGET, urand(5000, 10000));

		}

		void UpdateAI(uint32 diff) override
		{
			if (!me->GetVictim() && me->IsInCombat())
				if (!me->SelectNearestPlayer(100.0f)) {
					EnterEvadeMode();
					Reset();
					events.Reset();
				}
			
			events.Update(diff);
			DoMeleeAttackIfReady();
			if (me->GetHealthPct() < 75.0f && !SpiritCreated) {
				SpiritCreated = true;
				me->Yell(TEXT_SPIRIT_FREE);
				spirit = me->SummonCreature(SPIRIT_ENTRY, spirit_pos);
				events.ScheduleEvent(EVENT_CAN_BUBBLE_NOW, urand(3000, 7000));
				events.ScheduleEvent(EVENT_DARK_BUFF, urand(1000, 3000));
			}

			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_DAMAGE_OVERFLOW:
					DoCast(SPELL_DARK_ENRAGE);
					events.ScheduleEvent(EVENT_DAMAGE_OVERFLOW, urand(28000, 37000));
					break;
				case EVENT_CAN_BUBBLE_NOW:
					DoCast(SPELL_BUBBLE);
					events.ScheduleEvent(EVENT_CAN_BUBBLE_NOW, urand(18000, 24000));
					break;
				case EVENT_DARK_BUFF:
					DoCast(SPELL_DARK_BUFF);
					events.ScheduleEvent(EVENT_DARK_BUFF, urand(10000, 18000));
					break;
				case EVENT_DARK_JUSTICE:
					DoCastVictim(SPELL_DARK_JUSTICE);
					events.ScheduleEvent(EVENT_DARK_JUSTICE, urand(10000, 14000));
					break;
				case EVENT_DARK_DEBUFF:
					DoCastVictim(SPELL_DARK_DEBUFF);
					events.ScheduleEvent(EVENT_DARK_BUFF, urand(4000, 8000));
					break;
				case EVENT_STUN:
					DoCastVictim(SPELL_STUN);
					events.ScheduleEvent(EVENT_STUN, urand(6000, 14000));
					break;
				case EVENT_DARK:
					DoCastVictim(SPELL_DARK);
					events.ScheduleEvent(EVENT_DARK, urand(4000, 10000));
					break;
				case SWITCH_TARGET:
					me->AddThreat(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), 10000);
					events.ScheduleEvent(SWITCH_TARGET, urand(5000, 10000));
					break;
				}
			}
		}

		void DamageTaken(Unit* done_by, uint32 &damage) override
		{
			uint32* dmg = &damage;
			if ((DamageDoneByPlayer >= me->GetMaxHealth() / 1.6f) && done_by->IsControlledByPlayer()) 
			{
				damage = 0;
				me->SetHealth(me->GetHealth()); //CloseVisualBug;
				//if (urand(0, 9) == 3) { me->SetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER, 100 + times_inc++);}
				if (!DamageOveflow) {
					DamageOveflow = true;
					events.ScheduleEvent(EVENT_DAMAGE_OVERFLOW, urand(1000, 5000));
				}
			}
			else if (done_by->IsControlledByPlayer()){
				DamageDoneByPlayer += *dmg;
			} 
			else if ((DamageDoneByHeal >= me->GetMaxHealth() / 2) && !done_by->IsControlledByPlayer()) {
				damage = 0;
				me->SetHealth(me->GetHealth());
			}
			else if (done_by == me->FindNearestCreature(SPIRIT_ENTRY, 1000.0f)->ToUnit()) {
				DamageDoneByHeal += *dmg;
			}

			if (damage > me->GetHealth()) {
				GameObject* door = me->FindNearestGameObject(300000, 200.0f);
				if (door && door != NULL) door->SetGoState(GOState::GO_STATE_ACTIVE);

				spirit = me->FindNearestCreature(SPIRIT_ENTRY, 100.0f);
				if (spirit && spirit != NULL) spirit->SetVisible(false);
				//me->GetInstanceScript()->SetData(1, DONE);
			}
		}

		void JustDied(Unit* /*killer*/) override
		{ 
		}

	private:
		uint32 DamageDoneByPlayer = 0, DamageDoneByHeal = 0, times_inc = 0;
		Unit* spirit;
		Position spirit_pos = Position(274.509064f, -100.029732f, 28.869146f, 3.145755f);
		bool DamageOveflow = false, SpiritCreated = false;
	};

	
	CreatureAI* GetAI(Creature* creature) const override
	{
		return new boss_dead_paladinAI(creature);
	}
};

class spirit_cre : public CreatureScript
{
public:
	spirit_cre() : CreatureScript("npc_dead_paladin_spirit") { }

	struct spirit_creAI : public ScriptedAI
	{
		spirit_creAI(Creature* creature) : ScriptedAI(creature) {
			Initialize();
		}

		void Initialize() {
			me->SetMaxHealth(me->FindNearestCreature(DEAD_PALADIN_ENTRY, 1000.0f)->GetMaxHealth());
			me->SetHealth(me->GetMaxHealth() / 2);
			me->setFaction(35); 
			Creature* palad = me->FindNearestCreature(DEAD_PALADIN_ENTRY, 1000.0f);
			if (palad != NULL) DoCast(palad, 72735);
		}

		void HealReceived(Unit* healer, uint32& heal) override
		{
			Creature* palad = me->FindNearestCreature(DEAD_PALADIN_ENTRY, 1000.0f);
			if (palad && palad != NULL) me->DealDamage(palad, heal);
			if (heal >= me->GetMaxHealth() - me->GetHealth()) heal = 0;

		}

		void DamageTaken(Unit* done_by, uint32 &damage) override
		{
			damage = 0;
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new spirit_creAI(creature);
	}
};

void AddSC_boss_dead_palladin()
{
	new boss_dead_paladin();
	new spirit_cre();
}