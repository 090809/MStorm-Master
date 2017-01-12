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

#define QUEST_SURVIVE_ID 509190
#define DUMMY_ENTRY 509190

enum DATA {
	DATA_HIGH_GUID_PLAYER = 1,
};

struct Positions
{
	float x, y, z, entry;
};

enum TEXTS {
	TEXT_KILL_ALL = 0,
	TEXT_NO_QUEST,
};

enum SPELLS {
	SPELL_INTERUPT = 29961,
	SPELL_POISON = 44289,
};

enum EVENTS {
	EVENT_SPAWN_NEW_WAVE = 1,
	EVENT_INTERUPT,
	EVENT_TIMER_INTERUPT,
	EVENT_DE_INVIS,
	EVENT_POISON,
};

static Positions SpawnPos[12]
{
	{ -97.352760, 124.630942, -40.284003, DUMMY_ENTRY + 1 },	//Мощный стрелок 1
	{ -96.954849, 173.927353, -40.284003, DUMMY_ENTRY + 1 },	//Мощный стрелок 2
	{ -97.173470, 150.150650, -40.284003, DUMMY_ENTRY + 2 },	//Антиинвиз
	{ -105.956398, 149.986420, -40.284003, DUMMY_ENTRY + 3 },	//Антимагия
	{ -104.770660, 158.200424, -40.284003, DUMMY_ENTRY + 4 },	//Хилер 1
	{ -87.645645, 140.628510, -40.384003, DUMMY_ENTRY + 4 },	//Хилер 2
	{ -123.887978, 150.055098, -40.384003, DUMMY_ENTRY + 5 },	//Жир 1
	{ -123.887978, 147.055098, -40.384003, DUMMY_ENTRY + 5 },	//Жир 2
	{ -123.887978, 153.055098, -40.384003, DUMMY_ENTRY + 5 },	//Жир 3
	{ -70.387375, 150.055098, -40.384003, DUMMY_ENTRY + 6 },	//Быстрый 1
	{ -70.387375, 147.055098, -40.384003, DUMMY_ENTRY + 6 },	//Быстрый 2
	{ -70.387375, 153.055098, -40.384003, DUMMY_ENTRY + 6 }		//Быстрый 3
};

class ChallengeSurviveDummy : public CreatureScript
{
enum DUMMY_ACTIONS {
	ACTION_START = 0,
	ACTION_END_EVENT,
	ACTION_FAILED,
};

public:
	ChallengeSurviveDummy() : CreatureScript("ChallengeSurvive_Dummy") { }

	struct ChallengeSurviveAI : public BossAI
	{
		ChallengeSurviveAI(Creature* creature) : BossAI(creature, 1)
		{
			init();
		}
		
		void init() {
			waves_count = 0;
			PlayersList.clear();
			selected_player = NULL;
			SummonedCreatures.clear();
		}

		void Reset() override
		{
			_Reset();
			init();
		}

		void DoAction(int32 action) override {
			if (action == 1) // Начало ивента.
			{
				me->GetPlayerListInGrid(PlayersList, 1000.0f);
				for (std::list<Player*>::const_iterator iter = PlayersList.begin(); iter != PlayersList.end(); ++iter)
					if ((*iter)->GetGUIDHigh() != me->AI()->GetData(DATA_HIGH_GUID_PLAYER))
					{
						me->DealDamage((*iter), (*iter)->GetMaxHealth() * 2);
						kill_all_text = true;
					} else selected_player = (*iter);

				if (kill_all_text) me->Yell(TEXT_KILL_ALL);
				events.ScheduleEvent(EVENT_SPAWN_NEW_WAVE, 1 * IN_MILLISECONDS * 60);
				SpawnNewWave();
			} else if (action == 2) { //Побеееееда
				me->GetPlayerListInGrid(PlayersList, 1000.0f);
				for (std::list<Player*>::const_iterator iter = PlayersList.begin(); iter != PlayersList.end(); ++iter)
					if ((*iter)->GetGUIDHigh() == me->AI()->GetData(DATA_HIGH_GUID_PLAYER))
					{
						(*iter)->CompleteQuest(QUEST_SURVIVE_ID);
						break;
					}
			} else { // Еще один лох откинул коньки
				me->GetPlayerListInGrid(PlayersList, 1000.0f);
				for (std::list<Player*>::const_iterator iter = PlayersList.begin(); iter != PlayersList.end(); ++iter)
					if ((*iter)->GetGUIDHigh() == me->AI()->GetData(DATA_HIGH_GUID_PLAYER))
					{
						(*iter)->FailQuest(QUEST_SURVIVE_ID);
						break;
					}
				
				for (std::vector<Creature*>::const_iterator iter = SummonedCreatures.begin(); iter != SummonedCreatures.end(); ++iter)
				{
					Creature* cre = (*iter);
					if (cre->IsAlive())
						cre->AI()->Reset();
				}
			}
		}

		void SpawnNewWave() {
			if (waves_count < 5) {
				waves_count++;
				for (int i = 0; i < 12; i++)
				{
					SummonedCreatures.push_back(me->SummonCreature(
						SpawnPos[i].entry, SpawnPos[i].x, SpawnPos[i].y, SpawnPos[i].z
						));
					(*(SummonedCreatures.end()))->GetAI()->SetData(DATA_HIGH_GUID_PLAYER, me->AI()->GetData(DATA_HIGH_GUID_PLAYER));
				}
			} else DoAction(ACTION_END_EVENT);
		}

		void UpdateAI(uint32 diff) override
		{
			if (!selected_player || !selected_player->IsAlive()) DoAction(ACTION_FAILED);
			events.Update(diff);
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_SPAWN_NEW_WAVE:
					SpawnNewWave();
					events.ScheduleEvent(EVENT_SPAWN_NEW_WAVE, 1 * IN_MILLISECONDS * 60);
					break;
				}
			}
		}

	public:
		std::list<Player*> PlayersList;
		bool kill_all_text;
		int waves_count;
		Player* selected_player;
		std::vector<Creature*> SummonedCreatures;
	};

	bool OnGossipHello(Player* player, Creature* creature) override
	{
		//player->ADD_GOSSIP_ITEM(GOSSIP_ICON_TALK, "Подробнее об испытании Выживания.", 0, 1);
		player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_BATTLE, "Начать испытание.", 0, 2, "Вы уверены, что хотите начать испытание?", 0, false);
		player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
		return true;
	};

	bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
	{
		player->PlayerTalkClass->ClearMenus();
		switch (action)
		{
		case 1: //Вывод информации
			break;
		case 2:
			if (!player->hasQuest(QUEST_SURVIVE_ID)) {
				creature->Whisper(TEXT_NO_QUEST, player);
				break;
			}
			creature->AI()->DoAction(ACTION_START);
			creature->AI()->SetData(DATA_HIGH_GUID_PLAYER, player->GetGUIDHigh());
			break;
		};
		return true;
	}

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new ChallengeSurviveAI(creature);
	};
};

class ChallengeSurviveTotemAntiMagic : public CreatureScript
{
public:
	ChallengeSurviveTotemAntiMagic() : CreatureScript("npc_ChallengeSurviveTotemAntiMagic") { }

	struct npc_ChallengeSurviveTotemAntiMagicAI : public ScriptedAI //npc 32353
	{
		npc_ChallengeSurviveTotemAntiMagicAI(Creature* creature) : ScriptedAI(creature)
		{
			init();
		}

		void init() {
			me->SetMaxHealth(643314);
			me->SetHealth(643314);
		}

		void Reset() override { me->DespawnOrUnsummon(); }

		void SetData(uint32 value, uint32 uiValue) override
		{
			if (value == DATA_HIGH_GUID_PLAYER)
			{
				me->GetPlayerListInGrid(PlayersList, 1000.0f);
				for (std::list<Player*>::const_iterator iter = PlayersList.begin(); iter != PlayersList.end(); ++iter)
					if ((*iter)->GetGUIDHigh() == me->AI()->GetData(DATA_HIGH_GUID_PLAYER))
					{
						selected_player = (*iter);
						break;
					}
				events.ScheduleEvent(EVENT_INTERUPT, 500);
			}
		}
		//void EnterEvadeMode() override { Reset(); }

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);
			if (selected_player->IsNonMeleeSpellCast(false, false, true)) 
				spell_casting = true;
			else spell_casting = false;
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
					case EVENT_INTERUPT:
						if (spell_casting) {
							events.ScheduleEvent(EVENT_TIMER_INTERUPT, 500);
							events.ScheduleEvent(EVENT_INTERUPT, 15000);
						} else events.ScheduleEvent(EVENT_INTERUPT, 500); 
						break;
					case EVENT_TIMER_INTERUPT:
						me->CastSpell(selected_player, SPELL_INTERUPT);
						break;
				}
			}
		}

	public:
		EventMap events;
		std::list<Player*> PlayersList;
		Player* selected_player;
		bool spell_casting;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new npc_ChallengeSurviveTotemAntiMagicAI(creature);
	}
};

class ChallengeSurviveTotemInvisibility : public CreatureScript
{
public:
	ChallengeSurviveTotemInvisibility() : CreatureScript("npc_ChallengeSurviveTotemInvisibility") { }

	struct ChallengeSurviveTotemInvisibilityAI : public ScriptedAI //npc 32353
	{
		ChallengeSurviveTotemInvisibilityAI(Creature* creature) : ScriptedAI(creature)
		{
			init();
		}

		void init() {
			me->SetMaxHealth(476314);
			me->SetHealth(476314);
		}

		void Reset() override { me->DespawnOrUnsummon(); }

		void SetData(uint32 value, uint32 uiValue) override
		{
			if (value == DATA_HIGH_GUID_PLAYER)
			{
				me->GetPlayerListInGrid(PlayersList, 1000.0f);
				for (std::list<Player*>::const_iterator iter = PlayersList.begin(); iter != PlayersList.end(); ++iter)
					if ((*iter)->GetGUIDHigh() == me->AI()->GetData(DATA_HIGH_GUID_PLAYER))
					{
						selected_player = (*iter);
						break;
					}
				events.ScheduleEvent(EVENT_DE_INVIS, 2500);
			}
		}
		//void EnterEvadeMode() override { Reset(); }

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);

			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_DE_INVIS:
					if (urand(0, 2) == 1 && selected_player && selected_player->IsAlive() && me->IsInRange(selected_player, 0, 25))
						me->CastSpell(selected_player, 770);
					events.ScheduleEvent(EVENT_DE_INVIS, 5500);
				}
			}
		}

	public:
		EventMap events;
		std::list<Player*> PlayersList;
		Player* selected_player;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new ChallengeSurviveTotemInvisibilityAI(creature);
	}
};

class ChallengeSurviveNPCFighterMelee: public CreatureScript
{
public:
	ChallengeSurviveNPCFighterMelee() : CreatureScript("npc_ChallengeSurviveNPCFighterMelee") { }

	struct ChallengeSurviveNPCFighterMeleeAI : public ScriptedAI //npc 32353
	{
		ChallengeSurviveNPCFighterMeleeAI(Creature* creature) : ScriptedAI(creature)
		{
			init();
		}

		void init() {
			me->SetMaxHealth(786314);
			me->SetHealth(786314);
		}

		void Reset() override { me->DespawnOrUnsummon(); }

		void SetData(uint32 value, uint32 uiValue) override
		{
			if (value == DATA_HIGH_GUID_PLAYER)
			{
				me->GetPlayerListInGrid(PlayersList, 1000.0f);
				for (std::list<Player*>::const_iterator iter = PlayersList.begin(); iter != PlayersList.end(); ++iter)
					if ((*iter)->GetGUIDHigh() == me->AI()->GetData(DATA_HIGH_GUID_PLAYER))
					{
						selected_player = (*iter);
						break;
					}
				events.ScheduleEvent(EVENT_POISON, 500);
			}
		}
		//void EnterEvadeMode() override { Reset(); }

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);
			DoMeleeAttackIfReady();
			while (uint32 eventId = events.ExecuteEvent()) {
				switch (eventId) {
				case EVENT_POISON:
					if (urand(0, 2) == 1 && selected_player && selected_player->IsAlive() && me->IsInRange(selected_player, 0, 4)) {
						me->CastSpell(selected_player, SPELL_POISON);
						events.ScheduleEvent(EVENT_POISON, 14000);
					} else events.ScheduleEvent(EVENT_DE_INVIS, 850);
				}
			}
		}

	public:
		EventMap events;
		std::list<Player*> PlayersList;
		Player* selected_player;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new ChallengeSurviveNPCFighterMeleeAI(creature);
	}
};


void AddSC_Challenge_Survive()
{
	new ChallengeSurviveDummy();
	new ChallengeSurviveTotemAntiMagic();
	new ChallengeSurviveTotemInvisibility();
} 
