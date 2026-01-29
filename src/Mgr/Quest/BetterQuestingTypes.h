/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#ifndef MOD_BETTERQUESTING_TYPES_H
#define MOD_BETTERQUESTING_TYPES_H

#include "Common.h"
#include <string>
#include <vector>

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

// Result of path validation for a spawn point
struct PathableSpawnPoint
{
    SpawnPoint spawn;
    float pathLength;      // Length of path (0 if invalid)
    bool isPathable;       // True if valid path exists
    float directDistance;  // Straight-line distance
    float resolvedZ;       // Z coordinate found via probing (may differ from surface)
};

// Result of multi-level Z probing
struct ZProbeResult
{
    float z;               // Valid Z coordinate found
    float pathLength;      // Path length to this Z level (0 if not validated)
    bool isValid;          // True if a valid Z was found
    bool isUnderground;    // True if this Z is below surface level
};

// Navigable spawn point with optional approach waypoints
struct NavigableSpawnPoint
{
    SpawnPoint spawn;
    float pathLength;       // Total path length (including approach waypoints)
    bool isPathable;        // True if valid path exists (direct or via waypoints)
    float directDistance;   // Straight-line distance
    float resolvedZ;        // Z coordinate found via probing
    bool usesApproach;      // True if approach waypoints are needed
    std::vector<ApproachWaypoint> approachPath;  // Waypoints to reach this spawn
};

// Quest guide definition
struct QuestGuideData
{
    uint32 guideId;
    std::string name;
    std::string description;
    uint8 faction;          // 0=Alliance, 1=Horde, 2=Both
    uint8 startingRace;     // 0=any, 1=Human, 2=Orc, etc.
    uint8 minLevel;
    uint8 maxLevel;
    uint8 priority;
    bool enabled;
};

// Quest step within a guide
struct QuestGuideStep
{
    uint32 guideId;
    uint32 stepOrder;
    uint32 questId;
    uint8 minLevel;
    uint8 maxLevel;
    uint32 zoneId;
    bool isOptional;
    bool skipIfCompleted;
};

// Zone transition definition
struct QuestGuideZoneTransition
{
    uint32 guideId;
    uint32 fromZoneId;
    uint32 toZoneId;
    uint8 triggerLevel;
    uint32 triggerQuestComplete;
    uint8 priority;
};

// Player's progress through a guide
struct QuestGuideProgress
{
    uint32 guid;
    uint32 guideId;
    uint32 currentStep;
    uint32 activeQuestId{0};    // Quest being worked on (persisted across logout)
    uint8 activeSubStatus{0};   // QuestingSubStatus enum value (persisted across logout)
};

#endif // MOD_BETTERQUESTING_TYPES_H
