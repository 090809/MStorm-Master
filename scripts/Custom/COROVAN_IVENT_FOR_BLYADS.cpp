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


class spawner : public CreatureScript {
public:
	spawner() : CreatureScript("spawner") { }
	struct SpawnerAI : public ScriptedAI
	{
		//npc_nether_portalAI(Creature* creature) : ScriptedAI(creature), _summons(me) { }
		SpawnerAI(Creature* creature) : ScriptedAI(creature), _summons(me) { };
		TempSummon* summoned;
		uint32 GroupMembersCount = 0;

		int8 StandartCount;
		int8 EliteCount;
		int8 BossCount;

		int32 _updateTimer;
		int8 Action;

		void DoAction(int32 action) override
		{
			_updateTimer = 0.5 * 1000;
			me->CastSpell(me, 68424, false);
			Action = action;
			switch (Action)
			{
				//Standart
			case 1:
				StandartCount = 2 + GroupMembersCount;
				EliteCount = 0;
				BossCount = 0;
				break;
				//Elite
			case 2:
				StandartCount = 1 + GroupMembersCount;
				EliteCount = 1 + GroupMembersCount;
				BossCount = 0;
				break;
				//Boss
			case 3:
				StandartCount = 3 + GroupMembersCount;
				EliteCount = 1 + GroupMembersCount;
				BossCount = 1;
				break;
			}
		};

		void JustSummoned(Creature* summoned) override
		{
			_summons.Summon(summoned);
			// makes immediate corpse despawn of summoned Mistress of Pain
			summoned->SetCorpseDelay(0);
		}

		void UpdateAI(uint32 diff) override
		{
			if (_updateTimer <= diff)
			{
				_updateTimer = 0.7 * 1000;
				if (BossCount > 0) {
					summoned = me->SummonCreature(444003, me->GetPosition());
					BossCount--;
				}
				else if (EliteCount > 0) {
					summoned = me->SummonCreature(444002, me->GetPosition());
					EliteCount--;
				}
				else if (StandartCount > 0) {
					summoned = me->SummonCreature(444001, me->GetPosition());
					StandartCount--;
				}
				else {
					me->DespawnOrUnsummon();
				}
				summoned->ToCreature()->Attack(me->FindNearestCreature(444000, 100.0f), true);
			}
			else {
				_updateTimer -= diff;
			}
		}

		void SetData(uint32 /*value*/, uint32 uiValue) override
		{
			GroupMembersCount = uiValue;
		}

	private:
		SummonList _summons;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new SpawnerAI(creature);
	}
};


class npc_caravan : public CreatureScript
{

	bool started = false;

public:
	npc_caravan() : CreatureScript("npc_caravan") { }

	struct CaravanAI : public ScriptedAI
	{
		Group* main_group;
		int phase;
		uint32 GroupMembersCount;
		bool CanStart;
		bool event_done = false;
		uint32 QuestID = 444004;
		bool started = false;;
		std::vector<Player*> Player_list;

		CaravanAI(Creature* creature) : ScriptedAI(creature)
		{
			Initialize();
		}

		void Initialize()
		{
			me->setFaction(1665);
			phase = 0;
			me->setRegeneratingHealth(false);
			//me->SetReactState(REACT_PASSIVE);
		}

		void EnterCombat(Unit* /*victim*/) override
		{
			//me->AttackStop();
		}

		void Reset() override
		{
			Initialize();
		}

		void SetData(uint32 /*value*/, uint32 speedRate) override
		{
			me->SetSpeed(UnitMoveType(0), speedRate, true);
		}

		void DoAction(int32 action) override
		{
			if (!started) {
				Player* player = me->SelectNearestPlayer(100000.0f);
				if (player->GetGroup()) {
					GroupMembersCount = player->GetGroup()->GetMembersCount();
					main_group = player->GetGroup();
					for (GroupReference* itr = player->GetGroup()->GetFirstMember(); itr != NULL; itr = itr->next()) {
						Player_list.push_back(itr->GetSource());
					}
				}
				else {
					GroupMembersCount = 1;
					Player_list.push_back(player);
				}
				started = true;
				me->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
				me->LoadPath(5555557);
				me->GetMotionMaster()->Initialize();
			}
		}

		void MovementInform(uint32 /*type*/, uint32 id) override
		{
			if (!event_done) {
				TempSummon* SpawnerCreature;
				switch (id)
				{
				case 9: //1
					SpawnerCreature = me->SummonCreature(444009, 1994.680786f, 1613.819214f, 85.362480f, 4.634248);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(1);
					break;
				case 17: //2
					//SpawnerCreature = me->SummonCreature(444009, 2075.704590f, 1436.829834f, 70.905060f, 1.524248);
					//SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					//SpawnerCreature->AI()->DoAction(2);
					break;
				case 32: //3
					SpawnerCreature = me->SummonCreature(444009, 2119.201660f, 1427.404053f, 66.642937f, 2.147855);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(1);
					break;
				case 56: ///4
					SpawnerCreature = me->SummonCreature(444009, 2185.767607f, 1539.539297f, 74.05446f, 4.122248);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(1);
					break;
				case 68: //5
					SpawnerCreature = me->SummonCreature(444009, 2179.427002f, 1637.992678f, 84.176186f, 4.197747);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(1);
					break; //6
				case 72:
					SpawnerCreature = me->SummonCreature(444009, 2147.949951f, 1623.080322f, 83.567169f, 0.741552);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(2);
					break; //7
				case 84:
					SpawnerCreature = me->SummonCreature(444009, 2125.2923213f, 1709.136841f, 82.941223f, 4.564084);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(2);
					break;
				case 91: //8
					SpawnerCreature = me->SummonCreature(444009, 2081.692871f, 1748.328369f, 78.996170f, 5.775954);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(2);
					break;
				case 99: //9
					SpawnerCreature = me->SummonCreature(444009, 2108.745117f, 1825.241333f, 83.271492f, 4.432140);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(2);
					break;
				case 127: //10
					SpawnerCreature = me->SummonCreature(444009, 2084.660889f, 1976.606812f, 67.501183f, 3.446759);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(2);
					break;
				case 139: //11
					SpawnerCreature = me->SummonCreature(444009, 2153.147461f, 1964.482422f, 64.314720f, 2.229892);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(1);
					break;
				case 154: //12
					SpawnerCreature = me->SummonCreature(444009, 2192.313965f, 1909.926758f, 73.118744f, 3.239137);
					SpawnerCreature->AI()->SetData(10, GroupMembersCount);
					SpawnerCreature->AI()->DoAction(3);
					break;
				case 159: //13
					me->StopMoving();
					me->LoadPath(5555559);
					//me->GetMotionMaster()->Initialize();
					for (int i = 0; i < Player_list.size(); i++) {
						Player* pl = Player_list[i];
						if (pl)
							pl->CompleteQuest(QuestID);
					};
					me->SetSpeed(UnitMoveType(0), 0, true);
					event_done = true;
					break;
				default:
					break;
				}
			}
		}

		void DamageTaken(Unit* /*done_by*/, uint32 &damage) override
		{
		}

		void UpdateAI(uint32 diff) override
		{
			if (Player_list.size() < 0) {
				me->Respawn(true);
			}
			if (me->IsInCombat()) {
				bool ooc;
				for (int itr = 0; itr < Player_list.size(); itr++) {
					ooc = Player_list[itr]->ToPlayer()->IsInCombat();
					if (ooc) break;
				}
				if (!ooc) {
					me->CombatStop();
				}
			}
		}

		void JustDied(Unit* /*killer*/) override
		{
			for (int itr = 0; itr < Player_list.size(); itr++) {
				if (Player_list[itr])
					Player_list[itr]->ToPlayer()->SetQuestStatus(QuestID, QUEST_STATUS_FAILED, true);
			}
		}

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new CaravanAI(creature);
	}

	bool OnGossipHello(Player* player, Creature* creature) override
	{
		if (!started && player->GetGroup())
		{
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Пошли. Давай доведем тебя до моей мамки", 631, 1);
			player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());

			return true;
		}
		else if (!started && !player->GetGroup()) {
			player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Пошли. Давай доведем тебя до моей мамки", 631, 2);
			player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
			return true;
		}
		return true;

	}

	bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
	{
		player->PlayerTalkClass->ClearMenus();
		player->CLOSE_GOSSIP_MENU();
		creature->AI()->DoAction(1);
		return true;
	}

};

void AddSC_start_event_scr()
{
	new npc_caravan();
	new spawner();
}