/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "MobPackValue.h"

#include "AttackersValue.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "LootMgr.h"
#include "Playerbots.h"
#include "ServerFacade.h"
#include "TravelMgr.h"

namespace
{
    // Check if a mob is needed for one of the bot's quests
    bool IsMobNeededForQuest(Player* bot, Unit* unit)
    {
        if (!bot || !unit)
            return false;

        uint32 entry = unit->GetEntry();

        QuestStatusMap& questMap = bot->getQuestStatusMap();
        for (auto& quest : questMap)
        {
            Quest const* questTemplate = sObjectMgr->GetQuestTemplate(quest.first);
            if (!questTemplate)
                continue;

            uint32 questId = questTemplate->GetQuestId();
            if (!questId)
                continue;

            QuestStatus status = bot->GetQuestStatus(questId);
            if (status != QUEST_STATUS_INCOMPLETE)
                continue;

            auto it = questMap.find(questId);
            if (it == questMap.end())
                continue;
            QuestStatusData const* questStatus = &it->second;

            // Check if this mob is a quest kill objective
            for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
            {
                int32 requiredNpc = questTemplate->RequiredNpcOrGo[j];
                if (requiredNpc > 0 && static_cast<uint32>(requiredNpc) == entry)
                {
                    int required = questTemplate->RequiredNpcOrGoCount[j];
                    int available = questStatus->CreatureOrGOCount[j];
                    if (required && available < required)
                        return true;
                }
            }
        }

        // Also check for quest loot
        if (CreatureTemplate const* data = sObjectMgr->GetCreatureTemplate(entry))
        {
            if (uint32 lootId = data->lootid)
            {
                if (LootTemplates_Creature.HaveQuestLootForPlayer(lootId, bot))
                    return true;
            }
        }

        return false;
    }
}

MobPacksNearbyValue::MobPacksNearbyValue(PlayerbotAI* botAI, std::string const name, float range)
    : CalculatedValue<std::vector<MobPackInfo>>(botAI, name, 2 * 1000),
      range(range),
      packRadius(sPlayerbotAIConfig->travelPackRadius)
{
}

std::vector<MobPackInfo> MobPacksNearbyValue::Calculate()
{
    std::vector<MobPackInfo> packs;

    if (!bot || !bot->IsInWorld())
        return packs;

    // Find all hostile units nearby
    std::list<Unit*> hostiles;
    Acore::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, range);
    Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, hostiles, u_check);
    Cell::VisitObjects(bot, searcher, range);

    // Filter to only valid potential targets
    hostiles.remove_if([this](Unit* unit) {
        return !AttackersValue::IsPossibleTarget(unit, bot, range);
    });

    if (hostiles.size() < 2)
        return packs;

    // Group mobs into packs
    GroupMobsIntoPacks(hostiles, packs);

    return packs;
}

void MobPacksNearbyValue::GroupMobsIntoPacks(std::list<Unit*>& hostiles, std::vector<MobPackInfo>& packs)
{
    std::unordered_set<ObjectGuid> assigned;

    for (Unit* mob : hostiles)
    {
        if (assigned.find(mob->GetGUID()) != assigned.end())
            continue;

        // Start a new pack with this mob
        MobPackInfo pack;
        pack.members.push_back(mob->GetGUID());
        assigned.insert(mob->GetGUID());

        // Find all mobs within pack radius that form a connected cluster
        std::vector<Unit*> toCheck;
        toCheck.push_back(mob);

        while (!toCheck.empty())
        {
            Unit* current = toCheck.back();
            toCheck.pop_back();

            for (Unit* other : hostiles)
            {
                if (assigned.find(other->GetGUID()) != assigned.end())
                    continue;

                if (AreMobsInSamePack(current, other))
                {
                    pack.members.push_back(other->GetGUID());
                    assigned.insert(other->GetGUID());
                    toCheck.push_back(other);
                }
            }
        }

        // Only add packs with 2+ mobs
        if (pack.members.size() >= sPlayerbotAIConfig->travelAvoidPackMinSize)
        {
            CalculatePackStats(pack, hostiles);
            packs.push_back(pack);
        }
    }
}

void MobPacksNearbyValue::CalculatePackStats(MobPackInfo& pack, std::list<Unit*>& hostiles)
{
    if (pack.members.empty())
        return;

    float sumX = 0, sumY = 0, sumZ = 0;
    pack.highestLevel = 0;
    pack.lowestLevel = UINT32_MAX;

    std::vector<Unit*> packUnits;

    for (ObjectGuid guid : pack.members)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit)
            continue;

        packUnits.push_back(unit);
        sumX += unit->GetPositionX();
        sumY += unit->GetPositionY();
        sumZ += unit->GetPositionZ();

        uint32 level = unit->GetLevel();
        if (level > pack.highestLevel)
            pack.highestLevel = level;
        if (level < pack.lowestLevel)
            pack.lowestLevel = level;
    }

    if (packUnits.empty())
        return;

    pack.centerX = sumX / packUnits.size();
    pack.centerY = sumY / packUnits.size();
    pack.centerZ = sumZ / packUnits.size();

    // Calculate radius and find most isolated mob
    float maxDist = 0;
    float maxDistFromCenter = 0;

    for (Unit* unit : packUnits)
    {
        float distFromCenter = unit->GetDistance(pack.centerX, pack.centerY, pack.centerZ);
        if (distFromCenter > maxDistFromCenter)
            maxDistFromCenter = distFromCenter;

        // Find min distance to any other pack member (most isolated)
        float minDistToOther = FLT_MAX;
        for (Unit* other : packUnits)
        {
            if (unit == other)
                continue;

            float dist = unit->GetDistance(other);
            if (dist < minDistToOther)
                minDistToOther = dist;
        }

        if (minDistToOther > maxDist)
        {
            maxDist = minDistToOther;
            pack.mostIsolatedMob = unit->GetGUID();
        }
    }

    pack.radius = maxDistFromCenter;
}

bool MobPacksNearbyValue::AreMobsInSamePack(Unit* mob1, Unit* mob2)
{
    if (!mob1 || !mob2)
        return false;

    float dist = mob1->GetDistance(mob2);
    return dist <= packRadius;
}

bool MobPacksNearbyValue::IsMobInPack(ObjectGuid mobGuid)
{
    std::vector<MobPackInfo> packs = Calculate();
    for (const MobPackInfo& pack : packs)
    {
        for (ObjectGuid guid : pack.members)
        {
            if (guid == mobGuid)
                return true;
        }
    }
    return false;
}

MobPackInfo* MobPacksNearbyValue::GetPackForMob(ObjectGuid mobGuid)
{
    static std::vector<MobPackInfo> cachedPacks;
    cachedPacks = Calculate();

    for (MobPackInfo& pack : cachedPacks)
    {
        for (ObjectGuid guid : pack.members)
        {
            if (guid == mobGuid)
                return &pack;
        }
    }
    return nullptr;
}

std::vector<MobPackInfo> MobPacksNearbyValue::GetPacksInPath(float startX, float startY, float startZ,
                                                             float endX, float endY, float endZ,
                                                             float pathWidth)
{
    std::vector<MobPackInfo> blockedPacks;
    std::vector<MobPackInfo> allPacks = Calculate();

    // Direction vector from start to end
    float dirX = endX - startX;
    float dirY = endY - startY;
    float pathLength = sqrt(dirX * dirX + dirY * dirY);

    if (pathLength < 0.1f)
        return blockedPacks;

    // Normalize direction
    dirX /= pathLength;
    dirY /= pathLength;

    for (const MobPackInfo& pack : allPacks)
    {
        // Vector from start to pack center
        float toPackX = pack.centerX - startX;
        float toPackY = pack.centerY - startY;

        // Project onto path direction
        float projection = toPackX * dirX + toPackY * dirY;

        // Check if pack is along the path (not behind or past destination)
        if (projection < 0 || projection > pathLength)
            continue;

        // Perpendicular distance from pack center to path line
        float perpX = toPackX - projection * dirX;
        float perpY = toPackY - projection * dirY;
        float perpDist = sqrt(perpX * perpX + perpY * perpY);

        // Check if pack is close enough to block the path
        if (perpDist <= pathWidth + pack.radius + sPlayerbotAIConfig->aggroDistance)
        {
            blockedPacks.push_back(pack);
        }
    }

    return blockedPacks;
}

uint8 MobPackCountValue::Calculate()
{
    std::vector<MobPackInfo> packs =
        AI_VALUE(std::vector<MobPackInfo>, "mob packs nearby");
    return static_cast<uint8>(packs.size());
}

bool DangerousPackInPathValue::Calculate()
{
    if (!botAI->AllowActivity(TRAVEL_ACTIVITY))
        return false;

    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return false;

    WorldPosition botPos(bot);
    WorldPosition destPos = *target->getPosition();

    // Get packs blocking the path
    MobPacksNearbyValue* packValue = dynamic_cast<MobPacksNearbyValue*>(
        context->GetValue<std::vector<MobPackInfo>>("mob packs nearby"));
    if (!packValue)
        return false;

    std::vector<MobPackInfo> blockedPacks = packValue->GetPacksInPath(
        botPos.getX(), botPos.getY(), botPos.getZ(),
        destPos.getX(), destPos.getY(), destPos.getZ(),
        sPlayerbotAIConfig->aggroDistance);

    uint32 botLevel = bot->GetLevel();
    uint32 levelDiffThreshold = sPlayerbotAIConfig->travelAvoidLevelDiff;

    for (const MobPackInfo& pack : blockedPacks)
    {
        // Check if any mob in the pack is needed for a quest
        // If so, don't treat this pack as dangerous - we need to engage them
        bool hasQuestObjective = false;
        for (ObjectGuid guid : pack.members)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (unit && IsMobNeededForQuest(bot, unit))
            {
                hasQuestObjective = true;
                break;
            }
        }

        // Skip packs that contain quest objectives - we want to fight those
        if (hasQuestObjective)
            continue;

        // Check if pack is dangerous based on size
        if (pack.GetSize() >= sPlayerbotAIConfig->travelAvoidPackMinSize)
            return true;

        // Check if any mob is too high level
        if (pack.highestLevel > botLevel + levelDiffThreshold)
            return true;
    }

    return false;
}

Unit* BestPullTargetFromPackValue::Calculate()
{
    if (!botAI->AllowActivity(TRAVEL_ACTIVITY))
        return nullptr;

    TravelTarget* target = AI_VALUE(TravelTarget*, "travel target");
    if (!target || !target->isTraveling())
        return nullptr;

    WorldPosition botPos(bot);
    WorldPosition destPos = *target->getPosition();

    MobPacksNearbyValue* packValue = dynamic_cast<MobPacksNearbyValue*>(
        context->GetValue<std::vector<MobPackInfo>>("mob packs nearby"));
    if (!packValue)
        return nullptr;

    std::vector<MobPackInfo> blockedPacks = packValue->GetPacksInPath(
        botPos.getX(), botPos.getY(), botPos.getZ(),
        destPos.getX(), destPos.getY(), destPos.getZ(),
        sPlayerbotAIConfig->aggroDistance);

    if (blockedPacks.empty())
        return nullptr;

    uint32 botLevel = bot->GetLevel();
    uint32 pullMaxPackSize = sPlayerbotAIConfig->travelPullMaxPackSize;
    uint32 pullLevelLimit = sPlayerbotAIConfig->travelPullLevelLimit;

    // Find the first pack we can safely pull from
    for (const MobPackInfo& pack : blockedPacks)
    {
        // Skip packs that are too large
        if (pack.GetSize() > pullMaxPackSize)
            continue;

        // Skip if any mob is too high level
        if (pack.highestLevel > botLevel + pullLevelLimit)
            continue;

        // Find best target: most isolated AND lowest level
        Unit* bestTarget = nullptr;
        float bestScore = -1.0f;

        for (ObjectGuid guid : pack.members)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (!unit || !unit->IsAlive())
                continue;

            // Calculate isolation score (distance from pack center)
            float distFromCenter = unit->GetDistance(pack.centerX, pack.centerY, pack.centerZ);

            // Prefer lower level and more isolated
            float levelPenalty = static_cast<float>(unit->GetLevel() - pack.lowestLevel);
            float score = distFromCenter - (levelPenalty * 5.0f);

            if (score > bestScore)
            {
                bestScore = score;
                bestTarget = unit;
            }
        }

        if (bestTarget)
            return bestTarget;
    }

    return nullptr;
}
