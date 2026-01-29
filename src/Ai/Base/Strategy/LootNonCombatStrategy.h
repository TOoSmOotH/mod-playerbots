/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_LOOTNONCOMBATSTRATEGY_H
#define _PLAYERBOT_LOOTNONCOMBATSTRATEGY_H

#include "Multiplier.h"
#include "Strategy.h"

class PlayerbotAI;

// Multiplier that delays target selection when there's loot available or needs to eat/drink
// This enables kill-loot-drink-kill pattern instead of kill-kill-kill-loot
class LootBeforeRetargetMultiplier : public Multiplier
{
public:
    LootBeforeRetargetMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "loot before retarget") {}
    float GetValue(Action* action) override;
};

class LootNonCombatStrategy : public Strategy
{
public:
    LootNonCombatStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
    std::string const getName() override { return "loot"; }
};

class GatherStrategy : public Strategy
{
public:
    GatherStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "gather"; }
};

class RevealStrategy : public Strategy
{
public:
    RevealStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override { return "reveal"; }
};

class UseBobberStrategy : public Strategy
{
public:
    UseBobberStrategy(PlayerbotAI* botAI) : Strategy(botAI){}
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    std::string const getName() override {return "use bobber";}
};

#endif
