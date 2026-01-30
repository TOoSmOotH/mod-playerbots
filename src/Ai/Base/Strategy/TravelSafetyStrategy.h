/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_TRAVELSAFETYSTRATEGY_H
#define _PLAYERBOT_TRAVELSAFETYSTRATEGY_H

#include "Strategy.h"

class PlayerbotAI;

/**
 * @brief Strategy for safely handling mob packs during travel
 *
 * This strategy implements a hybrid approach:
 * 1. Avoidance (priority): Detect mob packs and calculate detour paths
 * 2. Single-target pulling: When avoidance fails, pull isolated mobs with kiting
 * 3. Flee and retry: When overwhelmed, flee toward travel destination and retry
 *
 * Decision flow:
 * Detect Pack -> Can Avoid? -> DETOUR
 *            -> Can't Avoid + Can Pull? -> PULL + KITE
 *            -> Too Dangerous? -> FLEE + RETRY
 */
class TravelSafetyStrategy : public Strategy
{
public:
    TravelSafetyStrategy(PlayerbotAI* botAI);

    std::string const getName() override { return "travel safety"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

/**
 * @brief Strategy specifically for avoiding mob packs (mode 1)
 */
class TravelAvoidStrategy : public Strategy
{
public:
    TravelAvoidStrategy(PlayerbotAI* botAI);

    std::string const getName() override { return "travel avoid"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif
