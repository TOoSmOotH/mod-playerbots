#ifndef _PLAYERBOT_TBCDUNGEONHRTRIGGERS_H
#define _PLAYERBOT_TBCDUNGEONHRTRIGGERS_H

#include "Trigger.h"
#include "PlayerbotAIConfig.h"
#include "GenericTriggers.h"
#include "DungeonStrategyUtils.h"

enum HellfireRampartsIDs
{
    // Watchkeeper Gargolmar
    NPC_WATCHKEEPER_GARGOLMAR       = 17306,
    NPC_HELLFIRE_WATCHER            = 17309,    // Healer adds
    SPELL_MORTAL_WOUND              = 30641,    // Reduces healing by 5% per stack, 20 sec

    // Omor the Unscarred
    NPC_OMOR_THE_UNSCARRED          = 17308,
    NPC_FIENDISH_HOUND              = 17540,    // Summoned adds
    SPELL_TREACHEROUS_AURA_N        = 30695,    // Curse: 360-440 shadow dmg to nearby allies, 15 sec
    SPELL_TREACHEROUS_AURA_H        = 37566,    // Heroic version: 4500-6000 shadow dmg
    SPELL_SUMMON_FIENDISH_HOUND     = 30707,
    SPELL_SHADOW_BOLT_N             = 30686,
    SPELL_SHADOW_BOLT_H             = 39297,
    SPELL_DEMONIC_SHIELD            = 31901,    // 75% damage reduction at 20% health

    // Vazruden the Herald
    NPC_VAZRUDEN                    = 17537,

    // Nazan
    NPC_NAZAN                       = 17536,
    SPELL_FIREBALL_N                = 30691,
    SPELL_FIREBALL_H                = 36920,
    SPELL_CONE_OF_FIRE_N            = 30926,    // Flame Breath
    SPELL_CONE_OF_FIRE_H            = 36921,
    SPELL_LIQUID_FIRE               = 30643,    // Ground fire pool aura
    SPELL_LIQUID_FIRE_AURA          = 30644,    // Damage aura from standing in fire
};

#define SPELL_TREACHEROUS_AURA      DUNGEON_MODE(bot, SPELL_TREACHEROUS_AURA_N, SPELL_TREACHEROUS_AURA_H)
#define SPELL_CONE_OF_FIRE          DUNGEON_MODE(bot, SPELL_CONE_OF_FIRE_N, SPELL_CONE_OF_FIRE_H)

class OmorTreacherousAuraTrigger : public Trigger
{
public:
    OmorTreacherousAuraTrigger(PlayerbotAI* ai) : Trigger(ai, "omor treacherous aura") {}
    bool IsActive() override;
};

class NazanLiquidFireTrigger : public Trigger
{
public:
    NazanLiquidFireTrigger(PlayerbotAI* ai) : Trigger(ai, "nazan liquid fire") {}
    bool IsActive() override;
};

class NazanFlameBreathTrigger : public Trigger
{
public:
    NazanFlameBreathTrigger(PlayerbotAI* ai) : Trigger(ai, "nazan flame breath") {}
    bool IsActive() override;
};

#endif
