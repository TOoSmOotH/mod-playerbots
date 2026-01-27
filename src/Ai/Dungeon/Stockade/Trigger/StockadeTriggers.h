#ifndef _PLAYERBOT_CLASSICDUNGEONSTOCKSTRIGGERS_H
#define _PLAYERBOT_CLASSICDUNGEONSTOCKSTRIGGERS_H

#include "Trigger.h"
#include "PlayerbotAIConfig.h"
#include "GenericTriggers.h"

enum StockadeIDs
{
    // Targorr the Dread
    NPC_TARGORR_THE_DREAD           = 1696,
    SPELL_THRASH                    = 3391,     // 2 extra attacks
    SPELL_ENRAGE                    = 8599,     // Enrage buff

    // Kam Deepfury
    NPC_KAM_DEEPFURY                = 1666,
    SPELL_SHIELD_SLAM               = 8242,     // 20 damage + 2 sec stun
    SPELL_IMPROVED_BLOCKING         = 3419,     // 75% block chance for 6 sec

    // Hamhock
    NPC_HAMHOCK                     = 1717,
    SPELL_BLOODLUST                 = 6742,     // 30% attack speed buff

    // Dextren Ward
    NPC_DEXTREN_WARD                = 1663,

    // Bazil Thredd
    NPC_BAZIL_THREDD                = 1716,
    SPELL_SMOKE_BOMB                = 7964,     // AoE 4 sec stun

    // Bruegal Ironknuckle (rare)
    NPC_BRUEGAL_IRONKNUCKLE         = 1720,
};

class BazilSmokeBombTrigger : public Trigger
{
public:
    BazilSmokeBombTrigger(PlayerbotAI* ai) : Trigger(ai, "bazil smoke bomb") {}
    bool IsActive() override;
};

#endif
