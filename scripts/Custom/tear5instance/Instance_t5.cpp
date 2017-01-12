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

class DeathSphere : public CreatureScript
{
public:
	DeathSphere() : CreatureScript("npc_deathsphere") { }

	struct DeathSphereAI : public ScriptedAI
	{
		DeathSphereAI(Creature* creature) : ScriptedAI(creature) { }

		void UpdateAI(uint32 diff) override
		{
			if (!me->HasAura(63537)) me->AddAura(63537, me);
		}

		void MoveInLineOfSight(Unit* who)
		{ 
			if (me->IsWithinDistInMap(who, 1.0f) && who->IsControlledByPlayer()) {
				DoCast(who, 33331, true);
			}
		}
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new DeathSphereAI(creature);
	}
};

class BodyOnTheFloor : public CreatureScript
{
public:
	BodyOnTheFloor() : CreatureScript("npc_bodyonthefloor") { }

	struct BodyOnTheFloorAI : public ScriptedAI
	{
		BodyOnTheFloorAI(Creature* creature) : ScriptedAI(creature) {
			if (urand(0, 1) == 1) { random_setted_true = true; }
		}

		void MoveInLineOfSight(Unit* who)
		{
			if (me->IsWithinDistInMap(who, 1.0f) && who->IsControlledByPlayer() && random_setted_true) {
				DoCast(who, 33331, true);
			}
		}
	private:
		bool random_setted_true;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new BodyOnTheFloorAI(creature);
	}
};

void AddSC_instance_t5()
{
	new DeathSphere();
	new BodyOnTheFloor();
} 
