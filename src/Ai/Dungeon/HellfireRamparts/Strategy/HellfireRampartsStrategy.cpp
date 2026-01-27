#include "HellfireRampartsStrategy.h"
#include "HellfireRampartsMultipliers.h"

void TbcDungeonHRStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Watchkeeper Gargolmar
    // - Mortal Wound stacks on tank, healers need to manage
    // - Charge targets furthest player - ranged should stay close
    // - Has healer adds (Hellfire Watchers) - handled by normal kill priority

    // Omor the Unscarred
    // - Treacherous Aura: Cursed player deals shadow damage to nearby allies
    //   Must spread out when affected
    triggers.push_back(new TriggerNode("omor treacherous aura",
        { NextAction("avoid treacherous aura", ACTION_RAID + 5) }));
    // - Fiendish Hound adds spawn throughout fight - normal target priority handles
    // - Shadow Whip (levitate) - can't do much about this

    // Vazruden the Herald + Nazan
    // - Phase 1: Kill two sentries
    // - Phase 2: Vazruden engages while Nazan flies overhead dropping fireballs
    // - Phase 3: Nazan lands and uses Flame Breath (frontal cone)

    // Avoid standing in Liquid Fire pools
    triggers.push_back(new TriggerNode("nazan liquid fire",
        { NextAction("avoid liquid fire", ACTION_RAID + 6) }));

    // Avoid Nazan's frontal Flame Breath
    triggers.push_back(new TriggerNode("nazan flame breath",
        { NextAction("avoid flame breath", ACTION_RAID + 5) }));
}

void TbcDungeonHRStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new OmorMultiplier(botAI));
    multipliers.push_back(new NazanMultiplier(botAI));
}
