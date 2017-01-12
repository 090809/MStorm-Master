#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "Chat.h"

class player_account_mounts : public PlayerScript
{
public:
	player_account_mounts() : PlayerScript("player_account_mounts") {}

	void OnLogin(Player* player, bool /*firstLogin*/) override
	{
		PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_LEARNED_MOUNTS);
		stmt->setUInt32(0, player->GetSession()->GetAccountId());
		PreparedQueryResult result = CharacterDatabase.Query(stmt);

		if (result)
		{
			do
			{
				Field* fields = result->Fetch();
				player->LearnSpell(fields[0].GetInt32(), false);
			} while (result->NextRow());
		}
	}

	void OnSpellLearn(Player* player, uint32 spell_id) override
	{
		SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell_id);
		if (spellInfo->IsAbilityOfSkillType(SKILL_MOUNTS))
		{
			PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_LEARNED_MOUNTS);
			stmt->setUInt32(0, player->GetSession()->GetAccountId());
			stmt->setUInt32(1, spell_id);
			CharacterDatabase.Execute(stmt);
		}
	}
};

class player_first_login_open_book : public PlayerScript
{
public:
	player_first_login_open_book() : PlayerScript("player_first_login_open_book") {}

	void OnLogin(Player* player, bool firstLogin) override
	{
		if (!firstLogin)
			return;

		if (!(player->GetSession()->GetTutorialInt(0) < 1000))
			return;

		WorldSession* session = player->GetSession();
		uint32 pageID = 5000;
		while (pageID)
		{
			PageText const* pageText = sObjectMgr->GetPageText(pageID);
			// guess size
			WorldPacket data(SMSG_PAGE_TEXT_QUERY_RESPONSE, 50);
			data << pageID;

			if (!pageText)
			{
				data << "Item page missing.";
				data << uint32(0);
				pageID = 0;
			}
			else
			{
				std::string Text = pageText->Text;

				int loc_idx = session->GetSessionDbLocaleIndex();
				if (loc_idx >= 0)
					if (PageTextLocale const* player = sObjectMgr->GetPageTextLocale(pageID))
						ObjectMgr::GetLocaleString(player->Text, loc_idx, Text);

				data << Text;
				data << uint32(pageText->NextPage);
				pageID = pageText->NextPage;
			}
			session->SendPacket(&data);

			TC_LOG_DEBUG("network", "WORLD: Sent SMSG_PAGE_TEXT_QUERY_RESPONSE");
		}
	}
};

void AddSC_player_account_mounts()
{
	new player_account_mounts();
	new player_first_login_open_book();
}