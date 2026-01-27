#ifndef _PLAYERBOT_CLASSICDUNGEONDMSTRATEGY_H
#define _PLAYERBOT_CLASSICDUNGEONDMSTRATEGY_H

#include "Multiplier.h"
#include "AiObjectContext.h"
#include "Strategy.h"

class ClassicDungeonDMStrategy : public Strategy
{
public:
    ClassicDungeonDMStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    virtual std::string const getName() override { return "deadmines"; }
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
