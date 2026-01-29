/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#ifndef MOD_CORE_QUEST_DATA_MGR_H
#define MOD_CORE_QUEST_DATA_MGR_H

#include "Bot/Interface/ICoreQuestDataMgr.h"
#include <unordered_map>

class Player;

class CoreQuestDataMgr : public ICoreQuestDataMgr
{
public:
    static CoreQuestDataMgr* instance();

    void LoadData() override;
    bool IsDataLoaded() const override { return _dataLoaded; }

    // POI fallback - main integration point
    bool GetQuestPOI(uint32 questId, int32 objectiveIndex, uint32 mapId,
                     std::vector<SpawnPoint>& positions) override;

    // Spawn lookups
    std::vector<SpawnPoint> GetNpcSpawns(uint32 npcId, uint32 mapId = 0) override;
    std::vector<SpawnPoint> GetObjectSpawns(uint32 objectId, uint32 mapId = 0) override;
    std::vector<SpawnPoint> GetQuestStarterLocations(uint32 questId, uint32 mapId = 0) override;
    std::vector<SpawnPoint> GetQuestEnderLocations(uint32 questId, uint32 mapId = 0) override;

    // Quest chain/prereq awareness (using core Quest data)
    bool HasPrerequisites(uint32 questId) const override;
    bool ArePrerequisitesMet(Player* player, uint32 questId) const override;
    std::vector<uint32> GetPrerequisiteQuests(uint32 questId) const override;
    uint32 GetNextQuestInChain(uint32 questId) const override;

    // Quest recommendations
    std::vector<uint32> GetRecommendedQuests(Player* player, uint32 zoneId, uint32 limit = 10) override;

    // Approach waypoints (for cave navigation) - loaded from DB
    std::vector<ApproachWaypoint> GetApproachWaypoints(ApproachWaypointTargetType type, uint32 targetId, uint32 mapId = 0) override;
    std::vector<ApproachWaypoint> GetZoneApproachWaypoints(uint32 zoneId, uint32 mapId = 0) override;
    std::vector<ApproachWaypoint> GetNpcApproachWaypoints(uint32 npcId, uint32 mapId = 0) override;
    std::vector<ApproachWaypoint> GetObjectApproachWaypoints(uint32 objectId, uint32 mapId = 0) override;
    std::vector<ApproachWaypoint> GetQuestApproachWaypoints(uint32 questId, uint32 mapId = 0) override;

    // Statistics
    uint32 GetNpcSpawnCount() const override { return _npcSpawnCount; }
    uint32 GetObjectSpawnCount() const override { return _objectSpawnCount; }
    uint32 GetApproachWaypointCount() const override { return _approachWaypointCount; }

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
