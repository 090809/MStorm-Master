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

/*
 * Scripts for spells with SPELLFAMILY_MAGE and SPELLFAMILY_GENERIC spells used by mage players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_mage_".
 */

#include "Unit.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Containers.h"
#include "SpellHistory.h"

enum MysticSpells
{
	SPELL_MYSTIC_DAIMOS_SIGIL = 300007,
	SPELL_MYSTIC_MANTRA_STAMINA,
	SPELL_MYSTIC_MANTRA_SPEED,
	SPELL_MYSTIC_MANTRA_DEFENSE,

	SPELL_MYSTIC_MYST_POWER = 300015,

	SPELL_MYSTIC_MANTRA		  = 300011,
	SPELL_MYSTIC_MANTRA_COOLDOWN,
	SPELL_MYSTIC_MANTRA_SPIRIT = 300301,

	SPELL_MYSTIC_PRISMA_PAS = 300173,
	SPELL_MYSTIC_PRISMA_ACT
};

uint32 MysicMantraUsesSpells[] = {  SPELL_MYSTIC_MANTRA_STAMINA, 
									SPELL_MYSTIC_MANTRA_SPEED, 
									SPELL_MYSTIC_MANTRA_DEFENSE,
									300140,					//Клеймор Исполина
									300146};				//Метка мистицизма

#define MYSTIC_USED_MANTRA_SPELLS 5

enum MysticSpellIcons
{
};

bool MantraCooldown(Unit* caster) {
	if (!caster->HasAura(SPELL_MYSTIC_MANTRA))
		return false;
	caster->RemoveAura(SPELL_MYSTIC_MANTRA);
	caster->CastSpell(caster, SPELL_MYSTIC_MANTRA_COOLDOWN, TRIGGERED_FULL_MASK);
	return true;
}

void MantraRemCooldown(Unit* caster) {
	if (caster->HasAura(SPELL_MYSTIC_MANTRA_COOLDOWN))
		caster->RemoveAura(SPELL_MYSTIC_MANTRA_COOLDOWN);
	if (!caster->HasAura(SPELL_MYSTIC_MANTRA))
		caster->CastSpell(caster, SPELL_MYSTIC_MANTRA, TRIGGERED_FULL_MASK);
}

class spell_myst_daimos_sigil : public SpellScriptLoader
{
public:
	spell_myst_daimos_sigil() : SpellScriptLoader("spell_myst_daimos_sigil") { }

	class spell_myst_daimos_sigil_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_myst_daimos_sigil_AuraScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_MYSTIC_DAIMOS_SIGIL))
				return false;
			return true;
		}

		void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
		{
			if (Unit* target = GetUnitOwner())
				/*if (target->HasAura(SPELL_MYSTIC_DAIMOS_SIGIL) || amount > 100)
					amount = int32(target->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ARCANE));
				else */
					amount = int32(target->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ARCANE) * amount / 100);
			canBeRecalculated = false;
		}

		void Register() override
		{
			DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_myst_daimos_sigil_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_DAMAGE_DONE);
			DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_myst_daimos_sigil_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_HEALING_DONE);
		}
	};

	class spell_myst_daimos_sigil_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_daimos_sigil_SpellScript);

		void RemoveIfNeed()
		{
			if (Unit* target = GetHitUnit())
				if (target->HasAura(GetSpellInfo()->Id))
					target->RemoveAura(GetSpellInfo()->Id);
		}

		void Register() override
		{
			BeforeHit += SpellHitFn(spell_myst_daimos_sigil_SpellScript::RemoveIfNeed);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_daimos_sigil_SpellScript();
	}

	AuraScript* GetAuraScript() const override
	{
		return new spell_myst_daimos_sigil_AuraScript();
	}
};

class spell_myst_stam_mantra : public SpellScriptLoader
{
public:
	spell_myst_stam_mantra() : SpellScriptLoader("spell_myst_stam_mantra") { }

	class spell_myst_stam_mantra_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_stam_mantra_SpellScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_MYSTIC_MANTRA_STAMINA))
				return false;
			return true;
		}

		SpellCastResult CheckCast()
		{
			Unit* caster = GetCaster();
			if (!MantraCooldown(caster))
				return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
			return SPELL_CAST_OK;
		}

		void Register() override
		{
			OnCheckCast += SpellCheckCastFn(spell_myst_stam_mantra_SpellScript::CheckCast);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_stam_mantra_SpellScript();
	}
};

class spell_myst_mantra : public SpellScriptLoader
{
public:
	spell_myst_mantra() : SpellScriptLoader("spell_myst_mantra") { }

	class spell_myst_mantra_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_mantra_SpellScript);

		SpellCastResult CheckCast()
		{
			Unit* caster = GetCaster();
			if (!MantraCooldown(caster))
				return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
			return SPELL_CAST_OK;
		}

		void Register() override
		{
			OnCheckCast += SpellCheckCastFn(spell_myst_mantra_SpellScript::CheckCast);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_mantra_SpellScript();
	}
};

class spell_myst_mantra_speed : public SpellScriptLoader
{
public:
	spell_myst_mantra_speed() : SpellScriptLoader("spell_myst_mantra_speed") { }

	class spell_myst_mantra_speed_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_mantra_speed_SpellScript);

		SpellCastResult CheckCast()
		{
			Unit* caster = GetCaster();
			if (!MantraCooldown(caster))
				return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
			return SPELL_CAST_OK;
		}

		void _AfterCast()
		{
			if (GetCaster()->GetAuraCount(SPELL_MYSTIC_MANTRA_SPEED) >= 2 && GetCaster()->HasAura(SPELL_MYSTIC_PRISMA_PAS))
			{
				GetCaster()->AddAura(SPELL_MYSTIC_PRISMA_ACT, GetCaster());
			}
		}

		void Register() override
		{
			OnCheckCast += SpellCheckCastFn(spell_myst_mantra_speed_SpellScript::CheckCast);
			AfterCast += SpellCastFn(spell_myst_mantra_speed_SpellScript::_AfterCast);

		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_mantra_speed_SpellScript();
	}
};

class spell_myst_mantra_cond : public SpellScriptLoader
{
public:
	spell_myst_mantra_cond() : SpellScriptLoader("spell_myst_mantra_cond") { }

	class spell_myst_mantra_cond_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_myst_mantra_cond_AuraScript);

		void RemoveEffect(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
		{
			Unit* target = GetTarget();
			target->AddAura(SPELL_MYSTIC_MANTRA, target);
		}

		void Register() override
		{
			OnEffectRemove += AuraEffectRemoveFn(spell_myst_mantra_cond_AuraScript::RemoveEffect, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_myst_mantra_cond_AuraScript();
	}
};

class spell_myst_mantra_hard_reset : public SpellScriptLoader
{
public:
	spell_myst_mantra_hard_reset() : SpellScriptLoader("spell_myst_mantra_hard_reset") { }

	class spell_myst_mantra_hard_reset_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_mantra_hard_reset_SpellScript);

		SpellCastResult CheckCast()
		{
			Unit* caster = GetCaster();
			if (caster->HasAura(SPELL_MYSTIC_MANTRA))
				return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

			caster->RemoveAura(SPELL_MYSTIC_MANTRA_COOLDOWN);
			caster->AddAura(SPELL_MYSTIC_MANTRA, caster);
			return SPELL_CAST_OK;
		}

		void Register() override
		{
			OnCheckCast += SpellCheckCastFn(spell_myst_mantra_hard_reset_SpellScript::CheckCast);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_mantra_hard_reset_SpellScript();
	}
};

class spell_myst_anomalyc : public SpellScriptLoader
{
public:
	spell_myst_anomalyc() : SpellScriptLoader("spell_myst_anomalyc") { }

	class spell_myst_anomalyc_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_anomalyc_SpellScript);

		SpellCastResult CheckCast()
		{
			dest = GetExplTargetDest();
			if (GetCaster()->GetPositionZ() - dest->GetPositionZ() < -5.0f)
				return SPELL_FAILED_OUT_OF_RANGE;

			orient = GetCaster()->GetOrientation();

			return SPELL_CAST_OK;
		}

		void HandleScript(SpellEffIndex effIndex)
		{
			Unit* caster = GetHitUnit();

			if (caster->HasSpell(300273))
			{
				caster->CastSpell(caster, 300274);
				caster->CastSpell(caster, 30918);
			}

			float x, y, z, o;
			dest->GetPosition(x, y, z, o);
			Unit* victim = caster->GetVictim();
			caster->NearTeleportTo(x, y, z, orient);
			
			if (victim)
				caster->Attack(victim, true);

			PreventHitDefaultEffect(effIndex);
		}

		void Register() override
		{
			OnCheckCast += SpellCheckCastFn(spell_myst_anomalyc_SpellScript::CheckCast);
			OnEffectHitTarget += SpellEffectFn(spell_myst_anomalyc_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_TELEPORT_UNITS);
		}

	private:
		float orient;
		WorldLocation const* dest;
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_anomalyc_SpellScript();
	}
};

class spell_myst_return : public SpellScriptLoader
{
public:
	spell_myst_return() : SpellScriptLoader("spell_myst_return") { }

	class spell_myst_return_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_return_SpellScript);

		SpellCastResult CheckCast()
		{
			if (!GetCaster()->HasAura(300274) || !GetCaster()->GetGameObject(300274))
				return SPELL_FAILED_TARGET_AURASTATE;

			return SPELL_CAST_OK;
		}

		void HandleScript(SpellEffIndex effIndex)
		{
			float x, y, z, o;
			GetCaster()->GetGameObject(300274)->GetPosition(x, y, z, o);
			Unit* caster = GetHitUnit();
			caster->NearTeleportTo(x, y, z, orient);

			caster->RemoveAura(300274);
			caster->RemoveGameObject(300013, true);
			caster->CastSpell(caster, 30918);

			PreventHitDefaultEffect(effIndex);
		}

		void Register() override
		{
			OnCheckCast += SpellCheckCastFn(spell_myst_return_SpellScript::CheckCast);
			OnEffectHitTarget += SpellEffectFn(spell_myst_return_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
		}

	private:
		float orient;
		WorldLocation const* dest;
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_return_SpellScript();
	}
};

class spell_myst_myst_power : public SpellScriptLoader
{
public:
	spell_myst_myst_power() : SpellScriptLoader("spell_myst_myst_power") { }

	class spell_myst_myst_power_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_myst_myst_power_AuraScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_MYSTIC_MYST_POWER))
				return false;
			return true;
		}

		void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
		{
			PreventDefaultAction();

			float ap = GetTarget()->GetTotalAttackPowerValue(BASE_ATTACK);
			int32 arcane = GetTarget()->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ARCANE);
			arcane += eventInfo.GetProcTarget()->SpellBaseDamageBonusTaken(SPELL_SCHOOL_MASK_ARCANE);
			int32 bp = int32((0.0088f * arcane) * GetTarget()->GetAttackTime(BASE_ATTACK) / 1000);
			GetTarget()->CastCustomSpell(SPELL_MYSTIC_MYST_POWER+1, SPELLVALUE_BASE_POINT0, bp, eventInfo.GetProcTarget(), true, NULL, aurEff);
		}

		void Register() override
		{
			OnEffectProc += AuraEffectProcFn(spell_myst_myst_power_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_myst_myst_power_AuraScript();
	}
};

class spell_myst_orders_secret : public SpellScriptLoader
{
public:
	spell_myst_orders_secret() : SpellScriptLoader("spell_myst_orders_secret") { }

	class spell_myst_orders_secret_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_myst_orders_secret_AuraScript);

	public:
		spell_myst_orders_secret_AuraScript()
		{
			absorbChance = 0;
		}

	private:
		uint32 absorbChance;

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(300157))
				return false;
			return true;
		}

		bool Load() override
		{
			absorbChance = GetSpellInfo()->Effects[EFFECT_0].CalcValue();
			return GetUnitOwner()->GetTypeId() == TYPEID_PLAYER;
		}

		void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
		{
			// Set absorbtion amount to unlimited
			amount = -1;
		}

		void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
		{
			Player* target = GetTarget()->ToPlayer();
			if (dmgInfo.GetDamage() < target->GetHealth()
				|| target->GetSpellHistory()->HasCooldown(300157)
				|| !roll_chance_i(absorbChance))
				return;

			target->CastSpell(target, 300005, true);
			target->GetSpellHistory()->AddCooldown(300157, 0, std::chrono::minutes(2));
			absorbAmount = dmgInfo.GetDamage();
			if (target->HasAura(300158)) {
				target->HealBySpell(target, sSpellMgr->GetSpellInfo(300158), uint32(target->GetMaxHealth()*0.03f));
			}
			else if (target->HasAura(300159)) {
				target->HealBySpell(target, sSpellMgr->GetSpellInfo(300159), uint32(target->GetMaxHealth()*0.06f));
			}
			else if (target->HasAura(300160)) {
				target->HealBySpell(target, sSpellMgr->GetSpellInfo(300160), uint32(target->GetMaxHealth()*0.09f));
			}
		}

		void Register() override
		{
			DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_myst_orders_secret_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
			OnEffectAbsorb += AuraEffectAbsorbFn(spell_myst_orders_secret_AuraScript::Absorb, EFFECT_0);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_myst_orders_secret_AuraScript();
	}
};

class spell_myst_dark_connect : public SpellScriptLoader
{
public:
	spell_myst_dark_connect() : SpellScriptLoader("spell_myst_dark_connect") { }

	class spell_myst_dark_connect_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_dark_connect_SpellScript);

		void HandleScript(SpellEffIndex effIndex)
		{
			uint32 heal_damage = uint32(GetCaster()->GetMaxHealth() * 0.2f);
			GetCaster()->DealDamage(GetCaster(), heal_damage);
			GetCaster()->HealBySpell(GetHitUnit(), GetSpellInfo(), heal_damage);
		}

		SpellCastResult CheckCast()
		{
			if (GetCaster()->GetMaxHealth() * 0.35f >= GetCaster()->GetHealth())
				return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
			return SPELL_CAST_OK;
		}

		void Register() override
		{
			OnCheckCast += SpellCheckCastFn(spell_myst_dark_connect_SpellScript::CheckCast);
			OnEffectHitTarget += SpellEffectFn(spell_myst_dark_connect_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_dark_connect_SpellScript();
	}
};

class spell_myst_deceit : public SpellScriptLoader
{
public:
	spell_myst_deceit() : SpellScriptLoader("spell_myst_deceit") { }

	class spell_myst_myst_power_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_myst_myst_power_AuraScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(300001))
				return false;
			return true;
		}

		void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
		{
			GetCaster()->CastSpell(eventInfo.GetDamageInfo()->GetVictim(), eventInfo.GetSpellInfo(), TRIGGERED_FULL_MASK);
		}

		void Register() override
		{
			OnEffectProc += AuraEffectProcFn(spell_myst_myst_power_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_myst_myst_power_AuraScript();
	}
};

class spell_myst_replace : public SpellScriptLoader
{
public:
	spell_myst_replace() : SpellScriptLoader("spell_myst_replace") { }

	class spell_myst_replace_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_replace_SpellScript);

		void HandleScript(SpellEffIndex effIndex)
		{
			Unit* target = GetHitUnit();
			Position dest = target->GetPosition();

			float x, y, z, o;
			x = dest.GetPositionX();
			y = dest.GetPositionY();
			z = dest.GetPositionZ();
			o = dest.GetOrientation();

			target->NearTeleportTo(GetCaster()->GetPositionX(), GetCaster()->GetPositionY(), GetCaster()->GetPositionZ(), GetCaster()->GetOrientation());
			GetCaster()->NearTeleportTo(x, y, z, o);

			PreventHitDefaultEffect(effIndex);
		}

		void Register() override
		{
			OnEffectHitTarget += SpellEffectFn(spell_myst_replace_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_DUMMY);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_replace_SpellScript();
	}
};

class spell_myst_mantra_spirit : public SpellScriptLoader
{
public:
	spell_myst_mantra_spirit() : SpellScriptLoader("spell_myst_mantra_spirit") { }

	class spell_myst_mantra_spirit_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_myst_mantra_spirit_AuraScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_MYSTIC_MANTRA_SPIRIT))
				return false;
			return true;
		}

		void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
		{
			if (Unit* target = GetUnitOwner())
				amount = int32(target->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ARCANE) * amount / 100);
			canBeRecalculated = false;
		}

		void Register() override
		{
			DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_myst_mantra_spirit_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_DAMAGE_DONE);
			DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_myst_mantra_spirit_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_HEALING_DONE);
		}
	};

	class spell_myst_mantra_spirit_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_mantra_spirit_SpellScript);

		void RemoveIfNeed()
		{
			if (Unit* target = GetHitUnit())
				if (target->HasAura(SPELL_MYSTIC_MANTRA_SPIRIT))
					target->RemoveAura(SPELL_MYSTIC_MANTRA_SPIRIT);
		}

		SpellCastResult CheckCast()
		{
			Unit* caster = GetCaster();
			if (!MantraCooldown(caster))
				return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
			return SPELL_CAST_OK;
		}

		void Register() override
		{
			BeforeHit += SpellHitFn(spell_myst_mantra_spirit_SpellScript::RemoveIfNeed);
			OnCheckCast += SpellCheckCastFn(spell_myst_mantra_spirit_SpellScript::CheckCast);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_mantra_spirit_SpellScript();
	}

	AuraScript* GetAuraScript() const override
	{
		return new spell_myst_mantra_spirit_AuraScript();
	}
};

class MysticLogIn : public PlayerScript
{
public:
	MysticLogIn() : PlayerScript("MysticLogIn")
	{ }

	void OnLogin(Player* player, bool /*firstLogin*/) override
	{
		if (player->getClass() == CLASS_MYSTIC)
			MantraRemCooldown(player);
	}
};

enum DarkBlood {
	SPELL_DARK_BLOOD = 300003,
	SPELL_DARK_BLOOD_COPY = 300020
};

class spell_myst_dark_blood : public SpellScriptLoader
{
public:
	spell_myst_dark_blood() : SpellScriptLoader("spell_myst_dark_blood") { }

	class spell_myst_dark_blood_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_myst_dark_blood_AuraScript);

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(SPELL_DARK_BLOOD) 
			 || !sSpellMgr->GetSpellInfo(SPELL_DARK_BLOOD_COPY))
				return false;
			return true;
		}

		void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
		{
			switch (GetSpellInfo()->Id)
			{
			case SPELL_DARK_BLOOD:
				if (GetTarget()->HasAura(SPELL_DARK_BLOOD_COPY, GetCasterGUID()))
					GetTarget()->RemoveAura(SPELL_DARK_BLOOD_COPY, GetCasterGUID());
				break;
			case SPELL_DARK_BLOOD_COPY:
				if (GetTarget()->HasAura(SPELL_DARK_BLOOD, GetCasterGUID()))
					GetTarget()->RemoveAura(SPELL_DARK_BLOOD, GetCasterGUID());
				break;
			}
		}

		void Register() override
		{
			OnEffectApply += AuraEffectApplyFn(spell_myst_dark_blood_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_myst_dark_blood_AuraScript();
	}
};

class spell_myst_unsee : public SpellScriptLoader
{
public:
	spell_myst_unsee() : SpellScriptLoader("spell_myst_unsee") { }

	class spell_myst_unsee_SpellScript : public SpellScript
	{
		PrepareSpellScript(spell_myst_unsee_SpellScript);

		bool Load() override
		{
			return GetCaster()->GetTypeId() == TYPEID_PLAYER;
		}

		void HandleDummy(SpellEffIndex /*effIndex*/)
		{
			Unit* caster = GetCaster();
			caster->GetSpellHistory()->ResetCooldowns([caster](SpellHistory::CooldownStorageType::iterator itr) -> bool
			{
				SpellInfo const* spellInfo = sSpellMgr->EnsureSpellInfo(itr->first);
				if (spellInfo->SpellFamilyName != SPELLFAMILY_MYSTIC)
					return false;
				return (spellInfo->SpellFamilyFlags[0] & 0x41)
					|| (spellInfo->SpellFamilyFlags[1] & 0xA6);
			}, true);
		}

		void Register() override
		{
			OnEffectHitTarget += SpellEffectFn(spell_myst_unsee_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
		}
	};

	SpellScript* GetSpellScript() const override
	{
		return new spell_myst_unsee_SpellScript();
	}
};

struct HittedBy_Time
{
	HittedBy_Time(Unit* un, int16 c)
	{
		hitted_by = un;
		count = c;
	}
	Unit* hitted_by;
	int16 count;
};

class spell_myst_daimos_shield : public SpellScriptLoader
{
public:
	spell_myst_daimos_shield() : SpellScriptLoader("spell_myst_daimos_shield") { }

	class spell_myst_daimos_shield_AuraScript : public AuraScript
	{
		PrepareAuraScript(spell_myst_daimos_shield_AuraScript);

	public:
		spell_myst_daimos_shield_AuraScript() {
		}

	private:

		bool Validate(SpellInfo const* /*spellInfo*/) override
		{
			if (!sSpellMgr->GetSpellInfo(300257))
				return false;
			return true;
		}

		bool Load() override
		{
			return GetUnitOwner()->GetTypeId() == TYPEID_PLAYER;
		}

		void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
		{
			hitted_by.clear();
			Unit* caster = GetCaster();
			// Set absorbtion amount to unlimited
			amount = -1;
			for (Unit::AuraApplicationMap::iterator iter = caster->GetAppliedAuras().begin(); iter != caster->GetAppliedAuras().end();)
			{
				AuraApplication const* aurApp = iter->second;
				Aura const* aura = aurApp->GetBase();
				if (!aura->GetSpellInfo()->HasAttribute(SPELL_ATTR4_UNK21)
					&& !aura->IsPassive()
					&& !aurApp->IsPositive()
					&& !aura->GetSpellInfo()->HasAttribute(SPELL_ATTR3_DEATH_PERSISTENT))
					caster->RemoveAura(iter);
				else
					++iter;
			}
		}

		void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
		{
			Player* target = GetTarget()->ToPlayer();
			absorbAmount = dmgInfo.GetDamage();

			if (absorbAmount < 100000)
				return;

			for (uint16 i = 0; i < hitted_by.size(); i++)
				if (dmgInfo.GetAttacker() == hitted_by[i].hitted_by)
				{
					if (hitted_by[i].count > 5) {
						for (Unit::AuraApplicationMap::iterator iter = target->GetAppliedAuras().begin(); iter != target->GetAppliedAuras().end();)
						{
							AuraApplication const* aurApp = iter->second;
							Aura const* aura = aurApp->GetBase();
							if (!aura->GetSpellInfo()->HasAttribute(SPELL_ATTR4_UNK21)
								&& !aura->IsPassive()
								&& !aurApp->IsPositive()
								&& !aura->GetSpellInfo()->HasAttribute(SPELL_ATTR3_DEATH_PERSISTENT))
								target->RemoveAura(iter);
							else
								++iter;
						}
						Remove();
						return;
					}
					hitted_by[i].count = hitted_by[i].count + 1;
					return;
				}
			HittedBy_Time str = HittedBy_Time(dmgInfo.GetAttacker(), 0);
			hitted_by.push_back(str);
		}

		uint8 a_counter;
		std::vector<HittedBy_Time> hitted_by;

		void Register() override
		{
			DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_myst_daimos_shield_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
			OnEffectAbsorb += AuraEffectAbsorbFn(spell_myst_daimos_shield_AuraScript::Absorb, EFFECT_0);
		}
	};

	AuraScript* GetAuraScript() const override
	{
		return new spell_myst_daimos_shield_AuraScript();
	}
};

void AddSC_mystic_spell_scripts()
{
	new spell_myst_daimos_sigil();
	new spell_myst_mantra();
	new spell_myst_stam_mantra();
	new spell_myst_mantra_cond();
	new spell_myst_mantra_hard_reset();
	new spell_myst_anomalyc();
	new spell_myst_myst_power();
	new spell_myst_orders_secret(); 
	new spell_myst_dark_connect();
	new spell_myst_deceit();
	new spell_myst_replace();
	new spell_myst_mantra_spirit();
	new spell_myst_mantra_speed();
	new spell_myst_dark_blood();
	new spell_myst_unsee();
	new spell_myst_return();
	new spell_myst_daimos_shield();
}

void AddSC_MysticLogIn() {
	new MysticLogIn;
}