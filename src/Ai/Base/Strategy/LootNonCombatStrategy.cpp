/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LootNonCombatStrategy.h"

#include "Action.h"
#include "AiObjectContext.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

// Multiplier that delays target selection when loot is available or needs to eat/drink
// Returns 0 to cancel targeting actions, allowing loot/food/drink to run first
float LootBeforeRetargetMultiplier::GetValue(Action* action)
{
    if (!action)
        return 1.0f;

    // Only affect targeting actions, not combat actions
    std::string const& name = action->getName();
    if (name == "dps assist" || name == "dps aoe" || name == "tank assist" ||
        name == "attack anything" || name == "attack quest target")
    {
        AiObjectContext* context = botAI->GetAiObjectContext();
        if (!context)
            return 1.0f;

        // Check if there's loot to pick up
        if (context->GetValue<bool>("has available loot")->Get())
        {
            return 0.0f;
        }

        // Check if mana is low and bot has mana (delay attack to drink first)
        Player* bot = botAI->GetBot();
        if (bot && !bot->IsInCombat() && bot->getPowerType() == POWER_MANA)
        {
            uint8 mana = context->GetValue<uint8>("mana", "self target")->Get();
            if (mana < sPlayerbotAIConfig->lowMana)
            {
                return 0.0f;
            }
        }

        // Check if health is low (delay attack to eat first)
        if (bot && !bot->IsInCombat())
        {
            uint8 health = context->GetValue<uint8>("health", "self target")->Get();
            if (health < sPlayerbotAIConfig->lowHealth)
            {
                return 0.0f;
            }
        }
    }

    return 1.0f;
}

void LootNonCombatStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("loot available", { NextAction("loot", 6.0f) }));
    triggers.push_back(
        new TriggerNode("far from loot target", { NextAction("move to loot", 7.0f) }));
    triggers.push_back(new TriggerNode("can loot", { NextAction("open loot", 8.0f) }));
    triggers.push_back(new TriggerNode("often", { NextAction("add all loot", 5.0f) }));
}

void LootNonCombatStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new LootBeforeRetargetMultiplier(botAI));
}

void GatherStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(
        new TriggerNode("timer", { NextAction("add gathering loot", 5.0f) }));
}

void RevealStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(
        new TriggerNode("often", { NextAction("reveal gathering item", 50.0f) }));
}

void UseBobberStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
     triggers.push_back(
        new TriggerNode("can use fishing bobber", { NextAction("use fishing bobber", 20.0f) }));
    triggers.push_back(
        new TriggerNode("random", { NextAction("remove bobber strategy", 20.0f) }));
}
