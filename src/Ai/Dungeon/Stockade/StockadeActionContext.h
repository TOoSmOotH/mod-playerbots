#ifndef _PLAYERBOT_CLASSICDUNGEONSTOCKSACTIONCONTEXT_H
#define _PLAYERBOT_CLASSICDUNGEONSTOCKSACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "StockadeActions.h"

class ClassicDungeonStocksActionContext : public NamedObjectContext<Action>
{
    public:
        ClassicDungeonStocksActionContext() {
            // Stockade has no special actions - all mechanics handled by multipliers
        }
};

#endif
