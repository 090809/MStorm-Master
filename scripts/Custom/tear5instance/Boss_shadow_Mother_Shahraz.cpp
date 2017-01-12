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

enum ACTIONS {
	ACTION_START = 1,
	CAST_DEATH_SPELL
};

enum EVENTS {
	EVENT_DEATH_TIME = 1,
	EVENT_DRAW_NEW_FIRES,
	EVENT_CALL_DUALS,
	EVENT_CAST_LIGHTING,
	EVENT_CAST_SPECTRAL,


	///
	EVENT_CAST_CIRCLE_OF_BLADES = 1,
	EVENT_CAST_DEATH_BLOW,
};

enum SPELLS {
	SPELL_LIGHTING = 40827,
	SPELL_FLAME_AURA = 74803,
	SPELL_SPECTRAL = 72688,

	SPELL_FIRE_RAY = 62894,
	SPELL_FROST_RAY = 62893,
	SPELL_NATURE_RAY = 62895,
	SPELL_ARCANE_RAY = 62897,


	////////////
	SPELL_CIRCLE_OF_BLADES = 63784,
	SPELL_DEATH_BLOW = 29572,
};

enum TEXTS {
	DEATH_TIMER_TRIGERRED = 100040,		//Пришло время разделаться с вами, раз и навсегда!
	SHADOW_MOTHER_SHAHRAZ_DEATH,	//Легион.. Вас... Уничтожит...
	PLAY_WITH_DARKNESS,				//Поиграем с тьмой?
	CALL_DUALS,						//Восстаньте, мертвые клинки! Режьте! Рубите! Танцуйте!
};

class Shadow_Mother_Shahraz : public CreatureScript
{
public:
	Shadow_Mother_Shahraz() : CreatureScript("boss_shadow_Mother_Shahraz") { }

	struct Shadow_Mother_ShahrazAI : public BossAI //npc 32353
	{
		Shadow_Mother_ShahrazAI(Creature* creature) : BossAI(creature, 4)
		{
			init();
		}

		void init() {
			me->SetMaxHealth(221354000); //Максимальное кол-во здоровья
			me->SetHealth(me->GetMaxHealth());
			me->SetMaxPower(POWER_MANA, 8946632);
			me->SetPower(POWER_MANA, 8946632);
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
			phase = 1;
			me->AddAura(60449, me);
			DoCast(420000);
			me->setFaction(35);
			Started = false;
		}

		SpellSchools GetRandomLowResist() {
			int i = urand(1, 4);
			SpellSchools k;
			uint32 _SPELL;

			if (me->HasAura(SPELL_FIRE_RAY)) me->RemoveAura(SPELL_FIRE_RAY);
			if (me->HasAura(SPELL_NATURE_RAY)) me->RemoveAura(SPELL_NATURE_RAY);
			if (me->HasAura(SPELL_FROST_RAY)) me->RemoveAura(SPELL_FROST_RAY);
			if (me->HasAura(SPELL_ARCANE_RAY)) me->RemoveAura(SPELL_ARCANE_RAY);

			me->SetResistance(SPELL_SCHOOL_HOLY, 350);
			me->SetResistance(SPELL_SCHOOL_FIRE, 750);
			me->SetResistance(SPELL_SCHOOL_NATURE, 750);
			me->SetResistance(SPELL_SCHOOL_FROST, 750);
			me->SetResistance(SPELL_SCHOOL_SHADOW, 350);
			me->SetResistance(SPELL_SCHOOL_ARCANE, 750);

			for (int j = 1; j < MAX_SPELL_SCHOOL; j++) {
				if (i == j) {
					switch (j) {
						//case 1: me->SetResistance(SPELL_SCHOOL_HOLY, 100);		k = SPELL_SCHOOL_HOLY; break;
					case 1: me->SetResistance(SPELL_SCHOOL_FIRE, 100);		k = SPELL_SCHOOL_FIRE;		_SPELL = SPELL_FIRE_RAY; break;
					case 2: me->SetResistance(SPELL_SCHOOL_NATURE, 100);	k = SPELL_SCHOOL_NATURE;	_SPELL = SPELL_NATURE_RAY; break;
					case 3: me->SetResistance(SPELL_SCHOOL_FROST, 100);		k = SPELL_SCHOOL_FROST;		_SPELL = SPELL_FROST_RAY; break;
						//case 5: me->SetResistance(SPELL_SCHOOL_SHADOW, 100);	k = SPELL_SCHOOL_SHADOW; break;
					case 4: me->SetResistance(SPELL_SCHOOL_ARCANE, 100);	k = SPELL_SCHOOL_ARCANE;	_SPELL = SPELL_ARCANE_RAY; break;
					}
				}
			}

			DoCast(_SPELL);
			return k;
		}

		void Reset() override
		{
			_Reset();
			init();
			me->SetPosition(me->GetHomePosition());
		}

		void MoveInLineOfSight(Unit* who)
		{
			if (who->GetTypeId() == TYPEID_PLAYER	&&
				!who->ToPlayer()->IsGameMaster()	&&
				me->IsWithinDistInMap(who, 10.0f)	&&
				!Started)
			{
				DoAction(ACTION_START);
			}
		}

		void DrawFire() {
			me->Yell(PLAY_WITH_DARKNESS);
			uint8 FiresToDraw = 2 + phase;
			std::vector<Creature*> Fires;

			for (std::list<Creature*>::const_iterator iter = FirePlaces.begin(); iter != FirePlaces.end(); ++iter) {
				Fires.push_back((*iter));
				if ((*iter)->HasAura(SPELL_FLAME_AURA)) (*iter)->RemoveAura(SPELL_FLAME_AURA);
			}

			while (FiresToDraw > 0) {
				int i = urand(0, Fires.size() - 1);
				Creature* fire = Fires[i];
				Fires.erase(Fires.begin() + i);
				std::vector<Creature*>(Fires).swap(Fires);
				fire->AddAura(SPELL_FLAME_AURA, fire);
				fire->AddAura(420000, fire);
				//fire->SetInt32Value(UNIT_FIELD_ATTACK_POWER, 50400);
				//fire->CastSpell(me, 74803, false);
				FiresToDraw--;
			}
			Fires.clear();
		}

		void CallDuals() {
			me->Yell(CALL_DUALS);
			Pl.clear();
			me->GetPlayerListInGrid(Pl, 50.0f);
			int i = 0;
			for (std::list<Player*>::const_iterator iter = Pl.begin(); iter != Pl.end(); ++iter) {
				Player* player = (*iter);
				Creature* dual_swords = me->SummonCreature(211832, player->GetPosition());
				dual_swords->Attack(player, true);
				dual_swords->SetInCombatWith(player);
				i++;
				if (i > 3) break;
			}
		}

		void DoAction(int32 action) override {
			if (action == ACTION_START)
			{
				Started = true;
				me->setFaction(14);
				me->FindNearestGameObject(186859, 50.0f)->SetGoState(GO_STATE_READY);
				me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_NOT_SELECTABLE);
				Player* pl = me->SelectNearestPlayer(10.0f);
				me->SetInCombatWith(pl);
				me->Attack(pl, true);
				me->GetCreatureListWithEntryInGrid(FirePlaces, 211831, 50.0f);
				DrawFire();
				events.ScheduleEvent(EVENT_CALL_DUALS, urand(5000, 7500));
				randomed_resist = GetRandomLowResist();
				events.ScheduleEvent(EVENT_DRAW_NEW_FIRES, urand(10000, 16000));
				events.ScheduleEvent(EVENT_CAST_LIGHTING, urand(5000, 7000));
				events.ScheduleEvent(EVENT_CAST_SPECTRAL, urand(4000, 6500));
			}
			else if (action == CAST_DEATH_SPELL)
			{
				DoCast(10260); //Only visual timer
				events.ScheduleEvent(EVENT_DEATH_TIME, 20000);
			}
		}

		void EnterEvadeMode() override {
			_EnterEvadeMode();
			Reset();
			Started = false;
			me->FindNearestGameObject(186859, 50.0f)->SetGoState(GOState::GO_STATE_ACTIVE);

			std::vector<Creature*> Fires;
			for (std::list<Creature*>::const_iterator iter = FirePlaces.begin(); iter != FirePlaces.end(); ++iter) {
				Fires.push_back((*iter));
				if ((*iter)->HasAura(SPELL_FLAME_AURA)) (*iter)->RemoveAura(SPELL_FLAME_AURA);
			}
		}

		void JustDied(Unit* /*killer*/) override
		{
			me->Yell(SHADOW_MOTHER_SHAHRAZ_DEATH);
			//me->SummonGameObject(300003, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, -1);
			me->FindNearestGameObject(186859, 50.0f)->SetGoState(GOState::GO_STATE_ACTIVE);

			std::vector<Creature*> Fires;
			for (std::list<Creature*>::const_iterator iter = FirePlaces.begin(); iter != FirePlaces.end(); ++iter) {
				Fires.push_back((*iter));
				if ((*iter)->HasAura(SPELL_FLAME_AURA)) (*iter)->RemoveAura(SPELL_FLAME_AURA);
			}
		}

		void UpdateAI(uint32 diff) override
		{
			if (!UpdateVictim())
				EnterEvadeMode();

			death_timer -= diff;
			events.Update(diff);
			if (phase != 4) DoMeleeAttackIfReady();
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_DEATH_TIME:
					me->GetPlayerListInGrid(Pl, 40.0f);
					for (std::list<Player*>::const_iterator iter = Pl.begin(); iter != Pl.end(); ++iter)
					{
						if ((*iter)->IsAlive()) me->Kill((*iter));
					}
					me->CombatStop(true);
					me->Yell(DEATH_TIMER_TRIGERRED);
					break;
				case EVENT_DRAW_NEW_FIRES:
					DrawFire();
					events.ScheduleEvent(EVENT_DRAW_NEW_FIRES, urand(10000, 16000));
					break;
				case EVENT_CALL_DUALS:
					CallDuals();
					events.ScheduleEvent(EVENT_CALL_DUALS, urand(25000, 36000));
					break;
				case EVENT_CAST_LIGHTING:
					if (phase != 4)
					{
						DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), SPELL_LIGHTING, true);
						events.ScheduleEvent(EVENT_CAST_LIGHTING, urand(5000, 7000));
					}
					break;
				case EVENT_CAST_SPECTRAL:
					if (phase != 4)
					{
						DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), SPELL_SPECTRAL, true);
						events.ScheduleEvent(EVENT_CAST_SPECTRAL, urand(4000, 6500));
					}
					break;
				}
			}
		}
		void SpellHit(Unit* /*caster*/, SpellInfo const* spell) override
		{
			if (spell->SchoolMask == randomed_resist) {
				death_timer = 200;
			}
		}

		void DamageTaken(Unit* done_by, uint32 &damage) override
		{
			if (damage >= me->GetHealth()) {
				if (phase < 3) {
					damage -= me->GetHealth();
					if (death_timer > 0) {
						me->SetMaxHealth(me->GetMaxHealth() / 2);
						damaged_by_spell = true;
					}
					me->SetHealth(me->GetMaxHealth());
					phase++;
					randomed_resist = GetRandomLowResist();
					DoCast(58854);
				}
				else if (phase == 3) {
					damage -= me->GetHealth();
					me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_INTERRUPT, true);  // Should be interruptable unless overridden by spell (Overload)
					me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_STUN, true);       // Reset immumity, Brundir should be stunnable by default
					DoAction(CAST_DEATH_SPELL);
					me->SetMaxHealth(79654287);
					me->SetHealth(me->GetMaxHealth());
					phase++;
				}
				//if (phase != 4 && (phase == 3 && !damaged_by_spell)) ; //Resurection - visual;
			}

		}

	private:
		uint8 phase, death_timer;
		SpellSchools randomed_resist;
		bool damaged_by_spell;
		std::list<Creature*> FirePlaces;
		std::list<Player*> Pl;
		bool Started;

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new Shadow_Mother_ShahrazAI(creature);
	}
};

class NPC_Dual_Swords : public CreatureScript
{
public:
	NPC_Dual_Swords() : CreatureScript("npc_dual_swords") { }

	struct NPC_Dual_SwordsAI : public ScriptedAI
	{
		NPC_Dual_SwordsAI(Creature* creature) : ScriptedAI(creature)
		{
			init();
		}

		void init() {
			me->SetMaxHealth(253435);
			me->SetInt32Value(UNIT_FIELD_ATTACK_POWER, 480400);
			DoCast(420000);
			me->SetHealth(me->GetMaxHealth());
			events.ScheduleEvent(EVENT_CAST_CIRCLE_OF_BLADES, urand(5000, 12000));
			DoCastVictim(SPELL_DEATH_BLOW);
			events.ScheduleEvent(EVENT_CAST_DEATH_BLOW, urand(5000, 9000));
		}

		void UpdateFlagsFromFireAuras() {
			FirePlaces.clear();
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
			me->GetCreatureListWithEntryInGrid(FirePlaces, 211831, 3.0f);
			for (std::list<Creature*>::const_iterator iter = FirePlaces.begin(); iter != FirePlaces.end(); ++iter) {
				if ((*iter)->HasAura(SPELL_FLAME_AURA)) {
					me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
					break;
				}
			}
		}

		void UpdateAI(uint32 diff) override
		{
			UpdateFlagsFromFireAuras();
			DoMeleeAttackIfReady();
			events.Update(diff);
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_CAST_CIRCLE_OF_BLADES:
					DoCast(SPELL_CIRCLE_OF_BLADES);
					events.ScheduleEvent(EVENT_CAST_CIRCLE_OF_BLADES, urand(14000, 18000));
					break;
				}
			}

		}
	private:
		std::list<Creature*> FirePlaces;
		EventMap events;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new NPC_Dual_SwordsAI(creature);
	}
};

void AddSC_Shadow_Mother_Shahraz()
{
	new Shadow_Mother_Shahraz();
	new NPC_Dual_Swords();
}
