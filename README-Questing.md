# Better Questing System

An enhancement to mod-playerbots that builds quest and spawn indexes from AzerothCore's ObjectMgr to improve bot quest accuracy, navigation, and intelligence.

## Features

- **POI Fallback**: Uses NPC/object spawn coordinates from core data when `quest_poi` data is missing
- **Quest Chain Awareness**: Tracks quest prerequisites and chain relationships using core Quest data
- **Intelligent Quest Selection**: Recommends appropriate quests based on player level, race, class, and zone
- **Path Validation**: Validates paths to objectives before pursuing them
- **Cave Navigation**: Multi-level Z probing and approach waypoints for underground areas
- **Quest Guide System**: Database-defined leveling paths similar to addon-based quest guides

## Requirements

- AzerothCore with WotLK (3.3.5a)
- mod-playerbots module

## Installation

### 1. Import Database Schema

```bash
# Import the approach waypoints table (optional, for cave navigation)
mysql -u root -p acore_world < data/sql/db_world/approach_waypoints.sql

# Import the quest guide tables (optional, for guided leveling)
mysql -u root -p acore_world < data/sql/db_world/quest_guide_tables.sql

# Import example guide data (Human 1-10)
mysql -u root -p acore_world < data/sql/db_world/guide_human_1_10.sql

# Import character progress tracking table (required for quest guides)
mysql -u root -p acore_characters < data/sql/db_characters/guide_progress.sql
```

### 2. Rebuild and Restart the Server

## Configuration

BetterQuesting settings are in the main `playerbots.conf` file under the `BETTER QUESTING` section:

```ini
# Enable/disable the module
BetterQuesting.Enabled = 1

# Boost quest priority (makes bots quest ~92% of the time)
BetterQuesting.QuestPriorityBoost = 1

# Use spawn data as POI fallback
BetterQuesting.POIFallback = 1

# Maximum distance for spawn points (yards)
BetterQuesting.MaxSpawnDistance = 2500.0

# Check quest prerequisites before accepting
BetterQuesting.PrerequisiteCheck = 1

# Enable quest chain awareness
BetterQuesting.QuestChainAwareness = 1

# Validate paths to objectives
BetterQuesting.PathValidation = 1

# Quest Guide System
BetterQuesting.QuestGuide.Enabled = 1
BetterQuesting.QuestGuide.StrictMode = 1
BetterQuesting.QuestGuide.PriorityBonus = 50
BetterQuesting.QuestGuide.AutoSelect = 1
BetterQuesting.QuestGuide.ZoneTransition = 1

# Cave Navigation
BetterQuesting.MultiLevelZProbe.Enabled = 1
BetterQuesting.MultiLevelZProbe.MaxDepth = 150.0
BetterQuesting.ApproachWaypoints.Enabled = 1
BetterQuesting.ApproachWaypoints.MaxWaypoints = 10

# Debug logging
BetterQuesting.LogFallback = 0
```

## Public API

The `BetterQuesting` namespace provides a clean API for integration:

```cpp
#include "Manager/Quest/BetterQuesting.h"

// Check if module is available
if (BetterQuesting::IsEnabled())
{
    // Get POI fallback data
    std::vector<SpawnPoint> positions;
    if (BetterQuesting::GetQuestPOIFallback(questId, objectiveIdx, mapId, positions))
    {
        // Use the positions...
    }

    // Get pathable spawns (validates navigation)
    auto spawns = BetterQuesting::GetPathableSpawns(bot, npcId, maxDistance);

    // Check prerequisites
    if (BetterQuesting::ArePrerequisitesMet(player, questId))
    {
        // Quest can be accepted...
    }

    // Get recommended quests
    auto recommended = BetterQuesting::GetRecommendedQuests(player, zoneId, 10);

    // Quest Guide integration
    if (BetterQuesting::IsQuestGuideEnabled())
    {
        uint32 currentQuest = BetterQuesting::GetCurrentGuideQuest(player);
        bool shouldMove = BetterQuesting::ShouldChangeZone(player, newZoneId);
    }
}
```

## How It Works

### Data Source

BetterQuesting builds indexes at server startup from AzerothCore's existing ObjectMgr data:

- **NPC Spawns**: `sObjectMgr->GetAllCreatureData()` - All creature spawn locations
- **Object Spawns**: `sObjectMgr->GetAllGOData()` - All gameobject spawn locations
- **Quest Starters**: `sObjectMgr->GetCreatureQuestRelationMap()` and `GetGOQuestRelationMap()`
- **Quest Enders**: `sObjectMgr->GetCreatureQuestInvolvedRelationMap()` and `GetGOQuestInvolvedRelationMap()`
- **Quest Prerequisites**: `Quest::GetPrevQuestId()`, `Quest::GetNextQuestInChain()`, `Quest::GetExclusiveGroup()`

This means:
- No external data extraction required
- Data is always in sync with your database
- No additional SQL tables needed for spawn/quest data
- Faster deployment and simpler maintenance

## Database Tables

### World Database (acore_world)

| Table | Description |
|-------|-------------|
| `playerbots_questing_approach_waypoints` | Cave entrances and difficult navigation paths |
| `playerbots_questing_quest_guides` | Guide definitions (faction, level range) |
| `playerbots_questing_guide_steps` | Individual steps within guides |
| `playerbots_questing_zone_transitions` | Zone transition triggers |

### Character Database (acore_characters)

| Table | Description |
|-------|-------------|
| `playerbots_questing_player_guide_progress` | Player guide progress tracking |

## Module Structure

```
src/
├── Config/
│   ├── BetterQuestingConfig.h          # Configuration interface
│   └── BetterQuestingConfig.cpp        # Configuration loading
├── Manager/Quest/
│   ├── BetterQuesting.h                # Public API namespace
│   ├── BetterQuesting.cpp              # Path validation, approach waypoints
│   ├── CoreQuestDataMgr.h              # Core data manager interface
│   ├── CoreQuestDataMgr.cpp            # Index building and queries
│   ├── QuestGuideMgr.h                 # Quest guide manager interface
│   └── QuestGuideMgr.cpp               # Guide progression logic
├── Bot/BaseAi/
│   ├── Actions/                        # Quest interaction actions
│   │   ├── QuestAction.h/cpp           # Base quest action
│   │   ├── AcceptQuestAction.h/cpp     # Accept quests
│   │   ├── DropQuestAction.h/cpp       # Drop quests
│   │   └── TalkToQuestGiverAction.h/cpp
│   └── Value/
│       └── QuestValues.h/cpp           # Quest state calculations
├── Scenario/ClassAi/Generic/Strategy/
│   └── QuestStrategies.h/cpp           # Quest execution strategies
└── ScriptHook/
    └── BetterQuestingScripts.cpp       # WorldScript/PlayerScript hooks

data/sql/
├── db_world/
│   ├── approach_waypoints.sql          # Cave navigation waypoints
│   ├── quest_guide_tables.sql          # Guide system schema
│   └── guide_human_1_10.sql            # Example guide data
└── db_characters/
    └── guide_progress.sql              # Player progress table

tools/
├── cave_detector.py                    # Detect caves from spawn patterns
└── cave_entrance_finder.py             # Find cave entrance waypoints
```

## Key Systems

### Quest Priority Boosting

When enabled, sets the RPG quest weight to 100 while other activities (wander, grind, camp, travel, rest) remain at 1-2. This results in bots spending approximately 92% of their time questing.

### Multi-Level Z Probing

For cave navigation, the system probes multiple depth levels:
- Surface (0y)
- Underground: -20, -40, -60, -80, -100, -120, -150 yards

At each level, it validates whether a walkable path exists to the spawn point.

### Approach Waypoints

Pre-defined navigation paths for difficult-to-reach locations like caves. Target types:
- 0 = Zone
- 1 = NPC
- 2 = Object
- 3 = Quest
- 4 = Area

Common problem areas (Frostmane Hold, Fargodeep Mine, etc.) come pre-populated.

### Quest Guide System

Database-defined leveling paths similar to RestedXP guides:
- Faction and race-specific guides
- Auto-selection for new players
- Zone transition support
- Priority bonus: +50 base, +25 for current step
- Strict mode (guide quests only) vs bonus mode (guide quests preferred)

## Troubleshooting

### Module not loading

1. Check that the module compiled successfully
2. Check server logs for initialization errors
3. Verify `BetterQuesting.Enabled = 1` in playerbots.conf

### Bots not finding quest objectives

1. Enable logging: `BetterQuesting.LogFallback = 1`
2. Check server logs for spawn index counts on startup
3. Verify the creature/gameobject exists in your database

### Bots not reaching cave objectives

1. Enable logging: `BetterQuesting.LogFallback = 1`
2. Check if approach waypoints exist for the location
3. Verify `MultiLevelZProbe.Enabled = 1`
4. Add custom approach waypoints to `playerbots_questing_approach_waypoints`

### Quest guide not working

1. Verify guide data exists in `playerbots_questing_quest_guides`
2. Check player's faction matches a guide
3. Enable `BetterQuesting.QuestGuide.AutoSelect = 1`

## Benefits Over External Data

1. **No external data dependency** - Uses data already loaded by AC core
2. **Always up-to-date** - Automatically reflects any database changes
3. **Faster startup** - No additional SQL queries needed for spawn data
4. **Simpler deployment** - No need to run extraction tools
5. **Less maintenance** - No need to update external data for new content

## Credits

- [AzerothCore](https://www.azerothcore.org/) - Server framework
- [mod-playerbots](https://github.com/mod-playerbots/mod-playerbots) - Playerbot module

## License

This module is released under the GNU AGPL v3 license, same as AzerothCore.
