# Copyright (C) 2008-2015 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# file(GLOB_RECURSE sources_Custom Custom/*.cpp Custom/*.h)

set(scripts_STAT_SRCS
  ${scripts_STAT_SRCS}
  ${sources_Custom}
    Custom/Transmog/Transmogrification.h
    Custom/Transmog/Transmogrification.cpp
    Custom/Transmog/Transmogrifier.cpp
	Custom/duel_reset.cpp 
	Custom/learn_all_spell.cpp
	Custom/COROVAN_IVENT_FOR_BLYADS.cpp
	Custom/npc_arena1v1.cpp
	Custom/npc_arena1v1.h
	Custom/tear5instance/Madness_Maiev.cpp
	Custom/tear5instance/Boss_Dead_Paladin.cpp
	Custom/tear5instance/Instance_t5.cpp
	Custom/tear5instance/Boss_Necromancers.cpp
	Custom/tear5instance/Boss_shadow_Mother_Shahraz.cpp
	Custom/npc_guild_level.cpp
	Custom/boss_new_killdjaden.cpp
	Custom/boss_challenge_survive.cpp
	Custom/enchant_visuals.cpp
	Custom/WorldBossesCustom.cpp
	Custom/CustomPlayerScripts.cpp
)

message("  -> Prepared: Custom")
