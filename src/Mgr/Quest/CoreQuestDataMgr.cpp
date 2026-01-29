/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#include "CoreQuestDataMgr.h"
#include "BetterQuestingConfig.h"
#include "Bot/Core/ManagerRegistry.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include <algorithm>

CoreQuestDataMgr* CoreQuestDataMgr::instance()
{
    static CoreQuestDataMgr instance;
    return &instance;
}

void CoreQuestDataMgr::LoadData()
{
    if (!sManagerRegistry.HasBetterQuestingConfig() || !sManagerRegistry.GetBetterQuestingConfig().IsEnabled())
    {
        LOG_INFO("module", "BetterQuesting: Module disabled, skipping data load");
        return;
    }

    uint32 oldMSTime = getMSTime();

    // Build indexes from AzerothCore's ObjectMgr
    BuildNpcSpawnIndex();
    BuildObjectSpawnIndex();
    BuildQuestRelationIndexes();

    // Load approach waypoints from database (bot-specific navigation data)
    LoadApproachWaypoints();

    _dataLoaded = true;

    LOG_INFO("module", "BetterQuesting: Indexed {} NPC spawns, {} object spawns, loaded {} approach waypoints in {} ms",
        _npcSpawnCount, _objectSpawnCount, _approachWaypointCount, GetMSTimeDiffToNow(oldMSTime));
}

void CoreQuestDataMgr::BuildNpcSpawnIndex()
{
    // Build NPC spawn index from creature table (already loaded by core)
    for (auto const& [guid, data] : sObjectMgr->GetAllCreatureData())
    {
        SpawnPoint spawn(data.mapid, data.posX, data.posY, data.posZ);
        _npcSpawns.emplace(data.id1, spawn);
        ++_npcSpawnCount;
    }

    LOG_DEBUG("module", "BetterQuesting: Indexed {} NPC spawn points", _npcSpawnCount);
}

void CoreQuestDataMgr::BuildObjectSpawnIndex()
{
    // Build GameObject spawn index from gameobject table (already loaded by core)
    for (auto const& [guid, data] : sObjectMgr->GetAllGOData())
    {
        SpawnPoint spawn(data.mapid, data.posX, data.posY, data.posZ);
        _objectSpawns.emplace(data.id, spawn);
        ++_objectSpawnCount;
    }

    LOG_DEBUG("module", "BetterQuesting: Indexed {} object spawn points", _objectSpawnCount);
}

void CoreQuestDataMgr::BuildQuestRelationIndexes()
{
    // Build quest starter/ender indexes from ObjectMgr's quest relation maps
    // These maps are: entry -> questId, we need reverse lookup: questId -> entry

    // Creature quest starters
    for (auto const& [creatureEntry, questId] : *sObjectMgr->GetCreatureQuestRelationMap())
    {
        _questStarterNpcs.emplace(questId, creatureEntry);
    }

    // Creature quest enders
    for (auto const& [creatureEntry, questId] : *sObjectMgr->GetCreatureQuestInvolvedRelationMap())
    {
        _questEnderNpcs.emplace(questId, creatureEntry);
    }

    // GameObject quest starters
    for (auto const& [goEntry, questId] : *sObjectMgr->GetGOQuestRelationMap())
    {
        _questStarterObjects.emplace(questId, goEntry);
    }

    // GameObject quest enders
    for (auto const& [goEntry, questId] : *sObjectMgr->GetGOQuestInvolvedRelationMap())
    {
        _questEnderObjects.emplace(questId, goEntry);
    }

    LOG_DEBUG("module", "BetterQuesting: Built quest relation indexes - {} NPC starters, {} NPC enders, {} object starters, {} object enders",
        _questStarterNpcs.size(), _questEnderNpcs.size(), _questStarterObjects.size(), _questEnderObjects.size());
}

void CoreQuestDataMgr::LoadApproachWaypoints()
{
    QueryResult result = WorldDatabase.Query("SELECT waypointId, targetType, targetId, mapId, "
        "waypointOrder, x, y, z, radius, description "
        "FROM playerbots_questing_approach_waypoints "
        "ORDER BY targetType, targetId, waypointOrder");

    if (!result)
    {
        LOG_DEBUG("module", "BetterQuesting: No approach waypoints found (optional table)");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        ApproachWaypoint waypoint;
        waypoint.waypointId = fields[0].Get<uint32>();
        waypoint.targetType = static_cast<ApproachWaypointTargetType>(fields[1].Get<uint8>());
        waypoint.targetId = fields[2].Get<uint32>();
        waypoint.mapId = fields[3].Get<uint32>();
        waypoint.waypointOrder = fields[4].Get<uint16>();
        waypoint.x = fields[5].Get<float>();
        waypoint.y = fields[6].Get<float>();
        waypoint.z = fields[7].Get<float>();
        waypoint.radius = fields[8].Get<float>();
        waypoint.description = fields[9].Get<std::string>();

        ApproachWaypointKey key;
        key.targetType = waypoint.targetType;
        key.targetId = waypoint.targetId;

        _approachWaypoints.emplace(key, waypoint);
        ++_approachWaypointCount;
    }
    while (result->NextRow());

    LOG_DEBUG("module", "BetterQuesting: Loaded {} approach waypoints", _approachWaypointCount);
}

bool CoreQuestDataMgr::GetQuestPOI(uint32 questId, int32 objectiveIndex, uint32 mapId,
                                    std::vector<SpawnPoint>& positions)
{
    if (!_dataLoaded)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // For objectiveIndex < 0, we're looking for quest completion (turn-in)
    if (objectiveIndex < 0)
    {
        positions = GetQuestEnderLocations(questId, mapId);
        return !positions.empty();
    }

    // For objectives, check the quest requirements
    if (objectiveIndex < QUEST_OBJECTIVES_COUNT)
    {
        // Check for required NPC kills/interactions
        if (quest->RequiredNpcOrGo[objectiveIndex] > 0)
        {
            uint32 npcId = quest->RequiredNpcOrGo[objectiveIndex];
            positions = GetNpcSpawns(npcId, mapId);
            if (!positions.empty())
                return true;
        }
        // Check for required game objects
        else if (quest->RequiredNpcOrGo[objectiveIndex] < 0)
        {
            uint32 objectId = -quest->RequiredNpcOrGo[objectiveIndex];
            positions = GetObjectSpawns(objectId, mapId);
            if (!positions.empty())
                return true;
        }

        // Check for required items (look for sources)
        if (quest->RequiredItemId[objectiveIndex] > 0)
        {
            // Items can come from NPCs or objects - this is more complex
            // For now, we return false and let the caller handle it
            // In the future, we could add item source tracking
        }
    }

    return false;
}

std::vector<SpawnPoint> CoreQuestDataMgr::GetNpcSpawns(uint32 npcId, uint32 mapId)
{
    std::vector<SpawnPoint> result;
    auto range = _npcSpawns.equal_range(npcId);

    for (auto it = range.first; it != range.second; ++it)
    {
        if (mapId == 0 || it->second.mapId == mapId)
        {
            result.push_back(it->second);
        }
    }

    return result;
}

std::vector<SpawnPoint> CoreQuestDataMgr::GetObjectSpawns(uint32 objectId, uint32 mapId)
{
    std::vector<SpawnPoint> result;
    auto range = _objectSpawns.equal_range(objectId);

    for (auto it = range.first; it != range.second; ++it)
    {
        if (mapId == 0 || it->second.mapId == mapId)
        {
            result.push_back(it->second);
        }
    }

    return result;
}

std::vector<SpawnPoint> CoreQuestDataMgr::GetQuestStarterLocations(uint32 questId, uint32 mapId)
{
    std::vector<SpawnPoint> result;

    // Get NPC starter locations
    auto npcRange = _questStarterNpcs.equal_range(questId);
    for (auto it = npcRange.first; it != npcRange.second; ++it)
    {
        auto spawns = GetNpcSpawns(it->second, mapId);
        result.insert(result.end(), spawns.begin(), spawns.end());
    }

    // Get Object starter locations
    auto objRange = _questStarterObjects.equal_range(questId);
    for (auto it = objRange.first; it != objRange.second; ++it)
    {
        auto spawns = GetObjectSpawns(it->second, mapId);
        result.insert(result.end(), spawns.begin(), spawns.end());
    }

    return result;
}

std::vector<SpawnPoint> CoreQuestDataMgr::GetQuestEnderLocations(uint32 questId, uint32 mapId)
{
    std::vector<SpawnPoint> result;

    // Get NPC ender locations
    auto npcRange = _questEnderNpcs.equal_range(questId);
    for (auto it = npcRange.first; it != npcRange.second; ++it)
    {
        auto spawns = GetNpcSpawns(it->second, mapId);
        result.insert(result.end(), spawns.begin(), spawns.end());
    }

    // Get Object ender locations
    auto objRange = _questEnderObjects.equal_range(questId);
    for (auto it = objRange.first; it != objRange.second; ++it)
    {
        auto spawns = GetObjectSpawns(it->second, mapId);
        result.insert(result.end(), spawns.begin(), spawns.end());
    }

    return result;
}

bool CoreQuestDataMgr::HasPrerequisites(uint32 questId) const
{
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check if quest has any prerequisites via core Quest data
    return quest->GetPrevQuestId() != 0;
}

bool CoreQuestDataMgr::ArePrerequisitesMet(Player* player, uint32 questId) const
{
    if (!player)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Check previous quest requirement
    int32 prevQuestId = quest->GetPrevQuestId();
    if (prevQuestId != 0)
    {
        // Positive means must be completed, negative means must NOT be completed
        if (prevQuestId > 0)
        {
            if (!player->GetQuestRewardStatus(prevQuestId))
                return false;
        }
        else
        {
            if (player->GetQuestRewardStatus(-prevQuestId))
                return false;
        }
    }

    // Check exclusive group (mutually exclusive quests)
    int32 exclusiveGroup = quest->GetExclusiveGroup();
    if (exclusiveGroup != 0)
    {
        // Check if any quest in the exclusive group has been completed
        ObjectMgr::QuestMap const& questTemplates = sObjectMgr->GetQuestTemplates();
        for (auto const& [otherQuestId, otherQuest] : questTemplates)
        {
            if (otherQuest->GetExclusiveGroup() == exclusiveGroup &&
                otherQuestId != questId &&
                player->GetQuestRewardStatus(otherQuestId))
            {
                return false;
            }
        }
    }

    return true;
}

std::vector<uint32> CoreQuestDataMgr::GetPrerequisiteQuests(uint32 questId) const
{
    std::vector<uint32> result;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return result;

    int32 prevQuestId = quest->GetPrevQuestId();
    if (prevQuestId > 0)
    {
        result.push_back(static_cast<uint32>(prevQuestId));
    }

    return result;
}

uint32 CoreQuestDataMgr::GetNextQuestInChain(uint32 questId) const
{
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return 0;

    // Check RewardNextQuest first (quest given on completion)
    uint32 rewardNext = quest->GetNextQuestInChain();
    if (rewardNext != 0)
        return rewardNext;

    // Fall back to NextQuestId
    int32 nextQuestId = quest->GetNextQuestId();
    if (nextQuestId > 0)
        return static_cast<uint32>(nextQuestId);

    return 0;
}

std::vector<uint32> CoreQuestDataMgr::GetRecommendedQuests(Player* player, uint32 zoneId, uint32 limit)
{
    std::vector<uint32> result;

    if (!player || !_dataLoaded)
        return result;

    if (!sManagerRegistry.HasBetterQuestingConfig())
        return result;

    auto& config = sManagerRegistry.GetBetterQuestingConfig();

    uint8 playerLevel = player->GetLevel();
    uint32 playerRaceMask = player->getRaceMask();
    uint32 playerClassMask = player->getClassMask();

    struct QuestScore
    {
        uint32 questId;
        int32 score;
    };
    std::vector<QuestScore> scoredQuests;

    ObjectMgr::QuestMap const& questTemplates = sObjectMgr->GetQuestTemplates();

    for (auto const& [questId, quest] : questTemplates)
    {
        // Skip if player doesn't meet level requirement
        if (quest->GetMinLevel() > playerLevel)
            continue;

        // Skip if race doesn't match (0 means all races)
        uint32 requiredRaces = quest->GetAllowableRaces();
        if (requiredRaces != 0 && !(requiredRaces & playerRaceMask))
            continue;

        // Skip if class doesn't match (0 means all classes)
        uint32 requiredClasses = quest->GetRequiredClasses();
        if (requiredClasses != 0 && !(requiredClasses & playerClassMask))
            continue;

        // Skip if player already has or completed quest
        if (player->GetQuestStatus(questId) != QUEST_STATUS_NONE)
            continue;

        // Skip if prerequisites not met
        if (config.IsPrerequisiteCheckEnabled() &&
            !ArePrerequisitesMet(player, questId))
            continue;

        // Calculate score based on level difference
        int32 questLevel = quest->GetQuestLevel();
        if (questLevel < 0)
            questLevel = playerLevel; // Scaling quest

        int32 levelDiff = playerLevel - questLevel;
        int32 score = 100;

        // Prefer quests close to player level
        if (levelDiff < -2)
            score -= ((-levelDiff - 2) * 20); // Too high level
        else if (levelDiff > 5)
            score -= ((levelDiff - 5) * 15); // Too low level (gray quests)

        // Bonus for quests in the same zone
        int32 questZone = quest->GetZoneOrSort();
        if (questZone > 0 && static_cast<uint32>(questZone) == zoneId)
            score += 30;

        // Bonus for chain quests (continuity)
        if (config.IsQuestChainAwarenessEnabled())
        {
            // Check if this quest continues a chain the player completed
            int32 prevQuestId = quest->GetPrevQuestId();
            if (prevQuestId > 0 && player->GetQuestRewardStatus(prevQuestId))
            {
                score += 25; // Bonus for being part of an active chain
            }
        }

        // Add guide priority bonus
        if (config.IsQuestGuideEnabled() && sManagerRegistry.HasQuestGuideMgr())
        {
            auto& guideMgr = sManagerRegistry.GetQuestGuideMgr();
            if (guideMgr.IsDataLoaded())
            {
                int32 guideBonus = guideMgr.GetGuideQuestPriorityBonus(player, questId);
                score += guideBonus;

                // In strict mode, filter out non-guide quests
                if (config.IsQuestGuideStrictMode() && guideBonus == 0)
                {
                    if (guideMgr.GetPlayerProgress(player->GetGUID().GetCounter()) != nullptr)
                    {
                        continue;  // Skip non-guide quests in strict mode
                    }
                }
            }
        }

        if (score > 0)
        {
            scoredQuests.push_back({questId, score});
        }
    }

    // Sort by score descending
    std::sort(scoredQuests.begin(), scoredQuests.end(),
        [](const QuestScore& a, const QuestScore& b) { return a.score > b.score; });

    // Return top results
    for (size_t i = 0; i < scoredQuests.size() && i < limit; ++i)
    {
        result.push_back(scoredQuests[i].questId);
    }

    return result;
}

std::vector<ApproachWaypoint> CoreQuestDataMgr::GetApproachWaypoints(ApproachWaypointTargetType type, uint32 targetId, uint32 mapId)
{
    std::vector<ApproachWaypoint> result;

    ApproachWaypointKey key;
    key.targetType = type;
    key.targetId = targetId;

    auto range = _approachWaypoints.equal_range(key);
    for (auto it = range.first; it != range.second; ++it)
    {
        // Filter by map if specified
        if (mapId == 0 || it->second.mapId == mapId)
        {
            result.push_back(it->second);
        }
    }

    // Sort by waypoint order
    std::sort(result.begin(), result.end(), [](const ApproachWaypoint& a, const ApproachWaypoint& b) {
        return a.waypointOrder < b.waypointOrder;
    });

    return result;
}

std::vector<ApproachWaypoint> CoreQuestDataMgr::GetZoneApproachWaypoints(uint32 zoneId, uint32 mapId)
{
    return GetApproachWaypoints(APPROACH_TARGET_ZONE, zoneId, mapId);
}

std::vector<ApproachWaypoint> CoreQuestDataMgr::GetNpcApproachWaypoints(uint32 npcId, uint32 mapId)
{
    return GetApproachWaypoints(APPROACH_TARGET_NPC, npcId, mapId);
}

std::vector<ApproachWaypoint> CoreQuestDataMgr::GetObjectApproachWaypoints(uint32 objectId, uint32 mapId)
{
    return GetApproachWaypoints(APPROACH_TARGET_OBJECT, objectId, mapId);
}

std::vector<ApproachWaypoint> CoreQuestDataMgr::GetQuestApproachWaypoints(uint32 questId, uint32 mapId)
{
    return GetApproachWaypoints(APPROACH_TARGET_QUEST, questId, mapId);
}
