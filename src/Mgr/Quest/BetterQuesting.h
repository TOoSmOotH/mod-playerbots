/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#ifndef MOD_BETTERQUESTING_HOOKS_H
#define MOD_BETTERQUESTING_HOOKS_H

#include "BetterQuestingTypes.h"
#include <vector>

class Player;
class Unit;

// Public API for mod-playerbots integration
// These functions can be called from mod-playerbots to get quest and spawn data

namespace BetterQuesting
{
    // Check if BetterQuesting module is enabled and data is loaded
    bool IsEnabled();

    // Get spawn points for quest POI fallback
    // Returns true if positions were found
    bool GetQuestPOIFallback(uint32 questId, int32 objectiveIndex, uint32 mapId,
                             std::vector<SpawnPoint>& positions);

    // Get NPC spawn locations
    std::vector<SpawnPoint> GetNpcSpawns(uint32 npcId, uint32 mapId = 0);

    // Get object spawn locations
    std::vector<SpawnPoint> GetObjectSpawns(uint32 objectId, uint32 mapId = 0);

    // Get quest starter locations
    std::vector<SpawnPoint> GetQuestStarterLocations(uint32 questId, uint32 mapId = 0);

    // Get quest ender locations
    std::vector<SpawnPoint> GetQuestEnderLocations(uint32 questId, uint32 mapId = 0);

    // Check if player meets quest prerequisites
    bool ArePrerequisitesMet(Player* player, uint32 questId);

    // Get next quest in chain (0 if none)
    uint32 GetNextQuestInChain(uint32 questId);

    // Get recommended quests for player in zone
    std::vector<uint32> GetRecommendedQuests(Player* player, uint32 zoneId, uint32 limit = 10);

    // ==========================================
    // Navigation / Path Validation API
    // ==========================================

    // Multi-level Z probing to find valid heights (especially for caves)
    // Probes multiple Z levels below surface to find walkable positions
    // Returns the best Z coordinate found, with path validation if unit is provided
    ZProbeResult ProbeMultiLevelZ(Unit* unit, float x, float y, float maxProbeDepth = 150.0f);

    // Check if a valid path exists from unit to destination
    // Returns path length if valid, 0 if no path
    float GetPathLength(Unit* unit, float x, float y, float z);

    // Check if path is valid (uses PathGenerator)
    bool IsPathValid(Unit* unit, float x, float y, float z);

    // Get spawn points sorted by pathability and distance
    // Returns only reachable spawn points, sorted by path length (shortest first)
    std::vector<PathableSpawnPoint> GetPathableSpawns(Unit* unit, const std::vector<SpawnPoint>& spawns,
                                                       float maxDistance = 1500.0f);

    // Get the best (closest pathable) spawn point for a quest objective
    // Returns true if a pathable spawn was found, fills outSpawn
    bool GetBestPathableQuestPOI(Player* player, uint32 questId, int32 objectiveIndex,
                                  SpawnPoint& outSpawn, float& outPathLength);

    // Get the best pathable NPC spawn location
    bool GetBestPathableNpcSpawn(Unit* unit, uint32 npcId, SpawnPoint& outSpawn,
                                  float maxDistance = 1500.0f);

    // Get the best pathable object spawn location
    bool GetBestPathableObjectSpawn(Unit* unit, uint32 objectId, SpawnPoint& outSpawn,
                                     float maxDistance = 1500.0f);

    // ==========================================
    // Approach Waypoint Navigation API
    // ==========================================

    // Get navigable spawn points with approach waypoint support
    // Falls back to approach waypoints when direct pathfinding fails
    std::vector<NavigableSpawnPoint> GetNavigableSpawns(Unit* unit, const std::vector<SpawnPoint>& spawns,
                                                         uint32 zoneId = 0, float maxDistance = 1500.0f);

    // Try to find an approach path to the given location using waypoints
    // Returns true if a valid waypoint path was found
    bool FindApproachPath(Unit* unit, float destX, float destY, float destZ, uint32 zoneId,
                          std::vector<ApproachWaypoint>& outPath, float& outTotalLength);

    // Validate that a waypoint chain is pathable
    bool ValidateApproachPath(Unit* unit, const std::vector<ApproachWaypoint>& waypoints);

    // ==========================================
    // Quest Guide API
    // ==========================================

    // Check if quest guide system is enabled
    bool IsQuestGuideEnabled();

    // Get the current guide quest for a player
    uint32 GetCurrentGuideQuest(Player* player);

    // Get list of upcoming guide quests
    std::vector<uint32> GetGuidedQuests(Player* player, uint32 limit = 5);

    // Check if player should transition to a new zone
    // Returns true if transition should happen, targetZoneId is filled with destination
    bool ShouldChangeZone(Player* player, uint32& targetZoneId);

    // Get priority bonus for a quest in the player's active guide
    int32 GetGuidePriorityBonus(Player* player, uint32 questId);

    // Check if a quest is in the player's active guide
    bool IsQuestInActiveGuide(Player* player, uint32 questId);

    // Initialize guide for player (auto-selects if not set)
    void InitializePlayerGuide(Player* player);

    // Sync player's guide progress based on completed quests
    void SyncGuideProgress(Player* player);
}

#endif // MOD_BETTERQUESTING_HOOKS_H
