#ifndef _PLAYERBOT_CLASSICDUNGEONDMTRIGGERS_H
#define _PLAYERBOT_CLASSICDUNGEONDMTRIGGERS_H

#include "Trigger.h"
#include "PlayerbotAIConfig.h"
#include "GenericTriggers.h"

enum DeadminesIDs
{
    // Rhahk'Zor
    NPC_RHAHKZOR                    = 644,
    SPELL_RHAHKZOR_SLAM             = 6304,     // Inflicts 64-86 damage, stuns for 3 sec

    // Sneed's Shredder
    NPC_SNEEDS_SHREDDER             = 642,
    SPELL_DISTRACTING_PAIN          = 3603,     // Reduces casting speed by 35% for 15 sec
    SPELL_TERRIFY                   = 7399,     // Fear for 4 sec

    // Sneed
    NPC_SNEED                       = 643,

    // Gilnid
    NPC_GILNID                      = 1763,
    SPELL_MOLTEN_METAL              = 5213,     // 30 Fire damage every 3 sec, slows for 15 sec

    // Mr. Smite
    NPC_MR_SMITE                    = 646,
    SPELL_SMITE_SLAM                = 6435,     // 50 damage, stuns for 3 sec
    SPELL_SMITE_STOMP               = 6432,     // AoE stun for 10 sec (phase transition)

    // Captain Greenskin
    NPC_CAPTAIN_GREENSKIN           = 647,

    // Edwin VanCleef
    NPC_EDWIN_VANCLEEF              = 639,
    NPC_DEFIAS_BLACKGUARD           = 636,      // Adds spawned at 50% health

    // Cookie
    NPC_COOKIE                      = 645,
    SPELL_COOKIE_COOKING            = 5174,     // Self-heal 111-129
    SPELL_ACID_SPLASH               = 6306,     // 50 Nature damage every 5 sec for 30 sec
};

class MrSmiteStompTrigger : public Trigger
{
public:
    MrSmiteStompTrigger(PlayerbotAI* ai) : Trigger(ai, "mr. smite stomp") {}
    bool IsActive() override;
};

class VanCleefAddsTrigger : public Trigger
{
public:
    VanCleefAddsTrigger(PlayerbotAI* ai) : Trigger(ai, "vancleef adds") {}
    bool IsActive() override;
};

#endif
