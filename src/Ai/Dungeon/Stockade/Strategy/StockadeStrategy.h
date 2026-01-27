#ifndef _PLAYERBOT_CLASSICDUNGEONSTOCKSSTRATEGY_H
#define _PLAYERBOT_CLASSICDUNGEONSTOCKSSTRATEGY_H

#include "Multiplier.h"
#include "AiObjectContext.h"
#include "Strategy.h"

class ClassicDungeonStocksStrategy : public Strategy
{
public:
    ClassicDungeonStocksStrategy(PlayerbotAI* ai) : Strategy(ai) {}
    virtual std::string const getName() override { return "stockade"; }
    virtual void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    virtual void InitMultipliers(std::vector<Multiplier*>& multipliers) override;
};

#endif
