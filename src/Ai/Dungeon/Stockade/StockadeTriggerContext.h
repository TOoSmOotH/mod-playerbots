#ifndef _PLAYERBOT_CLASSICDUNGEONSTOCKSTRIGGERCONTEXT_H
#define _PLAYERBOT_CLASSICDUNGEONSTOCKSTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "AiObjectContext.h"
#include "StockadeTriggers.h"

class ClassicDungeonStocksTriggerContext : public NamedObjectContext<Trigger>
{
    public:
        ClassicDungeonStocksTriggerContext()
        {
            creators["bazil smoke bomb"] = &ClassicDungeonStocksTriggerContext::bazil_smoke_bomb;
        }
    private:
        static Trigger* bazil_smoke_bomb(PlayerbotAI* ai) { return new BazilSmokeBombTrigger(ai); }
};

#endif
