/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TravelSafetyStrategy.h"

#include "Playerbots.h"

TravelSafetyStrategy::TravelSafetyStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

void TravelSafetyStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Priority order matters - higher relevance = higher priority

    // Highest priority: flee if being chased by multiple mobs
    triggers.push_back(
        new TriggerNode(
            "being chased during travel",
            {
                NextAction("flee to travel safe point", 90.0f)
            }
        )
    );

    // High priority: try to avoid mob packs first
    triggers.push_back(
        new TriggerNode(
            "should avoid mobs",
            {
                NextAction("avoid mob pack", 85.0f)
            }
        )
    );

    // Medium priority: if can't avoid, try pulling single target
    triggers.push_back(
        new TriggerNode(
            "can pull from pack",
            {
                NextAction("travel pull", 80.0f)
            }
        )
    );

    // Lower priority: generic mob pack in path warning
    triggers.push_back(
        new TriggerNode(
            "mob pack in path",
            {
                NextAction("avoid mob pack", 75.0f)
            }
        )
    );

    // If retries exhausted, abandon the travel target
    triggers.push_back(
        new TriggerNode(
            "travel retry exhausted",
            {
                NextAction("choose travel target", 70.0f)
            }
        )
    );
}

void TravelSafetyStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    Strategy::InitMultipliers(multipliers);
}

TravelAvoidStrategy::TravelAvoidStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

void TravelAvoidStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Only avoidance - no pulling
    triggers.push_back(
        new TriggerNode(
            "should avoid mobs",
            {
                NextAction("avoid mob pack", 85.0f)
            }
        )
    );

    triggers.push_back(
        new TriggerNode(
            "mob pack in path",
            {
                NextAction("avoid mob pack", 75.0f)
            }
        )
    );

    triggers.push_back(
        new TriggerNode(
            "travel retry exhausted",
            {
                NextAction("choose travel target", 70.0f)
            }
        )
    );
}
