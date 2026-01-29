/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_ICORE_QUEST_DATA_MGR_H
#define _PLAYERBOT_ICORE_QUEST_DATA_MGR_H

#include "Mgr/Quest/BetterQuestingTypes.h"
#include <vector>

class Player;

/**
 * @brief Interface for quest data indexing and lookup
 *
 * This interface abstracts access to quest spawn data, POI fallback,
 * quest chains, and approach waypoints.
 */
class ICoreQuestDataMgr
{
public:
    virtual ~ICoreQuestDataMgr() = default;

    // Data loading
    virtual void LoadData() = 0;
    virtual bool IsDataLoaded() const = 0;

    // POI fallback - main integration point
    virtual bool GetQuestPOI(uint32 questId, int32 objectiveIndex, uint32 mapId,
                             std::vector<SpawnPoint>& positions) = 0;

    // Spawn lookups
    virtual std::vector<SpawnPoint> GetNpcSpawns(uint32 npcId, uint32 mapId = 0) = 0;
    virtual std::vector<SpawnPoint> GetObjectSpawns(uint32 objectId, uint32 mapId = 0) = 0;
    virtual std::vector<SpawnPoint> GetQuestStarterLocations(uint32 questId, uint32 mapId = 0) = 0;
    virtual std::vector<SpawnPoint> GetQuestEnderLocations(uint32 questId, uint32 mapId = 0) = 0;

    // Quest chain/prereq awareness
    virtual bool HasPrerequisites(uint32 questId) const = 0;
    virtual bool ArePrerequisitesMet(Player* player, uint32 questId) const = 0;
    virtual std::vector<uint32> GetPrerequisiteQuests(uint32 questId) const = 0;
    virtual uint32 GetNextQuestInChain(uint32 questId) const = 0;

    // Quest recommendations
    virtual std::vector<uint32> GetRecommendedQuests(Player* player, uint32 zoneId, uint32 limit = 10) = 0;

    // Approach waypoints (for cave navigation)
    virtual std::vector<ApproachWaypoint> GetApproachWaypoints(ApproachWaypointTargetType type,
                                                               uint32 targetId, uint32 mapId = 0) = 0;
    virtual std::vector<ApproachWaypoint> GetZoneApproachWaypoints(uint32 zoneId, uint32 mapId = 0) = 0;
    virtual std::vector<ApproachWaypoint> GetNpcApproachWaypoints(uint32 npcId, uint32 mapId = 0) = 0;
    virtual std::vector<ApproachWaypoint> GetObjectApproachWaypoints(uint32 objectId, uint32 mapId = 0) = 0;
    virtual std::vector<ApproachWaypoint> GetQuestApproachWaypoints(uint32 questId, uint32 mapId = 0) = 0;

    // Statistics
    virtual uint32 GetNpcSpawnCount() const = 0;
    virtual uint32 GetObjectSpawnCount() const = 0;
    virtual uint32 GetApproachWaypointCount() const = 0;
};

#endif // _PLAYERBOT_ICORE_QUEST_DATA_MGR_H
