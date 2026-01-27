#ifndef _PLAYERBOT_TBCDUNGEONHRSTRATEGY_H
#define _PLAYERBOT_TBCDUNGEONHRSTRATEGY_H

#include "Multiplier.h"
#include "AiObjectContext.h"
#include "Strategy.h"

class TbcDungeonHRStrategy : public Strategy
{
public:
    TbcDungeonHRStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    virtual std::string const getName() override { return "hellfire ramparts"; }
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
