/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#ifndef MOD_CORE_QUEST_DATA_MGR_H
#define MOD_CORE_QUEST_DATA_MGR_H

#include "Common.h"
#include <unordered_map>
#include <vector>

class Player;

// Simple position structure for spawn locations (world coordinates)
struct SpawnPoint
{
    uint32 mapId;
    float x;  // World X coordinate
    float y;  // World Y coordinate
    float z;  // World Z coordinate

    SpawnPoint(uint32 map = 0, float posX = 0.0f, float posY = 0.0f, float posZ = 0.0f)
        : mapId(map), x(posX), y(posY), z(posZ) {}
};

// Approach waypoint target types
enum ApproachWaypointTargetType : uint8
{
    APPROACH_TARGET_ZONE    = 0,  // Zone-level waypoints
    APPROACH_TARGET_NPC     = 1,  // NPC-specific waypoints
    APPROACH_TARGET_OBJECT  = 2,  // Object-specific waypoints
    APPROACH_TARGET_QUEST   = 3,  // Quest-specific waypoints
    APPROACH_TARGET_AREA    = 4   // Area-specific waypoints
};

// Approach waypoint for difficult navigation (caves, etc.)
struct ApproachWaypoint
{
    uint32 waypointId;
    ApproachWaypointTargetType targetType;
    uint32 targetId;
    uint32 mapId;
    uint16 waypointOrder;
    float x;
    float y;
    float z;
    float radius;
    std::string description;
};

// Key for looking up approach waypoints
struct ApproachWaypointKey
{
    ApproachWaypointTargetType targetType;
    uint32 targetId;

    bool operator==(const ApproachWaypointKey& other) const
    {
        return targetType == other.targetType && targetId == other.targetId;
    }
};

// Hash function for ApproachWaypointKey
struct ApproachWaypointKeyHash
{
    std::size_t operator()(const ApproachWaypointKey& key) const
    {
        return std::hash<uint32>()(key.targetId) ^ (std::hash<uint8>()(key.targetType) << 16);
    }
};

class CoreQuestDataMgr
{
public:
    static CoreQuestDataMgr* instance();

    void LoadData();
    bool IsDataLoaded() const { return _dataLoaded; }

    // POI fallback - main integration point
    bool GetQuestPOI(uint32 questId, int32 objectiveIndex, uint32 mapId,
                     std::vector<SpawnPoint>& positions);

    // Spawn lookups
    std::vector<SpawnPoint> GetNpcSpawns(uint32 npcId, uint32 mapId = 0);
    std::vector<SpawnPoint> GetObjectSpawns(uint32 objectId, uint32 mapId = 0);
    std::vector<SpawnPoint> GetQuestStarterLocations(uint32 questId, uint32 mapId = 0);
    std::vector<SpawnPoint> GetQuestEnderLocations(uint32 questId, uint32 mapId = 0);

    // Quest chain/prereq awareness (using core Quest data)
    bool HasPrerequisites(uint32 questId) const;
    bool ArePrerequisitesMet(Player* player, uint32 questId) const;
    std::vector<uint32> GetPrerequisiteQuests(uint32 questId) const;
    uint32 GetNextQuestInChain(uint32 questId) const;

    // Quest recommendations
    std::vector<uint32> GetRecommendedQuests(Player* player, uint32 zoneId, uint32 limit = 10);

    // Approach waypoints (for cave navigation) - loaded from DB
    std::vector<ApproachWaypoint> GetApproachWaypoints(ApproachWaypointTargetType type, uint32 targetId, uint32 mapId = 0);
    std::vector<ApproachWaypoint> GetZoneApproachWaypoints(uint32 zoneId, uint32 mapId = 0);
    std::vector<ApproachWaypoint> GetNpcApproachWaypoints(uint32 npcId, uint32 mapId = 0);
    std::vector<ApproachWaypoint> GetObjectApproachWaypoints(uint32 objectId, uint32 mapId = 0);
    std::vector<ApproachWaypoint> GetQuestApproachWaypoints(uint32 questId, uint32 mapId = 0);

    // Statistics
    uint32 GetNpcSpawnCount() const { return _npcSpawnCount; }
    uint32 GetObjectSpawnCount() const { return _objectSpawnCount; }
    uint32 GetApproachWaypointCount() const { return _approachWaypointCount; }

private:
    CoreQuestDataMgr() = default;
    ~CoreQuestDataMgr() = default;

    void BuildNpcSpawnIndex();
    void BuildObjectSpawnIndex();
    void BuildQuestRelationIndexes();
    void LoadApproachWaypoints();

    bool _dataLoaded = false;
    uint32 _npcSpawnCount = 0;
    uint32 _objectSpawnCount = 0;
    uint32 _approachWaypointCount = 0;

    // NPC spawns indexed by npcId (entry)
    std::unordered_multimap<uint32, SpawnPoint> _npcSpawns;

    // Object spawns indexed by objectId (entry)
    std::unordered_multimap<uint32, SpawnPoint> _objectSpawns;

    // Quest starters indexed by questId -> NPC entries
    std::unordered_multimap<uint32, uint32> _questStarterNpcs;

    // Quest starters indexed by questId -> Object entries
    std::unordered_multimap<uint32, uint32> _questStarterObjects;

    // Quest enders indexed by questId -> NPC entries
    std::unordered_multimap<uint32, uint32> _questEnderNpcs;

    // Quest enders indexed by questId -> Object entries
    std::unordered_multimap<uint32, uint32> _questEnderObjects;

    // Approach waypoints for cave navigation, indexed by (targetType, targetId)
    std::unordered_multimap<ApproachWaypointKey, ApproachWaypoint, ApproachWaypointKeyHash> _approachWaypoints;
};

#define sCoreQuestDataMgr CoreQuestDataMgr::instance()

#endif // MOD_CORE_QUEST_DATA_MGR_H
