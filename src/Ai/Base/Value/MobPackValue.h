/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_MOBPACKVALUE_H
#define _PLAYERBOT_MOBPACKVALUE_H

#include "PlayerbotAIConfig.h"
#include "Value.h"

class PlayerbotAI;
class Unit;

/**
 * @brief Information about a detected mob pack
 */
struct MobPackInfo
{
    GuidVector members;           // GUIDs of all mobs in this pack
    float centerX;                // Center X coordinate of the pack
    float centerY;                // Center Y coordinate of the pack
    float centerZ;                // Center Z coordinate of the pack
    uint32 highestLevel;          // Highest level mob in the pack
    uint32 lowestLevel;           // Lowest level mob in the pack
    float radius;                 // Approximate radius of the pack
    ObjectGuid mostIsolatedMob;   // The mob furthest from pack center

    MobPackInfo() : centerX(0), centerY(0), centerZ(0), highestLevel(0), lowestLevel(0), radius(0) {}

    uint32 GetSize() const { return static_cast<uint32>(members.size()); }
    bool IsEmpty() const { return members.empty(); }
};

/**
 * @brief Value that detects clusters of hostile mobs that form "packs"
 *
 * A pack is defined as 2 or more hostile mobs within a configurable distance
 * of each other. This value scans for hostile mobs and groups them by proximity.
 */
class MobPacksNearbyValue : public CalculatedValue<std::vector<MobPackInfo>>
{
public:
    MobPacksNearbyValue(PlayerbotAI* botAI, std::string const name = "mob packs nearby",
                        float range = sPlayerbotAIConfig->sightDistance);

    std::vector<MobPackInfo> Calculate() override;

    /**
     * @brief Check if a specific mob is part of any detected pack
     */
    bool IsMobInPack(ObjectGuid mobGuid);

    /**
     * @brief Get the pack that contains a specific mob
     */
    MobPackInfo* GetPackForMob(ObjectGuid mobGuid);

    /**
     * @brief Get packs that are blocking a path from start to end
     */
    std::vector<MobPackInfo> GetPacksInPath(float startX, float startY, float startZ,
                                            float endX, float endY, float endZ,
                                            float pathWidth = 10.0f);

private:
    float range;
    float packRadius;  // Distance threshold for mobs to be considered part of same pack

    /**
     * @brief Group nearby hostile mobs into packs based on proximity
     */
    void GroupMobsIntoPacks(std::list<Unit*>& hostiles, std::vector<MobPackInfo>& packs);

    /**
     * @brief Calculate pack center and statistics
     */
    void CalculatePackStats(MobPackInfo& pack, std::list<Unit*>& hostiles);

    /**
     * @brief Check if two mobs are close enough to be in same pack
     */
    bool AreMobsInSamePack(Unit* mob1, Unit* mob2);
};

/**
 * @brief Value that returns the count of hostile mob packs near the bot
 */
class MobPackCountValue : public Uint8CalculatedValue
{
public:
    MobPackCountValue(PlayerbotAI* botAI) : Uint8CalculatedValue(botAI, "mob pack count", 2 * 1000) {}

    uint8 Calculate() override;
};

/**
 * @brief Value that checks if there's a dangerous pack in the travel path
 */
class DangerousPackInPathValue : public BoolCalculatedValue
{
public:
    DangerousPackInPathValue(PlayerbotAI* botAI)
        : BoolCalculatedValue(botAI, "dangerous pack in path", 1 * 1000) {}

    bool Calculate() override;
};

/**
 * @brief Value that returns the best mob to pull from a pack (most isolated, lowest level)
 */
class BestPullTargetFromPackValue : public UnitCalculatedValue
{
public:
    BestPullTargetFromPackValue(PlayerbotAI* botAI)
        : UnitCalculatedValue(botAI, "best pull target from pack", 1 * 1000) {}

    Unit* Calculate() override;
};

#endif
