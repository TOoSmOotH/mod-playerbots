#ifndef _PLAYERBOT_TBCDUNGEONHRMULTIPLIERS_H
#define _PLAYERBOT_TBCDUNGEONHRMULTIPLIERS_H

#include "Multiplier.h"

class OmorMultiplier : public Multiplier
{
    public:
        OmorMultiplier(PlayerbotAI* ai) : Multiplier(ai, "omor the unscarred") {}

    public:
        virtual float GetValue(Action* action);
};

class NazanMultiplier : public Multiplier
{
    public:
        NazanMultiplier(PlayerbotAI* ai) : Multiplier(ai, "nazan") {}

    public:
        virtual float GetValue(Action* action);
};

#endif
