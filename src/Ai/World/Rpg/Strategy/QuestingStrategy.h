/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_QUESTINGSTRATEGY_H
#define _PLAYERBOT_QUESTINGSTRATEGY_H

#include "Strategy.h"

class PlayerbotAI;

/**
 * @brief Strategy for deterministic quest following
 *
 * When enabled via `nc +questing`, this strategy:
 * 1. Immediately and deterministically starts questing (no random activity selection)
 * 2. Whispers status updates to the owner (e.g., "Traveling to accept [Quest]")
 * 3. Integrates with the Quest Guide system for guided leveling
 */
class QuestingStrategy : public Strategy
{
public:
    QuestingStrategy(PlayerbotAI* botAI);

    std::string const getName() override { return "questing"; }
    std::vector<NextAction> getDefaultActions() override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif
