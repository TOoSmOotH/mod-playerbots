/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TravelSafetyActions.h"

#include "FleeManager.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "TravelMgr.h"

// ============================================================================
// AvoidMobPackAction
// ============================================================================

bool AvoidMobPackAction::Execute(Event event)
{
    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return false;

    WorldPosition destPos = *target->getPosition();

    // Get packs blocking the path
    MobPacksNearbyValue* packValue = dynamic_cast<MobPacksNearbyValue*>(
        context->GetValue<std::vector<MobPackInfo>>("mob packs nearby"));
    if (!packValue)
        return false;

    WorldPosition botPos(bot);
    std::vector<MobPackInfo> blockedPacks = packValue->GetPacksInPath(
        botPos.getX(), botPos.getY(), botPos.getZ(),
        destPos.getX(), destPos.getY(), destPos.getZ(),
        sPlayerbotAIConfig->aggroDistance);

    if (blockedPacks.empty())
        return false;

    // Try to find a detour around the first blocking pack
    const MobPackInfo& pack = blockedPacks[0];
    float detourX, detourY, detourZ;

    if (CalculateDetourPoint(pack, destPos.getX(), destPos.getY(), detourX, detourY, detourZ))
    {
        // Move to the detour point
        return MoveTo(bot->GetMapId(), detourX, detourY, detourZ, false, false);
    }

    // No detour found - increment retry and let another action handle it
    target->incRetry(false);
    return false;
}

bool AvoidMobPackAction::isUseful()
{
    if (sPlayerbotAIConfig->travelSafetyMode == 0)
        return false;

    if (bot->IsInCombat())
        return false;

    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return false;

    return AI_VALUE(bool, "dangerous pack in path");
}

bool AvoidMobPackAction::CalculateDetourPoint(const MobPackInfo& pack, float destX, float destY,
                                               float& detourX, float& detourY, float& detourZ)
{
    float botX = bot->GetPositionX();
    float botY = bot->GetPositionY();
    float botZ = bot->GetPositionZ();

    // Calculate direction from bot to destination
    float dirX = destX - botX;
    float dirY = destY - botY;
    float pathLen = sqrt(dirX * dirX + dirY * dirY);

    if (pathLen < 1.0f)
        return false;

    // Normalize direction
    dirX /= pathLen;
    dirY /= pathLen;

    // Perpendicular directions (left and right)
    float perpLeftX = -dirY;
    float perpLeftY = dirX;
    float perpRightX = dirY;
    float perpRightY = -dirX;

    // Calculate detour distance: pack radius + aggro distance + safety margin
    float detourDist = pack.radius + sPlayerbotAIConfig->aggroDistance + 10.0f;

    // Try both perpendicular directions
    float candidates[2][3];
    bool valid[2] = {false, false};

    // Left detour
    candidates[0][0] = pack.centerX + perpLeftX * detourDist;
    candidates[0][1] = pack.centerY + perpLeftY * detourDist;
    candidates[0][2] = botZ;

    // Right detour
    candidates[1][0] = pack.centerX + perpRightX * detourDist;
    candidates[1][1] = pack.centerY + perpRightY * detourDist;
    candidates[1][2] = botZ;

    // Validate candidates
    for (int i = 0; i < 2; i++)
    {
        // Update Z coordinate
        bot->UpdateAllowedPositionZ(candidates[i][0], candidates[i][1], candidates[i][2]);

        // Check if position is reachable and safe
        if (IsPositionSafe(candidates[i][0], candidates[i][1], candidates[i][2], sPlayerbotAIConfig->aggroDistance))
        {
            if (bot->IsWithinLOS(candidates[i][0], candidates[i][1], candidates[i][2]))
            {
                valid[i] = true;
            }
        }
    }

    // Pick the best valid candidate (prefer one closer to destination)
    int best = -1;
    float bestDist = FLT_MAX;

    for (int i = 0; i < 2; i++)
    {
        if (!valid[i])
            continue;

        float distToDest = sqrt(
            (candidates[i][0] - destX) * (candidates[i][0] - destX) +
            (candidates[i][1] - destY) * (candidates[i][1] - destY));

        if (distToDest < bestDist)
        {
            bestDist = distToDest;
            best = i;
        }
    }

    if (best < 0)
        return false;

    detourX = candidates[best][0];
    detourY = candidates[best][1];
    detourZ = candidates[best][2];

    return true;
}

bool AvoidMobPackAction::IsPositionSafe(float x, float y, float z, float minDistance)
{
    GuidVector possibleTargets = AI_VALUE(GuidVector, "possible targets no los");

    for (ObjectGuid guid : possibleTargets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit)
            continue;

        float dist = unit->GetDistance(x, y, z);
        if (dist < minDistance)
            return false;
    }

    return true;
}

// ============================================================================
// TravelPullAction
// ============================================================================

bool TravelPullAction::Execute(Event event)
{
    Unit* pullTarget = AI_VALUE(Unit*, "best pull target from pack");
    if (!pullTarget)
        return false;

    // Get the pack this mob belongs to
    MobPacksNearbyValue* packValue = dynamic_cast<MobPacksNearbyValue*>(
        context->GetValue<std::vector<MobPackInfo>>("mob packs nearby"));
    if (!packValue)
        return false;

    MobPackInfo* pack = packValue->GetPackForMob(pullTarget->GetGUID());
    if (!pack)
        return false;

    // Calculate kite position
    float kiteX, kiteY, kiteZ;
    if (!CalculateKitePosition(pullTarget, *pack, kiteX, kiteY, kiteZ))
        return false;

    // Move to kite position first if not there
    float distToKite = bot->GetDistance(kiteX, kiteY, kiteZ);
    if (distToKite > sPlayerbotAIConfig->followDistance)
    {
        return MoveTo(bot->GetMapId(), kiteX, kiteY, kiteZ, false, false);
    }

    // We're at kite position - try to pull
    if (CastPullSpell(pullTarget))
    {
        // Set this as our pull target for the combat system
        context->GetValue<ObjectGuid>("pull target")->Set(pullTarget->GetGUID());
        return true;
    }

    return false;
}

bool TravelPullAction::isUseful()
{
    if (sPlayerbotAIConfig->travelSafetyMode != 2)  // Only in hybrid mode
        return false;

    if (bot->IsInCombat())
        return false;

    Unit* pullTarget = AI_VALUE(Unit*, "best pull target from pack");
    return pullTarget != nullptr;
}

bool TravelPullAction::CalculateKitePosition(Unit* pullTarget, const MobPackInfo& pack,
                                              float& kiteX, float& kiteY, float& kiteZ)
{
    float botX = bot->GetPositionX();
    float botY = bot->GetPositionY();
    float botZ = bot->GetPositionZ();

    // Calculate direction away from pack center
    float awayDirX = botX - pack.centerX;
    float awayDirY = botY - pack.centerY;
    float awayLen = sqrt(awayDirX * awayDirX + awayDirY * awayDirY);

    if (awayLen < 1.0f)
    {
        // Bot is at pack center - pick arbitrary direction
        awayDirX = 1.0f;
        awayDirY = 0.0f;
    }
    else
    {
        awayDirX /= awayLen;
        awayDirY /= awayLen;
    }

    // Kite position: kiteDistance away from pack center
    float kiteDistance = sPlayerbotAIConfig->travelKiteDistance;
    kiteX = pack.centerX + awayDirX * kiteDistance;
    kiteY = pack.centerY + awayDirY * kiteDistance;
    kiteZ = botZ;

    // Update Z coordinate
    bot->UpdateAllowedPositionZ(kiteX, kiteY, kiteZ);

    // Validate position
    if (!bot->IsWithinLOS(kiteX, kiteY, kiteZ))
        return false;

    return true;
}

uint32 TravelPullAction::GetPullSpellId()
{
    uint8 cls = bot->getClass();
    uint32 spellId = 0;

    switch (cls)
    {
        case CLASS_HUNTER:
            // Auto Shot or Arcane Shot
            spellId = AI_VALUE2(uint32, "spell id", "arcane shot");
            if (spellId)
                return spellId;
            return 75;  // Auto Shot

        case CLASS_MAGE:
            spellId = AI_VALUE2(uint32, "spell id", "frostbolt");
            if (spellId)
                return spellId;
            spellId = AI_VALUE2(uint32, "spell id", "fireball");
            if (spellId)
                return spellId;
            break;

        case CLASS_WARLOCK:
            spellId = AI_VALUE2(uint32, "spell id", "shadow bolt");
            if (spellId)
                return spellId;
            break;

        case CLASS_PRIEST:
            spellId = AI_VALUE2(uint32, "spell id", "smite");
            if (spellId)
                return spellId;
            spellId = AI_VALUE2(uint32, "spell id", "mind blast");
            if (spellId)
                return spellId;
            break;

        case CLASS_SHAMAN:
            spellId = AI_VALUE2(uint32, "spell id", "lightning bolt");
            if (spellId)
                return spellId;
            break;

        case CLASS_DRUID:
            spellId = AI_VALUE2(uint32, "spell id", "wrath");
            if (spellId)
                return spellId;
            spellId = AI_VALUE2(uint32, "spell id", "moonfire");
            if (spellId)
                return spellId;
            break;

        case CLASS_PALADIN:
            spellId = AI_VALUE2(uint32, "spell id", "judgement");
            if (spellId)
                return spellId;
            spellId = AI_VALUE2(uint32, "spell id", "exorcism");
            if (spellId)
                return spellId;
            break;

        case CLASS_WARRIOR:
            spellId = AI_VALUE2(uint32, "spell id", "throw");
            if (spellId)
                return spellId;
            spellId = AI_VALUE2(uint32, "spell id", "shoot");
            if (spellId)
                return spellId;
            spellId = AI_VALUE2(uint32, "spell id", "heroic throw");
            if (spellId)
                return spellId;
            break;

        case CLASS_ROGUE:
            spellId = AI_VALUE2(uint32, "spell id", "throw");
            if (spellId)
                return spellId;
            spellId = AI_VALUE2(uint32, "spell id", "shoot");
            if (spellId)
                return spellId;
            break;

        case CLASS_DEATH_KNIGHT:
            spellId = AI_VALUE2(uint32, "spell id", "death coil");
            if (spellId)
                return spellId;
            spellId = AI_VALUE2(uint32, "spell id", "icy touch");
            if (spellId)
                return spellId;
            break;
    }

    return 0;
}

bool TravelPullAction::CastPullSpell(Unit* target)
{
    uint32 spellId = GetPullSpellId();
    if (!spellId)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return false;

    // Check range
    float dist = bot->GetDistance(target);
    float maxRange = spellInfo->GetMaxRange(false);
    float minRange = spellInfo->GetMinRange(false);

    if (dist > maxRange || dist < minRange)
    {
        // Move to range
        float optimalRange = (maxRange + minRange) / 2.0f;
        if (optimalRange < minRange + 5.0f)
            optimalRange = minRange + 5.0f;

        return ChaseTo(target, optimalRange);
    }

    // Face target and cast
    bot->SetFacingToObject(target);

    return botAI->CastSpell(spellId, target);
}

// ============================================================================
// FleeToTravelSafePointAction
// ============================================================================

bool FleeToTravelSafePointAction::Execute(Event event)
{
    float fleeX, fleeY, fleeZ;

    if (!CalculateSafeFleePoint(fleeX, fleeY, fleeZ))
    {
        // Fallback to standard flee
        FleeManager manager(bot, sPlayerbotAIConfig->fleeDistance, false, WorldPosition(bot));
        if (manager.CalculateDestination(&fleeX, &fleeY, &fleeZ))
        {
            IncrementRetryCount();
            return MoveTo(bot->GetMapId(), fleeX, fleeY, fleeZ, false, false);
        }
        return false;
    }

    IncrementRetryCount();
    return MoveTo(bot->GetMapId(), fleeX, fleeY, fleeZ, false, false);
}

bool FleeToTravelSafePointAction::isUseful()
{
    if (sPlayerbotAIConfig->travelSafetyMode == 0)
        return false;

    if (!bot->IsInCombat())
        return false;

    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return false;

    // Check if we're fighting multiple mobs
    GuidVector attackers = AI_VALUE(GuidVector, "attackers");
    return attackers.size() >= 2;
}

bool FleeToTravelSafePointAction::CalculateSafeFleePoint(float& fleeX, float& fleeY, float& fleeZ)
{
    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return false;

    WorldPosition destPos = *target->getPosition();
    float botX = bot->GetPositionX();
    float botY = bot->GetPositionY();
    float botZ = bot->GetPositionZ();

    // Direction toward destination
    float dirX = destPos.getX() - botX;
    float dirY = destPos.getY() - botY;
    float dirLen = sqrt(dirX * dirX + dirY * dirY);

    if (dirLen < 1.0f)
        return false;

    dirX /= dirLen;
    dirY /= dirLen;

    // Try to flee in direction of destination with some angle variations
    float fleeDistance = sPlayerbotAIConfig->fleeDistance * 2;
    float angles[] = {0.0f, 0.3f, -0.3f, 0.6f, -0.6f};  // Radians offset from destination direction

    for (float angleOffset : angles)
    {
        float angle = atan2(dirY, dirX) + angleOffset;
        float testX = botX + cos(angle) * fleeDistance;
        float testY = botY + sin(angle) * fleeDistance;
        float testZ = botZ;

        bot->UpdateAllowedPositionZ(testX, testY, testZ);

        // Check if position is reachable
        if (!bot->IsWithinLOS(testX, testY, testZ))
            continue;

        // Check water
        Map* map = bot->GetMap();
        if (map && map->IsInWater(bot->GetPhaseMask(), testX, testY, testZ, bot->GetCollisionHeight()))
            continue;

        // Check distance from attackers
        bool safe = true;
        GuidVector attackers = AI_VALUE(GuidVector, "attackers");
        for (ObjectGuid guid : attackers)
        {
            Unit* attacker = botAI->GetUnit(guid);
            if (!attacker)
                continue;

            float dist = attacker->GetDistance(testX, testY, testZ);
            if (dist < sPlayerbotAIConfig->aggroDistance)
            {
                safe = false;
                break;
            }
        }

        if (safe)
        {
            fleeX = testX;
            fleeY = testY;
            fleeZ = testZ;
            return true;
        }
    }

    return false;
}

void FleeToTravelSafePointAction::IncrementRetryCount()
{
    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (target && target->isTraveling())
    {
        target->incRetry(false);

        // Add delay before next attempt
        target->setExpireIn(target->getTimeLeft() + sPlayerbotAIConfig->travelFleeRetryTime);
    }
}
