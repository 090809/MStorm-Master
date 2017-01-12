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

enum statics {
	TEXT_ENERGY_OUT = 100020,

	EVENT_SUMMON = 1,
};

class Necromancer : public CreatureScript
{
public:
	Necromancer() : CreatureScript("boss_t5_necromancer") { }

	struct NecromancerAI : public BossAI
	{
		NecromancerAI(Creature* creature) : BossAI(creature, 2) { init(); }

		void init() {
			//me->SetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER, 100);
			timer = 2200; 
			me->SetMaxHealth(114534892);
			me->SetHealth(114534892);
			me->setFaction(35);
			not_select_trigger = true;
		}

		void DoAction(int32 action) override {
			if (action == 1)
			{
				me->SetInCombatWithZone();
				events.ScheduleEvent(EVENT_SUMMON, timer);
				DoCast(trigger, 45491);
			}
		}
		
		void EnterEvadeMode() override {
			Reset();
		}

		void Reset() override {
			_Reset();
			init();
		}

		void SetData(uint32 value, uint32 uiValue) override
		{
			if (value == 1 && uiValue == 1) {
				DoAction(1);
			}
		}

		void UpdateAI(uint32 diff) override
		{
			if (not_select_trigger) {
				trigger = me->FindNearestCreature(211820, 100);
				not_select_trigger = false;
			}
			events.Update(diff);
			
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_SUMMON:
					TempSummon* cre = me->SummonCreature(211822, me->GetPosition());
					cre->SetMaxHealth(14534892);
					cre->SetHealth(14534892);
					cre->CastSpell(cre, 69350);
					//cre->CastSpell(cre, 72723);
					cre->CastSpell(cre, 67236);
					cre->GetMotionMaster()->MoveRandom(10.0f);
					//cre->Attack(me->SelectNearestPlayer(50.0f), true);
					timer -= 30;
					if (timer <= 1400)
					{
						//if (trigger) trigger->AI()->DoAction(1);
						me->Yell(TEXT_ENERGY_OUT);
						me->setFaction(14);
						DoCast(33908);
					} else { events.ScheduleEvent(EVENT_SUMMON, timer+urand(0, 4)); }
					break;
				}
			}
			if (!me->HasAura(33908)) DoCast(33908);
			DoMeleeAttackIfReady();
		}

	private:
		uint32 timer;
		Creature* trigger;
		bool not_select_trigger;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new NecromancerAI(creature);
	}
};

class NecroTrigger : public CreatureScript
{
public:
	NecroTrigger() : CreatureScript("npc_necro_trigger") { }

	struct NecroTriggerAI : public ScriptedAI
	{
		NecroTriggerAI(Creature* creature) : ScriptedAI(creature) { init(); }

		void init() {
			count = 3;
			not_started = true;
			me->setFaction(14);
			BombTimer = 0;
		}

		void EnterEvadeMode() override {
			std::list<Creature*> Skeletons;

			_EnterEvadeMode();
			for (std::list<Creature*>::const_iterator iter = Necros.begin(); iter != Necros.end(); ++iter) {
				if (*iter) (*iter)->Respawn(true);
			}
			me->GetCreatureListWithEntryInGrid(Skeletons, 211822, 100.0f);
			for (std::list<Creature*>::const_iterator iter = Skeletons.begin(); iter != Skeletons.end(); ++iter) {
				me->DealDamage((*iter), (*iter)->GetMaxHealth()+100);
			}
			me->FindNearestGameObject(175570, 100.0f)->SetGoState(GOState::GO_STATE_ACTIVE);
			init();
		}

		void EnterCombat(Unit* /*who*/) override {
			//BombTimer = 4500;
			//std::list<Creature*> Necros;
			me->GetCreatureListWithEntryInGrid(Necros, 211821, 50.0f);
			uint32 i = 0;
			if (Necros.size() > 0)
				for (std::list<Creature*>::const_iterator iter = Necros.begin(); iter != Necros.end(); ++iter)
				{
					if ((*iter)->IsAlive()) (*iter)->AI()->DoAction(1);
					i++;
				}
			//me->FindNearestGameObject(175570, 40.0f)->setActive(false);
			DoCast(me, 47848, true);
			not_started = false;
			me->FindNearestGameObject(175570, 100.0f)->SetGoState(GOState::GO_STATE_READY);
		}

		bool CheckNecrosAlive() {
			bool Alive;
			for (std::list<Creature*>::const_iterator iter = Necros.begin(); iter != Necros.end(); ++iter) {
				//for (int i = 0; i < 3; i++) {
				if ((*iter)->IsAlive()) {
					Alive = true;
					break;
				} else {
					Alive = false;
				}
			}
			return Alive;
		}

		void UpdateAI(uint32 diff) override
		{
			if (!not_started) {
				if (!UpdateVictim())
					EnterEvadeMode();
				//if (BombTimer <= 0 ) {
				//	DoCast(me->SelectNearestPlayer(50.0f), 71055, true);
				//	BombTimer = 4500;
				//} else { BombTimer -= diff; }

				if (!CheckNecrosAlive()) {
					me->DealDamage(me, me->GetMaxHealth() + 100); 
					me->SummonGameObject(300001, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0,0,0,0,0,-1);
					me->FindNearestGameObject(181170, 1000.0f)->SetGoState(GOState::GO_STATE_ACTIVE);
					me->FindNearestGameObject(175570, 100.0f)->SetGoState(GOState::GO_STATE_ACTIVE);
				}
			}
		}

	private:
		bool not_started = true;
		uint32 count, BombTimer = 0;
		Player* player;
		std::list<Creature*> Necros;
		bool NecrosAlive;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new NecroTriggerAI(creature);
	}
};

void AddSC_boss_necromancers()
{
	new Necromancer();
	new NecroTrigger();
} 
