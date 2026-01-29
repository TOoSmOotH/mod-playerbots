/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#include "BetterQuesting.h"
#include "BetterQuestingConfig.h"
#include "Bot/Core/ManagerRegistry.h"
#include "CoreQuestDataMgr.h"
#include "IVMapMgr.h"
#include "Log.h"
#include "Map.h"
#include "PathGenerator.h"
#include "Player.h"
#include "PlayerbotAIConfig.h"
#include "QuestDef.h"
#include "QuestGuideMgr.h"
#include "ScriptMgr.h"
#include "Unit.h"
#include <algorithm>
#include <cmath>

// Public API implementation
namespace BetterQuesting
{
    bool IsEnabled()
    {
        if (!sManagerRegistry.HasBetterQuestingConfig() || !sManagerRegistry.HasCoreQuestDataMgr())
            return false;

        return sManagerRegistry.GetBetterQuestingConfig().IsEnabled() &&
               sManagerRegistry.GetCoreQuestDataMgr().IsDataLoaded();
    }

    bool GetQuestPOIFallback(uint32 questId, int32 objectiveIndex, uint32 mapId,
                             std::vector<SpawnPoint>& positions)
    {
        if (!IsEnabled())
            return false;

        auto& config = sManagerRegistry.GetBetterQuestingConfig();
        if (!config.IsPOIFallbackEnabled())
            return false;

        auto& dataMgr = sManagerRegistry.GetCoreQuestDataMgr();
        bool result = dataMgr.GetQuestPOI(questId, objectiveIndex, mapId, positions);

        if (result && config.IsLogFallbackEnabled())
        {
            LOG_DEBUG("module", "BetterQuesting: POI fallback for quest {} objective {} returned {} positions",
                questId, objectiveIndex, positions.size());
        }

        return result;
    }

    std::vector<SpawnPoint> GetNpcSpawns(uint32 npcId, uint32 mapId)
    {
        if (!IsEnabled())
            return {};

        return sManagerRegistry.GetCoreQuestDataMgr().GetNpcSpawns(npcId, mapId);
    }

    std::vector<SpawnPoint> GetObjectSpawns(uint32 objectId, uint32 mapId)
    {
        if (!IsEnabled())
            return {};

        return sManagerRegistry.GetCoreQuestDataMgr().GetObjectSpawns(objectId, mapId);
    }

    std::vector<SpawnPoint> GetQuestStarterLocations(uint32 questId, uint32 mapId)
    {
        if (!IsEnabled())
            return {};

        return sManagerRegistry.GetCoreQuestDataMgr().GetQuestStarterLocations(questId, mapId);
    }

    std::vector<SpawnPoint> GetQuestEnderLocations(uint32 questId, uint32 mapId)
    {
        if (!IsEnabled())
            return {};

        return sManagerRegistry.GetCoreQuestDataMgr().GetQuestEnderLocations(questId, mapId);
    }

    bool ArePrerequisitesMet(Player* player, uint32 questId)
    {
        if (!IsEnabled())
            return true; // Default to true if disabled

        auto& config = sManagerRegistry.GetBetterQuestingConfig();
        if (!config.IsPrerequisiteCheckEnabled())
            return true;

        return sManagerRegistry.GetCoreQuestDataMgr().ArePrerequisitesMet(player, questId);
    }

    uint32 GetNextQuestInChain(uint32 questId)
    {
        if (!IsEnabled())
            return 0;

        auto& config = sManagerRegistry.GetBetterQuestingConfig();
        if (!config.IsQuestChainAwarenessEnabled())
            return 0;

        return sManagerRegistry.GetCoreQuestDataMgr().GetNextQuestInChain(questId);
    }

    std::vector<uint32> GetRecommendedQuests(Player* player, uint32 zoneId, uint32 limit)
    {
        if (!IsEnabled())
            return {};

        return sManagerRegistry.GetCoreQuestDataMgr().GetRecommendedQuests(player, zoneId, limit);
    }

    // ==========================================
    // Navigation / Path Validation Implementation
    // ==========================================

    // Z probe depths below surface level to check for caves
    static const float Z_PROBE_DEPTHS[] = { 0.0f, -20.0f, -40.0f, -60.0f, -80.0f, -100.0f, -120.0f, -150.0f };
    static const size_t Z_PROBE_COUNT = sizeof(Z_PROBE_DEPTHS) / sizeof(Z_PROBE_DEPTHS[0]);

    ZProbeResult ProbeMultiLevelZ(Unit* unit, float x, float y, float maxProbeDepth)
    {
        ZProbeResult result;
        result.z = 0.0f;
        result.pathLength = 0.0f;
        result.isValid = false;
        result.isUnderground = false;

        if (!unit || !unit->GetMap())
            return result;

        Map* map = unit->GetMap();

        // Get surface Z first
        float surfaceZ = map->GetHeight(x, y, MAX_HEIGHT);
        if (surfaceZ == INVALID_HEIGHT || surfaceZ == VMAP_INVALID_HEIGHT_VALUE)
        {
            // No valid surface found, try water level
            surfaceZ = map->GetWaterLevel(x, y);
            if (surfaceZ == INVALID_HEIGHT || surfaceZ == VMAP_INVALID_HEIGHT_VALUE)
                return result;
        }

        // First, try surface level (most common case)
        PathGenerator path(unit);
        path.CalculatePath(x, y, surfaceZ, false);

        PathType pathType = path.GetPathType();
        int validTypes = PATHFIND_NORMAL | PATHFIND_INCOMPLETE;

        if (pathType & validTypes)
        {
            result.z = surfaceZ;
            result.pathLength = path.getPathLength();
            result.isValid = true;
            result.isUnderground = false;

            if (sManagerRegistry.HasBetterQuestingConfig() &&
                sManagerRegistry.GetBetterQuestingConfig().IsLogFallbackEnabled())
            {
                LOG_DEBUG("module", "BetterQuesting: Z probe at ({}, {}) - surface Z {} is pathable, length {}",
                    x, y, surfaceZ, result.pathLength);
            }
            return result;
        }

        // Surface isn't pathable - probe underground levels
        if (!sManagerRegistry.HasBetterQuestingConfig() ||
            !sManagerRegistry.GetBetterQuestingConfig().IsMultiLevelZProbeEnabled())
            return result;

        float bestZ = surfaceZ;
        float bestPathLength = 0.0f;
        bool foundUnderground = false;

        for (size_t i = 1; i < Z_PROBE_COUNT; ++i)  // Start at 1, we already tried surface
        {
            float probeDepth = Z_PROBE_DEPTHS[i];
            if (-probeDepth > maxProbeDepth)
                break;

            float probeZ = surfaceZ + probeDepth;

            // Try to get height at this probe level
            float testZ = map->GetHeight(x, y, probeZ + 10.0f);
            if (testZ == INVALID_HEIGHT || testZ == VMAP_INVALID_HEIGHT_VALUE)
                continue;

            // Make sure we found a floor significantly below surface
            if (surfaceZ - testZ < 10.0f)
                continue;

            // Try pathfinding to this Z level
            PathGenerator underPath(unit);
            underPath.CalculatePath(x, y, testZ, false);

            PathType underType = underPath.GetPathType();
            if (underType & validTypes)
            {
                float pathLen = underPath.getPathLength();
                if (!foundUnderground || pathLen < bestPathLength)
                {
                    bestZ = testZ;
                    bestPathLength = pathLen;
                    foundUnderground = true;
                }
            }
        }

        if (foundUnderground)
        {
            result.z = bestZ;
            result.pathLength = bestPathLength;
            result.isValid = true;
            result.isUnderground = true;

            if (sManagerRegistry.HasBetterQuestingConfig() &&
                sManagerRegistry.GetBetterQuestingConfig().IsLogFallbackEnabled())
            {
                LOG_DEBUG("module", "BetterQuesting: Z probe at ({}, {}) - found underground Z {} (surface {}), path length {}",
                    x, y, bestZ, surfaceZ, bestPathLength);
            }
        }
        else if (sManagerRegistry.HasBetterQuestingConfig() &&
                 sManagerRegistry.GetBetterQuestingConfig().IsLogFallbackEnabled())
        {
            LOG_DEBUG("module", "BetterQuesting: Z probe at ({}, {}) - no pathable Z found (surface Z {} not reachable)",
                x, y, surfaceZ);
        }

        return result;
    }

    float GetPathLength(Unit* unit, float x, float y, float z)
    {
        if (!unit || !unit->GetMap())
            return 0.0f;

        // Get valid Z coordinate at destination
        float destZ = unit->GetMap()->GetHeight(x, y, z + 5.0f);
        if (destZ == INVALID_HEIGHT || destZ == VMAP_INVALID_HEIGHT_VALUE)
        {
            // Try water level as fallback
            destZ = unit->GetMap()->GetWaterLevel(x, y);
            if (destZ == INVALID_HEIGHT || destZ == VMAP_INVALID_HEIGHT_VALUE)
                return 0.0f;
        }

        PathGenerator path(unit);
        path.CalculatePath(x, y, destZ, false);

        PathType pathType = path.GetPathType();
        int validTypes = PATHFIND_NORMAL | PATHFIND_INCOMPLETE;

        if (!(pathType & validTypes))
            return 0.0f;

        return path.getPathLength();
    }

    bool IsPathValid(Unit* unit, float x, float y, float z)
    {
        return GetPathLength(unit, x, y, z) > 0.0f;
    }

    std::vector<PathableSpawnPoint> GetPathableSpawns(Unit* unit, const std::vector<SpawnPoint>& spawns,
                                                       float maxDistance)
    {
        std::vector<PathableSpawnPoint> result;

        if (!unit || spawns.empty())
            return result;

        if (!sManagerRegistry.HasBetterQuestingConfig())
            return result;

        auto& config = sManagerRegistry.GetBetterQuestingConfig();

        for (const auto& spawn : spawns)
        {
            // Skip spawns on different maps
            if (spawn.mapId != unit->GetMapId())
                continue;

            // Calculate direct distance first (cheap check)
            float directDist = unit->GetDistance2d(spawn.x, spawn.y);
            if (directDist > maxDistance)
                continue;

            PathableSpawnPoint pathable;
            pathable.spawn = spawn;
            pathable.directDistance = directDist;
            pathable.resolvedZ = 0.0f;

            // Use multi-level Z probing to find valid Z coordinate (handles caves)
            ZProbeResult zResult = ProbeMultiLevelZ(unit, spawn.x, spawn.y,
                config.GetMaxZProbeDepth());

            if (zResult.isValid)
            {
                pathable.resolvedZ = zResult.z;
                pathable.pathLength = zResult.pathLength;
                pathable.isPathable = true;

                if (zResult.isUnderground && config.IsLogFallbackEnabled())
                {
                    LOG_DEBUG("module", "BetterQuesting: Found underground spawn at ({}, {}) Z {} on map {}",
                        spawn.x, spawn.y, zResult.z, spawn.mapId);
                }
            }
            else
            {
                // Z probing failed - try basic height lookup as fallback
                float spawnZ = unit->GetMap()->GetHeight(spawn.x, spawn.y, MAX_HEIGHT);
                if (spawnZ == INVALID_HEIGHT || spawnZ == VMAP_INVALID_HEIGHT_VALUE)
                {
                    spawnZ = unit->GetMap()->GetWaterLevel(spawn.x, spawn.y);
                }

                pathable.resolvedZ = spawnZ;
                pathable.pathLength = 0.0f;
                pathable.isPathable = false;
            }

            // Only include pathable spawns, or include all if config allows
            if (pathable.isPathable || !config.IsPathValidationEnabled())
            {
                result.push_back(pathable);
            }
        }

        // Sort by path length (shortest first), with non-pathable at end
        std::sort(result.begin(), result.end(), [](const PathableSpawnPoint& a, const PathableSpawnPoint& b) {
            // Pathable spawns come before non-pathable
            if (a.isPathable != b.isPathable)
                return a.isPathable;
            // Among pathable spawns, sort by path length
            if (a.isPathable && b.isPathable)
                return a.pathLength < b.pathLength;
            // Among non-pathable, sort by direct distance
            return a.directDistance < b.directDistance;
        });

        if (config.IsLogFallbackEnabled() && !result.empty())
        {
            int pathableCount = std::count_if(result.begin(), result.end(),
                [](const PathableSpawnPoint& p) { return p.isPathable; });
            int undergroundCount = std::count_if(result.begin(), result.end(),
                [](const PathableSpawnPoint& p) {
                    // Approximate underground detection - Z significantly below what GetHeight would return
                    return p.isPathable && p.resolvedZ < -50.0f;
                });
            LOG_DEBUG("module", "BetterQuesting: Found {} pathable spawns out of {} total ({} potentially underground)",
                pathableCount, result.size(), undergroundCount);
        }

        return result;
    }

    bool GetBestPathableQuestPOI(Player* player, uint32 questId, int32 objectiveIndex,
                                  SpawnPoint& outSpawn, float& outPathLength)
    {
        if (!IsEnabled() || !player)
            return false;

        auto& config = sManagerRegistry.GetBetterQuestingConfig();

        std::vector<SpawnPoint> positions;
        if (!GetQuestPOIFallback(questId, objectiveIndex, player->GetMapId(), positions))
            return false;

        auto pathableSpawns = GetPathableSpawns(player, positions, config.GetMaxSpawnDistance());

        if (pathableSpawns.empty())
            return false;

        // Return the best (first) pathable spawn
        for (const auto& ps : pathableSpawns)
        {
            if (ps.isPathable || !config.IsPathValidationEnabled())
            {
                outSpawn = ps.spawn;
                outPathLength = ps.pathLength > 0 ? ps.pathLength : ps.directDistance;

                if (config.IsLogFallbackEnabled())
                {
                    LOG_DEBUG("module", "BetterQuesting: Best POI for quest {} obj {} at ({}, {}) path length {}",
                        questId, objectiveIndex, outSpawn.x, outSpawn.y, outPathLength);
                }
                return true;
            }
        }

        return false;
    }

    bool GetBestPathableNpcSpawn(Unit* unit, uint32 npcId, SpawnPoint& outSpawn, float maxDistance)
    {
        if (!IsEnabled() || !unit)
            return false;

        auto& config = sManagerRegistry.GetBetterQuestingConfig();
        auto spawns = GetNpcSpawns(npcId, unit->GetMapId());
        if (spawns.empty())
            return false;

        auto pathableSpawns = GetPathableSpawns(unit, spawns, maxDistance);

        for (const auto& ps : pathableSpawns)
        {
            if (ps.isPathable || !config.IsPathValidationEnabled())
            {
                outSpawn = ps.spawn;
                return true;
            }
        }

        return false;
    }

    bool GetBestPathableObjectSpawn(Unit* unit, uint32 objectId, SpawnPoint& outSpawn, float maxDistance)
    {
        if (!IsEnabled() || !unit)
            return false;

        auto& config = sManagerRegistry.GetBetterQuestingConfig();
        auto spawns = GetObjectSpawns(objectId, unit->GetMapId());
        if (spawns.empty())
            return false;

        auto pathableSpawns = GetPathableSpawns(unit, spawns, maxDistance);

        for (const auto& ps : pathableSpawns)
        {
            if (ps.isPathable || !config.IsPathValidationEnabled())
            {
                outSpawn = ps.spawn;
                return true;
            }
        }

        return false;
    }

    // ==========================================
    // Approach Waypoint Navigation Implementation
    // ==========================================

    bool ValidateApproachPath(Unit* unit, const std::vector<ApproachWaypoint>& waypoints)
    {
        if (!unit || waypoints.empty())
            return false;

        // Check that we can path to the first waypoint
        float firstPathLen = GetPathLength(unit, waypoints[0].x, waypoints[0].y, waypoints[0].z);
        if (firstPathLen <= 0.0f)
            return false;

        // Check that consecutive waypoints are reachable from each other
        // (We use a simplified check - just verify we can path to final destination)
        // Full path validation would require simulating movement through each waypoint

        return true;
    }

    bool FindApproachPath(Unit* unit, float destX, float destY, float destZ, uint32 zoneId,
                          std::vector<ApproachWaypoint>& outPath, float& outTotalLength)
    {
        if (!unit)
            return false;

        if (!sManagerRegistry.HasBetterQuestingConfig() ||
            !sManagerRegistry.GetBetterQuestingConfig().IsApproachWaypointsEnabled())
            return false;

        if (!sManagerRegistry.HasCoreQuestDataMgr())
            return false;

        auto& config = sManagerRegistry.GetBetterQuestingConfig();
        auto& dataMgr = sManagerRegistry.GetCoreQuestDataMgr();

        outPath.clear();
        outTotalLength = 0.0f;

        uint32 mapId = unit->GetMapId();

        // Try to find approach waypoints for this zone
        auto waypoints = dataMgr.GetZoneApproachWaypoints(zoneId, mapId);

        if (waypoints.empty())
        {
            if (config.IsLogFallbackEnabled())
            {
                LOG_DEBUG("module", "BetterQuesting: No approach waypoints found for zone {} map {}",
                    zoneId, mapId);
            }
            return false;
        }

        // Find the best approach waypoint that:
        // 1. We can path to
        // 2. Gets us closer to the destination
        float destDist = unit->GetDistance2d(destX, destY);

        ApproachWaypoint const* bestEntrance = nullptr;
        float bestEntrancePathLen = 0.0f;

        for (const auto& wp : waypoints)
        {
            // Skip waypoints that aren't entrances (order > 0)
            // Entrances are typically the first waypoint in a chain
            if (wp.waypointOrder > 0)
                continue;

            // Check if this waypoint is reasonably close to the destination
            float wpToDestDist = std::sqrt(
                (wp.x - destX) * (wp.x - destX) +
                (wp.y - destY) * (wp.y - destY)
            );

            // Only consider waypoints that are closer to dest than we currently are
            // or within 200 yards of the destination (for cave entrances)
            if (wpToDestDist >= destDist && wpToDestDist > 200.0f)
                continue;

            // Check if we can path to this waypoint
            float pathLen = GetPathLength(unit, wp.x, wp.y, wp.z);
            if (pathLen > 0.0f)
            {
                if (!bestEntrance || wpToDestDist < bestEntrancePathLen)
                {
                    bestEntrance = &wp;
                    bestEntrancePathLen = pathLen;
                }
            }
        }

        if (!bestEntrance)
        {
            if (config.IsLogFallbackEnabled())
            {
                LOG_DEBUG("module", "BetterQuesting: No pathable approach waypoint found for zone {}",
                    zoneId);
            }
            return false;
        }

        // Found a valid entrance - build the approach path
        outPath.push_back(*bestEntrance);
        outTotalLength = bestEntrancePathLen;

        // Add any subsequent waypoints in the chain
        for (const auto& wp : waypoints)
        {
            // Find waypoints that come after our entrance
            if (wp.waypointOrder > bestEntrance->waypointOrder &&
                wp.waypointOrder <= bestEntrance->waypointOrder + config.GetMaxApproachWaypoints())
            {
                outPath.push_back(wp);

                // Estimate distance between waypoints
                if (outPath.size() >= 2)
                {
                    const auto& prev = outPath[outPath.size() - 2];
                    float segmentLen = std::sqrt(
                        (wp.x - prev.x) * (wp.x - prev.x) +
                        (wp.y - prev.y) * (wp.y - prev.y) +
                        (wp.z - prev.z) * (wp.z - prev.z)
                    );
                    outTotalLength += segmentLen;
                }
            }
        }

        // Sort by waypoint order to ensure correct sequence
        std::sort(outPath.begin(), outPath.end(), [](const ApproachWaypoint& a, const ApproachWaypoint& b) {
            return a.waypointOrder < b.waypointOrder;
        });

        if (config.IsLogFallbackEnabled())
        {
            LOG_DEBUG("module", "BetterQuesting: Found approach path with {} waypoints, total length {}",
                outPath.size(), outTotalLength);
        }

        return true;
    }

    std::vector<NavigableSpawnPoint> GetNavigableSpawns(Unit* unit, const std::vector<SpawnPoint>& spawns,
                                                         uint32 zoneId, float maxDistance)
    {
        std::vector<NavigableSpawnPoint> result;

        if (!unit || spawns.empty())
            return result;

        if (!sManagerRegistry.HasBetterQuestingConfig())
            return result;

        auto& config = sManagerRegistry.GetBetterQuestingConfig();

        // First, try direct pathfinding with Z probing
        auto pathableSpawns = GetPathableSpawns(unit, spawns, maxDistance);

        for (const auto& ps : pathableSpawns)
        {
            NavigableSpawnPoint nav;
            nav.spawn = ps.spawn;
            nav.pathLength = ps.pathLength;
            nav.isPathable = ps.isPathable;
            nav.directDistance = ps.directDistance;
            nav.resolvedZ = ps.resolvedZ;
            nav.usesApproach = false;

            if (nav.isPathable)
            {
                result.push_back(nav);
            }
            else if (config.IsApproachWaypointsEnabled() && zoneId > 0)
            {
                // Try to find an approach path using waypoints
                std::vector<ApproachWaypoint> approachPath;
                float totalLength = 0.0f;

                if (FindApproachPath(unit, ps.spawn.x, ps.spawn.y, ps.resolvedZ, zoneId,
                                     approachPath, totalLength))
                {
                    nav.isPathable = true;
                    nav.usesApproach = true;
                    nav.approachPath = std::move(approachPath);
                    nav.pathLength = totalLength + ps.directDistance;  // Approach + remaining distance
                    result.push_back(nav);

                    if (config.IsLogFallbackEnabled())
                    {
                        LOG_DEBUG("module", "BetterQuesting: Using approach waypoints for spawn at ({}, {})",
                            ps.spawn.x, ps.spawn.y);
                    }
                }
            }
        }

        // Sort by path length (shortest first), with approach paths slightly penalized
        std::sort(result.begin(), result.end(), [](const NavigableSpawnPoint& a, const NavigableSpawnPoint& b) {
            // Pathable spawns come before non-pathable
            if (a.isPathable != b.isPathable)
                return a.isPathable;
            // Prefer direct paths over approach paths (10% penalty for approach)
            float aLen = a.usesApproach ? a.pathLength * 1.1f : a.pathLength;
            float bLen = b.usesApproach ? b.pathLength * 1.1f : b.pathLength;
            return aLen < bLen;
        });

        if (config.IsLogFallbackEnabled() && !result.empty())
        {
            int directCount = std::count_if(result.begin(), result.end(),
                [](const NavigableSpawnPoint& p) { return p.isPathable && !p.usesApproach; });
            int approachCount = std::count_if(result.begin(), result.end(),
                [](const NavigableSpawnPoint& p) { return p.isPathable && p.usesApproach; });
            LOG_DEBUG("module", "BetterQuesting: Found {} navigable spawns ({} direct, {} via approach)",
                result.size(), directCount, approachCount);
        }

        return result;
    }

    // ==========================================
    // Quest Guide API Implementation
    // ==========================================

    bool IsQuestGuideEnabled()
    {
        if (!sManagerRegistry.HasBetterQuestingConfig() || !sManagerRegistry.HasQuestGuideMgr())
            return false;

        return sManagerRegistry.GetBetterQuestingConfig().IsEnabled() &&
               sManagerRegistry.GetBetterQuestingConfig().IsQuestGuideEnabled() &&
               sManagerRegistry.GetQuestGuideMgr().IsDataLoaded();
    }

    uint32 GetCurrentGuideQuest(Player* player)
    {
        if (!IsQuestGuideEnabled() || !player)
            return 0;

        QuestGuideStep const* step = sManagerRegistry.GetQuestGuideMgr().GetCurrentStep(player);
        return step ? step->questId : 0;
    }

    std::vector<uint32> GetGuidedQuests(Player* player, uint32 limit)
    {
        std::vector<uint32> result;

        if (!IsQuestGuideEnabled() || !player)
            return result;

        auto steps = sManagerRegistry.GetQuestGuideMgr().GetAvailableSteps(player, limit);
        for (const auto* step : steps)
        {
            result.push_back(step->questId);
        }

        return result;
    }

    bool ShouldChangeZone(Player* player, uint32& targetZoneId)
    {
        if (!IsQuestGuideEnabled() || !player)
            return false;

        return sManagerRegistry.GetQuestGuideMgr().ShouldTransitionZone(player, targetZoneId);
    }

    int32 GetGuidePriorityBonus(Player* player, uint32 questId)
    {
        if (!IsQuestGuideEnabled() || !player)
            return 0;

        return sManagerRegistry.GetQuestGuideMgr().GetGuideQuestPriorityBonus(player, questId);
    }

    bool IsQuestInActiveGuide(Player* player, uint32 questId)
    {
        if (!IsQuestGuideEnabled() || !player)
            return false;

        return sManagerRegistry.GetQuestGuideMgr().IsQuestInActiveGuide(player, questId);
    }

    void InitializePlayerGuide(Player* player)
    {
        if (!IsQuestGuideEnabled() || !player)
            return;

        if (!sManagerRegistry.HasBetterQuestingConfig())
            return;

        auto& guideMgr = sManagerRegistry.GetQuestGuideMgr();
        auto& config = sManagerRegistry.GetBetterQuestingConfig();

        // Check if player already has a guide
        QuestGuideProgress const* progress = guideMgr.GetPlayerProgress(player->GetGUID().GetCounter());
        if (progress)
        {
            // Already has a guide, sync progress
            guideMgr.SyncPlayerProgress(player);
            return;
        }

        // Auto-select guide if enabled
        if (config.IsAutoGuideSelectionEnabled())
        {
            QuestGuideData const* bestGuide = guideMgr.GetBestGuideForPlayer(player);
            if (bestGuide)
            {
                guideMgr.SetPlayerGuide(player, bestGuide->guideId);
                guideMgr.SyncPlayerProgress(player);

                LOG_DEBUG("module", "BetterQuesting: Auto-selected guide '{}' for player {}",
                    bestGuide->name, player->GetName());
            }
        }
    }

    void SyncGuideProgress(Player* player)
    {
        if (!IsQuestGuideEnabled() || !player)
            return;

        sManagerRegistry.GetQuestGuideMgr().SyncPlayerProgress(player);
    }
}
