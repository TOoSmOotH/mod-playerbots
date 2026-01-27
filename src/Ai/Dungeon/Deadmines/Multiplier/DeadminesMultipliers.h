#ifndef _PLAYERBOT_CLASSICDUNGEONDMMULTIPLIERS_H
#define _PLAYERBOT_CLASSICDUNGEONDMMULTIPLIERS_H

#include "Multiplier.h"

class MrSmiteMultiplier : public Multiplier
{
    public:
        MrSmiteMultiplier(PlayerbotAI* ai) : Multiplier(ai, "mr. smite") {}

    public:
        virtual float GetValue(Action* action);
};

class VanCleefMultiplier : public Multiplier
{
    public:
        VanCleefMultiplier(PlayerbotAI* ai) : Multiplier(ai, "edwin vancleef") {}

    public:
        virtual float GetValue(Action* action);
};

#endif
