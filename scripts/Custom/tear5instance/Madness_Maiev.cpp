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

//////////////////////////////////////////////////////////
///////////
///////////			ИвентМайев.
///////////
//////////////////////////////////////////////////////////
#define PORTALS_COUNT 10;
#define PORTAL_ENTRY 191083;
struct PortalArray
{
	float x, y, z;
};

static PortalArray Portal[] =
{
	{ 355.157959, -28.471437, 30.827631 }, //1
	{ 355.157959, -48.701256, 30.829294 }, //2
	{ 354.726654, -67.393661, 30.829563 }, //3
	{ 354.113556, -110.926053, 32.489877 }, //4
	{ 394.213959, -111.015167, 32.489639 }, //5
	{ 394.906036, -66.302498, 30.829565 }, //6
	{ 394.776917, -47.685364, 30.289565 }, //7
	{ 394.465576, -28.152803, 30.828146 }, //8
	{ 374.390594, -43.933178, 30.828852 }, //9
	{ 374.390594, -71.194710, 30.829519 } //10
};

enum Texts
{
	SAY_INTRO = 100030,
	SAY_INTRO_2,
	SAY_INTRO_3,
	TEXT_OUTRO_1,
	TEXT_OUTRO_2,
};


enum Events
{
	EVENT_CHANGE_PHASE_TO_TRANSFORMATION = 0,
	EVENT_SAVE_TELEPORT = 1, //Телепортация Майев при получении урона
	EVENT_TRY_TO_CHANGE_WEAPON, //
	EVENT_CAST_SPELL_UCOL,
	EVENT_CAST_SPELL_INC_DAMAGE,
	EVENT_CAST_SPELL_DEATH_UDAR,
	EVENT_CAST_SPELL_RIVOK,
	EVENT_CAST_SPELL_DARK_RING,
	EVENT_CAST_SPELL_ICE_DAMAGE,
	EVENT_CAST_SPELL_STEL_DARKNESS,
	EVENT_CAST_SPELL_BURN,
	EVENT_CAST_SPELL_DARK_LANCE,
	EVENT_CAST_SPELL_THOUSAND_SOULS_DARKNESS,
	EVENT_CAST_SPELL_FROSTEN_KOSN,
	EVENT_AURA_KOSN_SKVERNI,
	EVENT_CAST_SPELL_SHADOW_STEP_COMBO,
	EVENT_KILL_LOW_BOLL_TARGET,
	EVENT_ADD_POISON,
};

enum Spells
{
	SPELL_METAMORPHOSES_VISUAL = 24085,  // Визуальный эффект превращения (3)
	SPELL_SPIRIT = 48380, // Логичная стадия анимации - 3 секунды. (1)
	SPELL_SPIRIT_DIES = 48596, //Осторожно - станит. Длительность анимации - 3 секунды (2)
	SPELL_DARKNESS_CAGE = 40647, //Использовать для начала ивента. У нас будет 30 секунд на полное проведение ивента начала.

	SPELL_TELEPORT_VISUAL = 50772, //До начала телепортации использовать этот спел, потом сделать телепортацию
	SPELL_DARK_CLOUD = 46265, //Темное облочко. Визуальный.

	SPELL_STEL_DARKNESS = 63619, // Стелющаяся тьма - 15% дмг на 6 секунд + телепорт //CD 6sec
	SPELL_THOUSAND_SOULS_DARKNESS = 45657, //Тьма тысячи душ //КД - 4 сек - - - - Вторая фаза
	AURA_KOSN_SKVERNI = 72450, // +75% дмг по цели, проклятие, действует 20 секунд, стакается - - - - Вторая фаза
	SPELL_FROSTEN_KOSN = 38240, //-75% маг дмг цели, действует 6 секунд - - - - Вторая фаза

	SPELL_SKVERN_RAGE = 41369, //*20 весь дмг на 10 секунд.. Зло? - - - - Четвертая фаза?
	SPELL_DEATH_PLAGUE = 72865, //Если в радиусе (8м) нет целей - через нное время убивает цель - - - - 3 фаза
	SPELL_DARK_LANCE = 71815, //МНОГО ДМГ - - - - Метоморфоза
	SPELL_BURN = 41960,

	SPELL_ICE_DAMAGE = 70292, // Дмг по процентам от хп, пока не ресторнет фул хп
	SPELL_UCOL = 55104, //Точный укол, урон = сила атаки. Не блокируется/уклоняется/парируетяс
	SPELL_DEATH_UDAR = 17547, //200% от урона, 5 секунд -50% хил.
	SPELL_INC_DAMAGE = 61599, //100% урона + 1000.
	SPELL_RIVOK = 59611, //Рывок. Стан на 2сек.
	SPELL_DARK_RING = 65209, //Кольцо тьмы - - - - Похоже на 45657. Использовать в первой фазе
	SPELL_STUN = 34510, //Оглушение на 4 секунды
};

enum Phases {
	PHASE_NULL = 0, //До начала ивента
	PHASE_INTRO = 1, //Ивент "преображения" Майев
	PHASE_START_FIGHT, //Первая фаза Боя
	PHASE_TRANSFORMATION, //Фаза трансформации Майев в демона (ивент
	PHASE_METAMORPHOSES, //Вторая фаза Боя
	PHASE_MIRROR, //Зеркальное отражение майев
	PHASE_MIRROR_EXPLODE, //Взрыв зеркала - ивент с оглушением и первращением
	PHASE_ENRAGED_FIGHT, //Третья фаза боя
	PHASE_DEATH_WILL, //Четвертая и последняя фаза боя
	PHASE_OUTRO //Ивент ранения, исцеления и "позорного бегства"
};

enum WeaponType {
	WEAPON_GLAIVES = 1,
	WEAPON_MAIEV_HANDS,
};

//enum Emotes {
//	EMOTE_ONESHOT_ROAR = 15
//};

enum ActionList {
	ACTION_START = 0,
	ACTION_MIRROR_DIE,
};

class boss_maiev_in_madness : public CreatureScript {

public:
	boss_maiev_in_madness() : CreatureScript("boss_maiev_in_madness") { }

	struct boss_maiev_in_madnessAI : public BossAI
	{
		boss_maiev_in_madnessAI(Creature* creature) : BossAI(creature, 3)
		{
			Initialize();
		}

		void Initialize() {
			DoCast(420000);
			me->SetMaxHealth(243 * 1041 * 984);
			me->SetHealth(me->GetMaxHealth());
			me->SetMaxPower(POWER_MANA, 93 * 1021);
			me->SetPower(POWER_MANA, me->GetMaxPower(POWER_MANA));
			me->DeMorph();
			phase = PHASE_NULL;
			SetEquipmentSlots(false, 32425);
			spirit_cast = false;
			spirit_dies = false;
			intro_text = false;
			intro_text_2 = false;
			intro_text_3 = false;
			me->setFaction(35); //Friendly
			weapon_type = WEAPON_MAIEV_HANDS;
		}

		void UpdateAI(uint32 diff) override
		{

			Position pos;
			events.Update(diff);
			switch (phase)
			{
			case PHASE_INTRO:
				timer -= diff;
				if (!intro_text) {
					me->SetInCombatWithZone();
					me->SetOrientation(1.570254f); //Поворачиваемся к игрокам
					me->UpdateOrientation(1.570254f);
					Talk(SAY_INTRO);
					me->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
					intro_text = true;
					timer = 15 * IN_MILLISECONDS;
				}
				else if (timer < 0 && !intro_text_2) {
					me->SetOrientation(4.680412f);
					me->UpdateOrientation(4.680412f);
					intro_text_2 = true;
					Talk(SAY_INTRO_2);
					timer = 15 * IN_MILLISECONDS;
					me->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
				}
				else if (timer < 0 && !intro_text_3) {
					me->SetDisplayId(23225); //FelElfHunterFemaleSkinBlack
					me->AddAura(SPELL_DARK_CLOUD, me);
					timer = 15 * IN_MILLISECONDS;
					intro_text_3 = true;
					Talk(SAY_INTRO_3);
				}
				else if (timer < 0) {
					me->setFaction(14); //Agressor;
					phase = PHASE_START_FIGHT;
					me->SetAttackTime(BASE_ATTACK, 1200);
					me->Attack(SelectTarget(SELECT_TARGET_RANDOM, 1, 80.0f), true);

					events.ScheduleEvent(EVENT_CAST_SPELL_UCOL, urand(500, 700));
					//events.ScheduleEvent(EVENT_CAST_SPELL_INC_DAMAGE, urand(600, 1200));
					events.ScheduleEvent(EVENT_CAST_SPELL_DARK_RING, urand(4000, 7000));
					events.ScheduleEvent(EVENT_CAST_SPELL_ICE_DAMAGE, urand(3000, 10000));
					events.ScheduleEvent(EVENT_CAST_SPELL_DEATH_UDAR, urand(600, 1200));
					//events.ScheduleEvent(EVENT_CAST_SPELL_RIVOK, urand(5000, 9000));
					events.ScheduleEvent(EVENT_CAST_SPELL_STEL_DARKNESS, urand(3000, 6000));
				}
				break;
			case PHASE_START_FIGHT:
				DoMeleeAttackIfReady();
				if (me->GetHealthPct() < 85.0f) {
					phase = PHASE_TRANSFORMATION;
				}
				if (!UpdateVictim())
					return;
				break;
			case PHASE_TRANSFORMATION:
				timer -= diff;
				if (!spirit_cast) {
					me->AttackStop();
					events.Reset();
					DoCast(me, SPELL_DARKNESS_CAGE, true);
					aura = me->AddAura(SPELL_SPIRIT_DIES, me);
					timer = 15 * IN_MILLISECONDS;
					spirit_cast = true;
				}
				else if (timer < 0 && !spirit_dies) {
					timer = 1 * IN_MILLISECONDS;
					spirit_dies = true;
				}
				else if (timer < 0) {
					me->RemoveAura(aura);
					DoCast(SPELL_METAMORPHOSES_VISUAL);
					phase = PHASE_METAMORPHOSES;
					for (int i = 0; i < portal_count; i++){
						GameObject* Portal_n = me->SummonGameObject(portal_entry, Portal[i].x, Portal[i].y, Portal[i].z, 0, 0, 0, 0, 0, 0);
						Portals.push_back(Portal_n);
					}
					me->SetDisplayId(25277); //Облик демона
					SaveTeleport();
					AttackStartNoMove(SelectTarget(SELECT_TARGET_FARTHEST, 1, 40.0f));
					events.ScheduleEvent(EVENT_CAST_SPELL_BURN, urand(1200, 2400));
					events.ScheduleEvent(EVENT_CAST_SPELL_FROSTEN_KOSN, urand(6000, 9000));
					events.ScheduleEvent(EVENT_CAST_SPELL_THOUSAND_SOULS_DARKNESS, urand(4000, 7000));
					events.ScheduleEvent(EVENT_CAST_SPELL_DARK_LANCE, urand(4000, 15000));
					events.ScheduleEvent(EVENT_AURA_KOSN_SKVERNI, urand(5000, 12000));
					normal_run = me->GetSpeed(MOVE_RUN);
					me->SetSpeed(MOVE_RUN, 0);
				}
				break;

			case PHASE_METAMORPHOSES:
				//me->GetSpeed(MOVE_RUN);
				//DealDamageTimer -= diff;
				if (!UpdateVictim())
					return;
				if (me->GetHealthPct() < 70.0f) {
					phase = PHASE_MIRROR;
					Copy = me->SummonCreature(211802, me->GetPosition());
					me->AddAura(4309, me);
					me->SetPosition(Position(Portal[9].x, Portal[9].y, Portal[9].z));
					pos = Position(Portal[9].x, Portal[9].y, Portal[9].z, 0);
					me->SendTeleportPacket(pos);

					events.Reset();
					events.ScheduleEvent(EVENT_CAST_SPELL_BURN, urand(1200, 2400));
					events.ScheduleEvent(EVENT_CAST_SPELL_FROSTEN_KOSN, urand(6000, 9000));
					events.ScheduleEvent(EVENT_CAST_SPELL_THOUSAND_SOULS_DARKNESS, urand(4000, 7000));
					events.ScheduleEvent(EVENT_CAST_SPELL_DARK_LANCE, urand(4000, 15000));
					events.ScheduleEvent(EVENT_AURA_KOSN_SKVERNI, urand(5000, 12000));
					//timer = 3 * IN_MILLISECONDS;
				}
			case PHASE_MIRROR:
				if (!UpdateVictim())
					return;
				break;
			case PHASE_MIRROR_EXPLODE:
				me->SetSpeed(MOVE_RUN, normal_run);
				me->RemoveAura(4309);
				me->SetPosition(Position(Portal[9].x, Portal[9].y, Portal[9].z));
				pos = Position(Portal[9].x, Portal[9].y, Portal[9].z, 0);
				me->SendTeleportPacket(pos);
				events.Reset();
				Copy->AI()->DoAction(1);
				me->GetMotionMaster()->MoveKnockbackFrom(me->GetPositionX(), me->GetPositionY(), 30, 5);
				me->SetHealth(me->GetHealth() - me->GetHealth() * 0.05f);
				DoCast(SPELL_METAMORPHOSES_VISUAL);
				me->SetDisplayId(23225); //FelElfHunterFemaleSkinBlack
				phase = PHASE_ENRAGED_FIGHT;

				//SetEquipmentSlots(false, 18582);
				rage_timer = urand(20000, 30000);
				BeingInRage = false;
				events.ScheduleEvent(EVENT_CAST_SPELL_UCOL, urand(600, 1400));
				events.ScheduleEvent(EVENT_CAST_SPELL_DEATH_UDAR, urand(600, 1200));
				events.ScheduleEvent(EVENT_CAST_SPELL_STEL_DARKNESS, urand(3000, 6000));
				events.ScheduleEvent(EVENT_CAST_SPELL_THOUSAND_SOULS_DARKNESS, urand(4000, 7000));
				events.ScheduleEvent(EVENT_CAST_SPELL_DARK_RING, urand(4000, 7000));
				//me->SetInt32Value(UNIT_FIELD_ATTACK_POWER, 480400);

				break;
			case PHASE_ENRAGED_FIGHT:
				if (!UpdateVictim())
					return;
				DoMeleeAttackIfReady();
				rage_timer -= diff;
				if (rage_timer <= 0 && !BeingInRage)  { DoCast(SPELL_SKVERN_RAGE); }

				if (me->GetHealthPct() < 25.0f)
				{
					events.Reset();
					events.ScheduleEvent(EVENT_CAST_SPELL_SHADOW_STEP_COMBO, urand(300, 700));
					events.ScheduleEvent(EVENT_KILL_LOW_BOLL_TARGET, urand(4000, 7000));
					events.ScheduleEvent(EVENT_ADD_POISON, 10000);
					phase = PHASE_DEATH_WILL;
				}
				break;
			case PHASE_DEATH_WILL:
				if (!UpdateVictim())
					return;
				DoMeleeAttackIfReady();
				if (me->GetHealthPct() < 5.0f)
				{
					me->AttackStop();
					me->CombatStop(true);
					me->setFaction(35); //Friendly
					phase = PHASE_OUTRO;
					//DEBUG ONLY
					//me->SetHealth(1);
				}
				break;
			case PHASE_OUTRO:
				timer -= diff;
				if (!outro_text) {
					events.Reset();
					outro_text = true;
					timer = 8000;
					me->Say(TEXT_OUTRO_1);
					me->DeMorph();
					DoCast(62165);
				}
				else if (!outro_text_2 && timer < 0) {
					timer = 8000;
					outro_text_2 = true;
					me->Say(TEXT_OUTRO_2);
				}
				else if (!outro_text_3 && timer < 0) {
					DoCast(41232);
					outro_text_3 = true;
					timer = 500;
				}
				else if (timer < 0)
				{
					me->DeleteThreatList();
					me->SetVisible(false);
					me->FindNearestGameObject(186859, 500.0f)->SetGoState(GOState::GO_STATE_ACTIVE);
					me->SummonGameObject(300002, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0, 0, 0, 0, 0, -1);
					phase = 11;
				}
				break;
			default:
				break;
			}

			while (uint32 eventId = events.ExecuteEvent()) {
				Unit* target;
				switch (eventId)
				{
				case EVENT_CAST_SPELL_UCOL:
					DoCastVictim(SPELL_UCOL, false);
					events.ScheduleEvent(EVENT_CAST_SPELL_UCOL, urand(500, 1400));
					break;
				case EVENT_CAST_SPELL_INC_DAMAGE:
					DoCastVictim(SPELL_INC_DAMAGE, false);
					events.ScheduleEvent(EVENT_CAST_SPELL_INC_DAMAGE, urand(600, 1200));
					break;
				case EVENT_CAST_SPELL_DARK_RING:
					DoCast(SPELL_DARK_RING);
					events.ScheduleEvent(EVENT_CAST_SPELL_DARK_RING, urand(5000, 9000));
					break;
				case EVENT_CAST_SPELL_STEL_DARKNESS:
					target = SelectTarget(SELECT_TARGET_FARTHEST, 1, 40.0f);
					//me->DeleteThreatList();
					me->AddThreat(target, 10000.0f);
					DoCastVictim(SPELL_STEL_DARKNESS);
					events.ScheduleEvent(EVENT_CAST_SPELL_STEL_DARKNESS, urand(3000, 6000));
					break;
				case EVENT_CAST_SPELL_ICE_DAMAGE:
					DoCastVictim(SPELL_ICE_DAMAGE);
					events.ScheduleEvent(EVENT_CAST_SPELL_ICE_DAMAGE, urand(3000, 10000));
					break;
				case EVENT_CAST_SPELL_DEATH_UDAR:
					DoCastVictim(SPELL_DEATH_UDAR);
					events.ScheduleEvent(EVENT_CAST_SPELL_DEATH_UDAR, urand(600, 1200));
					break;

					//PHASE = 2
				case EVENT_CAST_SPELL_BURN:
					DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), SPELL_BURN, true);
					events.ScheduleEvent(EVENT_CAST_SPELL_BURN, urand(1200, 2400));
					break;
				case EVENT_CAST_SPELL_FROSTEN_KOSN:
					DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), SPELL_FROSTEN_KOSN);
					events.ScheduleEvent(EVENT_CAST_SPELL_FROSTEN_KOSN, urand(6000, 9000));
					break;
				case EVENT_CAST_SPELL_THOUSAND_SOULS_DARKNESS:
					DoCast(SPELL_THOUSAND_SOULS_DARKNESS);
					events.ScheduleEvent(EVENT_CAST_SPELL_THOUSAND_SOULS_DARKNESS, urand(4000, 7000));
					break;
				case EVENT_CAST_SPELL_DARK_LANCE:
					DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), SPELL_DARK_LANCE, true);
					DoCastVictim(SPELL_DARK_LANCE, true);
					DoCast(SelectTarget(SELECT_TARGET_FARTHEST, 1, 40.0f), SPELL_DARK_LANCE, true);
					events.ScheduleEvent(EVENT_CAST_SPELL_DARK_LANCE, urand(4000, 15000));
					break;
				case EVENT_AURA_KOSN_SKVERNI:
					me->AddAura(AURA_KOSN_SKVERNI, me->GetVictim());
					me->AddAura(AURA_KOSN_SKVERNI, SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f));
					events.ScheduleEvent(EVENT_AURA_KOSN_SKVERNI, urand(5000, 12000));
					break;
					//3

					//4
				case EVENT_CAST_SPELL_SHADOW_STEP_COMBO:
					target = SelectTarget(SELECT_TARGET_FARTHEST, 1, 80.0f);
					//me->DeleteThreatList();
					me->AddThreat(target, 10000.0f);
					DoCast(target, SPELL_STEL_DARKNESS, true);
					DoCast(target, SPELL_UCOL);
					DoCast(target, SPELL_DEATH_UDAR);
					events.ScheduleEvent(EVENT_CAST_SPELL_SHADOW_STEP_COMBO, urand(300, 500));
					break;
				case EVENT_KILL_LOW_BOLL_TARGET:
					target = SelectTarget(SELECT_TARGET_FARTHEST, 1, 40.0f);
					//me->DeleteThreatList();
					me->AddThreat(target, 10000.0f);
					//while (target->GetHealthPct() > 60) { target = SelectTarget(SELECT_TARGET_RANDOM, 1, 80.0f); }
					DoCast(target, SPELL_STEL_DARKNESS, true);
					DoCast(target, SPELL_ICE_DAMAGE);
					DoCast(target, SPELL_DEATH_PLAGUE);
					events.ScheduleEvent(EVENT_KILL_LOW_BOLL_TARGET, urand(5000, 9000));
				case EVENT_ADD_POISON:
					me->AddAura(56605, me);
					events.ScheduleEvent(EVENT_ADD_POISON, 4000);
					break;
				default:
					break;
				}
			}
		}

		//void JustDied(Unit* /*killer*/) override
		//{ }

		void KilledUnit(Unit* victim) override
		{
			if (victim->IsControlledByPlayer())
			{
				BeingInRage = false;
				rage_timer = urand(0, 20000);
			}
		}

		void DoAction(int32 action) override
		{
			switch (action) {
			case ACTION_START:
				phase = PHASE_INTRO;
				break;
			case ACTION_MIRROR_DIE:
				phase = PHASE_MIRROR_EXPLODE;
				break;
			}
		}

		void MoveInLineOfSight(Unit* who)
		{
			if (who->GetTypeId() == TYPEID_PLAYER &&
				!who->ToPlayer()->IsGameMaster() &&
				me->IsWithinDistInMap(who, 30.0f) &&
				phase == PHASE_NULL) {
				DoAction(ACTION_START);
			}
		}

		void DamageTaken(Unit* done_by, uint32 &damage) override
		{
			switch (phase) {
			case PHASE_METAMORPHOSES:
				DamageCount++;
				if (DamageCount >= 100)
				{
					SaveTeleport();
					DamageCount = 0;
				}
				break;
			case PHASE_MIRROR:
				damage = 0;
				me->SetHealth(me->GetHealth()); //PreventVisualBug;
				break;
			default: break;
			}
		}

		void SaveTeleport() {
			DoCast(SPELL_TELEPORT_VISUAL);
			int i = urand(0, 9);
			while (i == last_teleport) i = urand(0, 9);
			me->SetPosition(Portal[i].x, Portal[i].y, Portal[i].z, 0);
			Position pos = Position(Portal[i].x, Portal[i].y, Portal[i].z, 0);
			me->SendTeleportPacket(pos);
			last_teleport = i;
			me->AttackStop();
			me->SendMeleeAttackStop(me->SelectVictim());
			me->AddThreat(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), 10000.0f);
		}

		//void EnterCombat(Unit* /*victim*/) override
		//{ }

		/*void ExecuteEvent(uint32 eventId) override
		{

		}*/


		void Reset() override
		{
			_Reset();
			if (Portals.size() > 0) {
				for (std::vector<GameObject*>::const_iterator iter = Portals.begin(); iter != Portals.end(); ++iter) { (*iter)->Delete(); }
				Portals.clear();
			}
			Initialize();
			me->SetPosition(me->GetHomePosition());
			Position pos = me->GetPosition();
			me->SendTeleportPacket(pos);

		}

		void EnterEvadeMode() override
		{
			Reset();
		}

		void SetData(uint32 value, uint32 uiValue) override
		{
			if (value == 1 && uiValue == 1) {
				DoAction(ACTION_START);
			}
			if (value == 2 && uiValue == 2) {
				me->SetHealth(me->GetMaxHealth()*0.8);
			}

			if (value == 3 && uiValue == 3) {
				DoAction(ACTION_MIRROR_DIE);
			}

			if (value == 4 && uiValue == 4) {
				phase = PHASE_ENRAGED_FIGHT;
				SetEquipmentSlots(false, 18582);
				rage_timer = urand(0, 30000);
				events.ScheduleEvent(EVENT_CAST_SPELL_UCOL, urand(600, 1400));
				events.ScheduleEvent(EVENT_CAST_SPELL_DEATH_UDAR, urand(600, 1200));
				events.ScheduleEvent(EVENT_CAST_SPELL_RIVOK, urand(5000, 9000));
				events.ScheduleEvent(EVENT_CAST_SPELL_STEL_DARKNESS, urand(3000, 6000));

			}
		}

	private:
		uint8 phase;
		Aura* aura;
		Creature* Copy;
		uint32 DamageCount = 0, rage_timer, normal_run;
		std::vector<GameObject*> Portals;
		int portal_count = PORTALS_COUNT;
		int portal_entry = PORTAL_ENTRY;
		int last_teleport = -1;
		int timer, weapon_type;
		bool spirit_cast, spirit_dies, intro_text, intro_text_2, intro_text_3, outro_text, outro_text_2, outro_text_3, BeingInRage = false;

	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new boss_maiev_in_madnessAI(creature);
	}
};

class boss_maiev_in_madness_shadow : public CreatureScript
{
public:
	boss_maiev_in_madness_shadow() : CreatureScript("boss_maiev_in_madness_shadow") { }

	struct boss_maiev_in_madness_shadowAI : public ScriptedAI //npc 32353
	{
		boss_maiev_in_madness_shadowAI(Creature* creature) : ScriptedAI(creature)
		{
			Initialize();
		}

		EventMap events;

		void Reset() override
		{
			//
		}

		void Initialize() {
			me->setFaction(14); //Agressor;
			me->SetInt32Value(UNIT_FIELD_ATTACK_POWER, 80400);
			me->SetMaxHealth(243 * 1041 * 984);
			me->SetHealth(me->GetMaxHealth());
			me->SetMaxPower(POWER_MANA, 93 * 1021);
			me->SetPower(POWER_MANA, me->GetMaxPower(POWER_MANA));

			events.ScheduleEvent(EVENT_CAST_SPELL_BURN, urand(1200, 2400));
			events.ScheduleEvent(EVENT_CAST_SPELL_FROSTEN_KOSN, urand(6000, 9000));
			events.ScheduleEvent(EVENT_CAST_SPELL_THOUSAND_SOULS_DARKNESS, urand(4000, 7000));
			events.ScheduleEvent(EVENT_CAST_SPELL_DARK_LANCE, urand(4000, 15000));
			events.ScheduleEvent(EVENT_AURA_KOSN_SKVERNI, urand(5000, 12000));
		}

		void DoAction(int32 action) override {
			if (action = 1)
			{
				DoCast(me, 72500, true);
				me->DespawnOrUnsummon();
			}
		}

		void EnterCombat(Unit* /*who*/) override
		{
		}

		void UpdateAI(uint32 diff) override
		{
			events.Update(diff);
			while (uint32 eventId = events.ExecuteEvent()) {
				Unit* target;
				switch (eventId)
				{
					//PHASE = 2
				case EVENT_CAST_SPELL_BURN:
					//me->tal
					DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), SPELL_BURN, true);
					events.ScheduleEvent(EVENT_CAST_SPELL_BURN, urand(1200, 2400));
					break;
				case EVENT_CAST_SPELL_FROSTEN_KOSN:
					DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), SPELL_FROSTEN_KOSN);
					events.ScheduleEvent(EVENT_CAST_SPELL_FROSTEN_KOSN, urand(6000, 9000));
					break;
				case EVENT_CAST_SPELL_THOUSAND_SOULS_DARKNESS:
					DoCast(SPELL_THOUSAND_SOULS_DARKNESS);
					events.ScheduleEvent(EVENT_CAST_SPELL_THOUSAND_SOULS_DARKNESS, urand(4000, 7000));
					break;
				case EVENT_CAST_SPELL_DARK_LANCE:
					DoCast(SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f), SPELL_DARK_LANCE, true);
					DoCastVictim(SPELL_DARK_LANCE, true);
					DoCast(SelectTarget(SELECT_TARGET_FARTHEST, 1, 40.0f), SPELL_DARK_LANCE, true);
					events.ScheduleEvent(EVENT_CAST_SPELL_DARK_LANCE, urand(4000, 15000));
					break;
				case EVENT_AURA_KOSN_SKVERNI:
					me->AddAura(AURA_KOSN_SKVERNI, me->GetVictim());
					me->AddAura(AURA_KOSN_SKVERNI, SelectTarget(SELECT_TARGET_RANDOM, 1, 40.0f));
					events.ScheduleEvent(EVENT_AURA_KOSN_SKVERNI, urand(5000, 12000));
					break;
				}
			};
		}

		void DamageTaken(Unit* done_by, uint32 &damage) override
		{
			damage *= 5;

			if (damage >= me->GetHealth()) {
				damage = me->GetHealth() - 1;
				if (!not_started)
				{
					events.Reset();
					me->SetPosition(373.231262f, -41.709305f, 30.830444f, 4.693759f);
					Position pos = Position(Position(373.231262f, -41.709305f, 30.830444f, 4.693759f));
					me->SendTeleportPacket(pos);
					Creature* owner = me->FindNearestCreature(211801, 100.0f);
					DoCast(owner, 40228);
					owner->AI()->DoAction(1);

					not_started = true;
				}

			}
		}
		bool not_started = false;
	};

	CreatureAI* GetAI(Creature* creature) const override
	{
		return new boss_maiev_in_madness_shadowAI(creature);
	}
};

class npc_valeera_sanguinar : public CreatureScript
{
public:
	npc_valeera_sanguinar() : CreatureScript("npc_valeera_sanguinar") { }

	struct npc_valeera_sanguinarAI : public ScriptedAI //npc 32353
	{
		npc_valeera_sanguinarAI(Creature* creature) : ScriptedAI(creature)
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

		void EnterCombat(Unit* /*who*/) override
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
		return new npc_valeera_sanguinarAI(creature);
	}
};

void AddSC_maiev_in_madness()
{
	new boss_maiev_in_madness();
	new boss_maiev_in_madness_shadow();
	new npc_valeera_sanguinar();
}