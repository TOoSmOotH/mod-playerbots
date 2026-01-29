/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "QuestingStrategy.h"

#include "Playerbots.h"

QuestingStrategy::QuestingStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

std::vector<NextAction> QuestingStrategy::getDefaultActions()
{
    // Higher relevance (12.0f) ensures questing takes priority over other activities like grinding (11.0f)
    return {
        NextAction("questing update", 12.0f)
    };
}

void QuestingStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Trigger for executing questing when in RPG_QUESTING status
    triggers.push_back(
        new TriggerNode(
            "questing status",
            {
                NextAction("questing execute", 4.0f)
            }
        )
    );

    // Attack only quest-specific mobs when at objective with no target
    triggers.push_back(
        new TriggerNode(
            "no target",
            {
                NextAction("attack quest target", 4.0f)
            }
        )
    );

    // Loot triggers - essential for quest item collection
    triggers.push_back(
        new TriggerNode(
            "can loot",
            {
                NextAction("open loot", 8.0f)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "far from loot target",
            {
                NextAction("move to loot", 7.0f)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "loot available",
            {
                NextAction("loot", 6.0f)
            }
        )
    );
    triggers.push_back(
        new TriggerNode(
            "often",
            {
                NextAction("add all loot", 5.0f)
            }
        )
    );

    // Stuck detection for selfbots - shorter timeout than regular stuck detection
    // Triggers when bot hasn't made progress toward quest objective for ~90 seconds
    triggers.push_back(
        new TriggerNode(
            "selfbot questing stuck",
            {
                NextAction("questing recover stuck", 10.0f)
            }
        )
    );
}
