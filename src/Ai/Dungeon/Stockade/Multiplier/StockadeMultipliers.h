#ifndef _PLAYERBOT_CLASSICDUNGEONSTOCKSMULTIPLIERS_H
#define _PLAYERBOT_CLASSICDUNGEONSTOCKSMULTIPLIERS_H

#include "Multiplier.h"

class BazilThreddMultiplier : public Multiplier
{
    public:
        BazilThreddMultiplier(PlayerbotAI* ai) : Multiplier(ai, "bazil thredd") {}

    public:
        virtual float GetValue(Action* action);
};

#endif
