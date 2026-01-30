/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TravelSafetyTriggers.h"

#include "MobPackValue.h"
#include "Playerbots.h"
#include "TravelMgr.h"

bool MobPackInPathTrigger::IsActive()
{
    // Check if travel safety is enabled
    if (sPlayerbotAIConfig->travelSafetyMode == 0)
        return false;

    // Only active if we're traveling
    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return false;

    // Check for dangerous pack in path
    return AI_VALUE(bool, "dangerous pack in path");
}

bool CanPullFromPackTrigger::IsActive()
{
    // Check if hybrid mode is enabled (mode 2)
    if (sPlayerbotAIConfig->travelSafetyMode != 2)
        return false;

    // Only active if we're traveling and there's a pack in path
    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return false;

    // Check if there's a dangerous pack
    if (!AI_VALUE(bool, "dangerous pack in path"))
        return false;

    // Check if we have a valid pull target
    Unit* pullTarget = AI_VALUE(Unit*, "best pull target from pack");
    if (!pullTarget)
        return false;

    // Check if bot has ranged abilities
    bool hasRanged = false;
    uint8 cls = bot->getClass();
    switch (cls)
    {
        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_PRIEST:
        case CLASS_SHAMAN:
        case CLASS_DRUID:
            hasRanged = true;
            break;
        case CLASS_PALADIN:
        case CLASS_WARRIOR:
        case CLASS_ROGUE:
        case CLASS_DEATH_KNIGHT:
            // These classes can body pull but it's risky
            // Check if distance allows safe approach
            {
                float dist = bot->GetDistance(pullTarget);
                hasRanged = (dist < sPlayerbotAIConfig->meleeDistance * 2);
            }
            break;
    }

    return hasRanged;
}

bool TravelRetryExhaustedTrigger::IsActive()
{
    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isActive())
        return false;

    return target->isMaxRetry(false);
}

bool ShouldAvoidMobsTrigger::IsActive()
{
    // Check if travel safety is enabled (mode 1 or 2)
    if (sPlayerbotAIConfig->travelSafetyMode == 0)
        return false;

    // Only active if we're traveling
    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return false;

    // Already in combat - can't avoid
    if (bot->IsInCombat())
        return false;

    // Check for dangerous pack in path
    if (!AI_VALUE(bool, "dangerous pack in path"))
        return false;

    // In avoid-only mode (1), always try to avoid
    // In hybrid mode (2), prefer avoidance first
    return true;
}

bool BeingChasedDuringTravelTrigger::IsActive()
{
    // Check if travel safety is enabled
    if (sPlayerbotAIConfig->travelSafetyMode == 0)
        return false;

    // Only active if we're traveling
    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return false;

    // Check if we're in combat and being attacked by multiple mobs
    if (!bot->IsInCombat())
        return false;

    GuidVector attackers = AI_VALUE(GuidVector, "attackers");
    if (attackers.size() < 2)
        return false;

    // Check if any attackers are from a pack we tried to avoid
    uint8 packCount = AI_VALUE(uint8, "mob pack count");
    return packCount > 0;
}
