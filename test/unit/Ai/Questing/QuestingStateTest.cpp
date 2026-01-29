/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>
#include <map>
#include <cmath>

/**
 * @brief Tests for questing state machine transitions
 *
 * These tests validate the questing substatus transitions and quest targeting logic
 * without requiring actual game objects.
 */

/**
 * @brief Questing substatus enum (mirrors NewRpgInfo.h)
 */
enum class QuestingSubStatus : uint8_t
{
    Idle = 0,
    TravelingToAccept = 1,
    TravelingToObjective = 2,
    TravelingToTurnIn = 3,
    Searching = 4
};

/**
 * @brief Quest status enum (mirrors game)
 */
enum class QuestStatus
{
    None,
    Incomplete,
    Complete,
    Rewarded
};

/**
 * @brief Tests for questing state transitions
 */
class QuestingStateTransitionTest : public ::testing::Test
{
protected:
    struct QuestingContext
    {
        uint32_t questId;
        QuestStatus questStatus;
        QuestingSubStatus subStatus;
        bool hasQuestGiver;
        bool hasObjective;
        bool hasApproachWaypoint;
        float distanceToTarget;
    };

    QuestingContext context_;
    std::map<QuestingSubStatus, std::set<QuestingSubStatus>> validTransitions_;

    void SetUp() override
    {
        context_ = {0, QuestStatus::None, QuestingSubStatus::Idle, false, false, false, 100.0f};

        // Define valid questing state transitions
        validTransitions_[QuestingSubStatus::Idle] = {
            QuestingSubStatus::TravelingToAccept,
            QuestingSubStatus::TravelingToObjective,
            QuestingSubStatus::TravelingToTurnIn,
            QuestingSubStatus::Searching
        };
        validTransitions_[QuestingSubStatus::TravelingToAccept] = {
            QuestingSubStatus::Idle,
            QuestingSubStatus::TravelingToObjective,
            QuestingSubStatus::TravelingToTurnIn,
            QuestingSubStatus::Searching
        };
        validTransitions_[QuestingSubStatus::TravelingToObjective] = {
            QuestingSubStatus::Idle,
            QuestingSubStatus::TravelingToTurnIn,
            QuestingSubStatus::Searching
        };
        validTransitions_[QuestingSubStatus::TravelingToTurnIn] = {
            QuestingSubStatus::Idle,
            QuestingSubStatus::TravelingToAccept,
            QuestingSubStatus::TravelingToObjective,
            QuestingSubStatus::Searching
        };
        validTransitions_[QuestingSubStatus::Searching] = {
            QuestingSubStatus::Idle,
            QuestingSubStatus::TravelingToAccept,
            QuestingSubStatus::TravelingToObjective,
            QuestingSubStatus::TravelingToTurnIn
        };
    }

    bool CanTransition(QuestingSubStatus from, QuestingSubStatus to)
    {
        if (from == to)
            return true;  // Same state is always valid
        auto it = validTransitions_.find(from);
        if (it == validTransitions_.end())
            return false;
        return it->second.find(to) != it->second.end();
    }

    QuestingSubStatus DetermineDesiredSubStatus()
    {
        // No quest - search for one
        if (context_.questId == 0)
            return QuestingSubStatus::Searching;

        // Quest complete - go turn in
        if (context_.questStatus == QuestStatus::Complete)
            return QuestingSubStatus::TravelingToTurnIn;

        // Quest incomplete - go to objective
        if (context_.questStatus == QuestStatus::Incomplete)
            return QuestingSubStatus::TravelingToObjective;

        // Quest not accepted yet - go accept
        if (context_.questStatus == QuestStatus::None && context_.hasQuestGiver)
            return QuestingSubStatus::TravelingToAccept;

        // No quest available nearby - search
        return QuestingSubStatus::Searching;
    }
};

TEST_F(QuestingStateTransitionTest, InitialStateIsIdle)
{
    EXPECT_EQ(QuestingSubStatus::Idle, context_.subStatus);
}

TEST_F(QuestingStateTransitionTest, NoQuestTriggersSearching)
{
    context_.questId = 0;
    EXPECT_EQ(QuestingSubStatus::Searching, DetermineDesiredSubStatus());
}

TEST_F(QuestingStateTransitionTest, CompleteQuestTriggersToTurnIn)
{
    context_.questId = 123;
    context_.questStatus = QuestStatus::Complete;
    EXPECT_EQ(QuestingSubStatus::TravelingToTurnIn, DetermineDesiredSubStatus());
}

TEST_F(QuestingStateTransitionTest, IncompleteQuestTriggersToObjective)
{
    context_.questId = 123;
    context_.questStatus = QuestStatus::Incomplete;
    EXPECT_EQ(QuestingSubStatus::TravelingToObjective, DetermineDesiredSubStatus());
}

TEST_F(QuestingStateTransitionTest, AvailableQuestTriggersToAccept)
{
    context_.questId = 123;
    context_.questStatus = QuestStatus::None;
    context_.hasQuestGiver = true;
    EXPECT_EQ(QuestingSubStatus::TravelingToAccept, DetermineDesiredSubStatus());
}

TEST_F(QuestingStateTransitionTest, ValidTransitionsFromIdle)
{
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Idle, QuestingSubStatus::TravelingToAccept));
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Idle, QuestingSubStatus::TravelingToObjective));
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Idle, QuestingSubStatus::TravelingToTurnIn));
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Idle, QuestingSubStatus::Searching));
}

TEST_F(QuestingStateTransitionTest, ValidTransitionsFromSearching)
{
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Searching, QuestingSubStatus::TravelingToAccept));
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Searching, QuestingSubStatus::TravelingToObjective));
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Searching, QuestingSubStatus::TravelingToTurnIn));
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Searching, QuestingSubStatus::Idle));
}

TEST_F(QuestingStateTransitionTest, SameStateAlwaysValid)
{
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Idle, QuestingSubStatus::Idle));
    EXPECT_TRUE(CanTransition(QuestingSubStatus::Searching, QuestingSubStatus::Searching));
    EXPECT_TRUE(CanTransition(QuestingSubStatus::TravelingToObjective, QuestingSubStatus::TravelingToObjective));
}

/**
 * @brief Tests for quest objective detection
 */
class QuestObjectiveTest : public ::testing::Test
{
protected:
    struct QuestObjective
    {
        uint32_t requiredId;      // NPC or item ID
        uint32_t requiredCount;
        uint32_t currentCount;
        bool isKillObjective;     // true = kill, false = item
    };

    struct Quest
    {
        uint32_t questId;
        std::vector<QuestObjective> objectives;
    };

    bool IsObjectiveComplete(QuestObjective const& obj)
    {
        return obj.currentCount >= obj.requiredCount;
    }

    bool AreAllObjectivesComplete(Quest const& quest)
    {
        for (auto const& obj : quest.objectives)
        {
            if (!IsObjectiveComplete(obj))
                return false;
        }
        return true;
    }

    bool IsTargetNeededForQuest(uint32_t targetId, Quest const& quest)
    {
        for (auto const& obj : quest.objectives)
        {
            if (obj.isKillObjective && obj.requiredId == targetId && !IsObjectiveComplete(obj))
                return true;
        }
        return false;
    }

    uint32_t GetRemainingKills(uint32_t targetId, Quest const& quest)
    {
        for (auto const& obj : quest.objectives)
        {
            if (obj.isKillObjective && obj.requiredId == targetId)
                return obj.requiredCount > obj.currentCount ? obj.requiredCount - obj.currentCount : 0;
        }
        return 0;
    }
};

TEST_F(QuestObjectiveTest, SingleObjectiveIncomplete)
{
    QuestObjective obj{80, 8, 3, true};  // Kill 8 kobolds, 3 done
    EXPECT_FALSE(IsObjectiveComplete(obj));
}

TEST_F(QuestObjectiveTest, SingleObjectiveComplete)
{
    QuestObjective obj{80, 8, 8, true};  // Kill 8 kobolds, 8 done
    EXPECT_TRUE(IsObjectiveComplete(obj));
}

TEST_F(QuestObjectiveTest, SingleObjectiveOverComplete)
{
    QuestObjective obj{80, 8, 10, true};  // Kill 8 kobolds, 10 done
    EXPECT_TRUE(IsObjectiveComplete(obj));
}

TEST_F(QuestObjectiveTest, QuestWithMultipleObjectives)
{
    Quest quest{21, {
        {80, 8, 8, true},   // Kobold Laborers - complete
        {257, 4, 2, false}  // Items - incomplete
    }};
    EXPECT_FALSE(AreAllObjectivesComplete(quest));
}

TEST_F(QuestObjectiveTest, QuestAllObjectivesComplete)
{
    Quest quest{21, {
        {80, 8, 8, true},
        {257, 4, 4, false}
    }};
    EXPECT_TRUE(AreAllObjectivesComplete(quest));
}

TEST_F(QuestObjectiveTest, TargetNeededForIncompleteObjective)
{
    Quest quest{21, {{80, 8, 3, true}}};
    EXPECT_TRUE(IsTargetNeededForQuest(80, quest));
}

TEST_F(QuestObjectiveTest, TargetNotNeededForCompleteObjective)
{
    Quest quest{21, {{80, 8, 8, true}}};
    EXPECT_FALSE(IsTargetNeededForQuest(80, quest));
}

TEST_F(QuestObjectiveTest, TargetNotNeededForDifferentMob)
{
    Quest quest{21, {{80, 8, 3, true}}};
    EXPECT_FALSE(IsTargetNeededForQuest(99, quest));  // Different mob
}

TEST_F(QuestObjectiveTest, RemainingKillsCalculation)
{
    Quest quest{21, {{80, 8, 3, true}}};
    EXPECT_EQ(5u, GetRemainingKills(80, quest));
}

TEST_F(QuestObjectiveTest, RemainingKillsZeroWhenComplete)
{
    Quest quest{21, {{80, 8, 8, true}}};
    EXPECT_EQ(0u, GetRemainingKills(80, quest));
}

/**
 * @brief Tests for quest target selection
 */
class QuestTargetSelectionTest : public ::testing::Test
{
protected:
    struct Position
    {
        float x, y, z;
    };

    struct PotentialTarget
    {
        uint32_t guid;
        uint32_t entry;
        Position pos;
        bool isAlive;
        uint8_t level;
        bool isElite;
    };

    struct SelectionContext
    {
        Position botPos;
        uint8_t botLevel;
        bool canFightElite;
        uint32_t questTargetEntry;
        uint32_t killsNeeded;
    };

    float CalculateDistance(Position const& a, Position const& b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    bool IsValidQuestTarget(PotentialTarget const& target, SelectionContext const& ctx)
    {
        // Must be alive
        if (!target.isAlive)
            return false;

        // Must match quest target
        if (target.entry != ctx.questTargetEntry)
            return false;

        // Must need more kills
        if (ctx.killsNeeded == 0)
            return false;

        // Level check (max 4 levels higher)
        if (target.level > ctx.botLevel + 4)
            return false;

        // Elite check
        if (target.isElite && !ctx.canFightElite)
            return false;

        return true;
    }

    uint32_t SelectBestTarget(std::vector<PotentialTarget> const& targets, SelectionContext const& ctx)
    {
        uint32_t bestGuid = 0;
        float bestDistance = 999999.0f;

        for (auto const& target : targets)
        {
            if (!IsValidQuestTarget(target, ctx))
                continue;

            float dist = CalculateDistance(ctx.botPos, target.pos);
            if (dist < bestDistance)
            {
                bestDistance = dist;
                bestGuid = target.guid;
            }
        }

        return bestGuid;
    }
};

TEST_F(QuestTargetSelectionTest, SelectsClosestValidTarget)
{
    std::vector<PotentialTarget> targets = {
        {1, 80, {100, 100, 0}, true, 5, false},  // Far
        {2, 80, {10, 10, 0}, true, 5, false},    // Close
        {3, 80, {50, 50, 0}, true, 5, false}     // Medium
    };
    SelectionContext ctx{{0, 0, 0}, 10, false, 80, 5};

    EXPECT_EQ(2u, SelectBestTarget(targets, ctx));
}

TEST_F(QuestTargetSelectionTest, IgnoresDeadTargets)
{
    std::vector<PotentialTarget> targets = {
        {1, 80, {10, 10, 0}, false, 5, false},  // Dead, closest
        {2, 80, {50, 50, 0}, true, 5, false}    // Alive, farther
    };
    SelectionContext ctx{{0, 0, 0}, 10, false, 80, 5};

    EXPECT_EQ(2u, SelectBestTarget(targets, ctx));
}

TEST_F(QuestTargetSelectionTest, IgnoresWrongEntry)
{
    std::vector<PotentialTarget> targets = {
        {1, 99, {10, 10, 0}, true, 5, false},   // Wrong entry, closest
        {2, 80, {50, 50, 0}, true, 5, false}    // Correct entry
    };
    SelectionContext ctx{{0, 0, 0}, 10, false, 80, 5};

    EXPECT_EQ(2u, SelectBestTarget(targets, ctx));
}

TEST_F(QuestTargetSelectionTest, IgnoresHighLevelTargets)
{
    std::vector<PotentialTarget> targets = {
        {1, 80, {10, 10, 0}, true, 20, false},  // Too high level
        {2, 80, {50, 50, 0}, true, 8, false}    // Acceptable level
    };
    SelectionContext ctx{{0, 0, 0}, 5, false, 80, 5};  // Bot level 5

    EXPECT_EQ(2u, SelectBestTarget(targets, ctx));
}

TEST_F(QuestTargetSelectionTest, IgnoresElitesWhenCantFight)
{
    std::vector<PotentialTarget> targets = {
        {1, 80, {10, 10, 0}, true, 5, true},    // Elite, closest
        {2, 80, {50, 50, 0}, true, 5, false}    // Normal
    };
    SelectionContext ctx{{0, 0, 0}, 10, false, 80, 5};  // Can't fight elite

    EXPECT_EQ(2u, SelectBestTarget(targets, ctx));
}

TEST_F(QuestTargetSelectionTest, AcceptsElitesWhenCanFight)
{
    std::vector<PotentialTarget> targets = {
        {1, 80, {10, 10, 0}, true, 5, true},    // Elite, closest
        {2, 80, {50, 50, 0}, true, 5, false}    // Normal
    };
    SelectionContext ctx{{0, 0, 0}, 10, true, 80, 5};  // Can fight elite

    EXPECT_EQ(1u, SelectBestTarget(targets, ctx));
}

TEST_F(QuestTargetSelectionTest, ReturnsZeroWhenNoValidTargets)
{
    std::vector<PotentialTarget> targets = {
        {1, 99, {10, 10, 0}, true, 5, false},   // Wrong entry
        {2, 80, {50, 50, 0}, false, 5, false}   // Dead
    };
    SelectionContext ctx{{0, 0, 0}, 10, false, 80, 5};

    EXPECT_EQ(0u, SelectBestTarget(targets, ctx));
}

TEST_F(QuestTargetSelectionTest, ReturnsZeroWhenKillsComplete)
{
    std::vector<PotentialTarget> targets = {
        {1, 80, {10, 10, 0}, true, 5, false}
    };
    SelectionContext ctx{{0, 0, 0}, 10, false, 80, 0};  // No kills needed

    EXPECT_EQ(0u, SelectBestTarget(targets, ctx));
}

/**
 * @brief Tests for approach waypoint selection
 */
class ApproachWaypointTest : public ::testing::Test
{
protected:
    struct Position
    {
        float x, y, z;
    };

    struct ApproachWaypoint
    {
        uint32_t waypointId;
        Position pos;
        float radius;
    };

    float CalculateDistance(Position const& a, Position const& b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    bool HasReachedWaypoint(Position const& botPos, ApproachWaypoint const& wp)
    {
        return CalculateDistance(botPos, wp.pos) <= wp.radius;
    }

    ApproachWaypoint const* FindBestWaypoint(
        Position const& botPos,
        Position const& targetPos,
        std::vector<ApproachWaypoint> const& waypoints,
        float maxWpToTargetDist = 500.0f)
    {
        ApproachWaypoint const* best = nullptr;
        float bestWpToTarget = 999999.0f;

        for (auto const& wp : waypoints)
        {
            float wpToTarget = CalculateDistance(wp.pos, targetPos);

            // Only consider waypoints within reasonable distance of target
            if (wpToTarget > maxWpToTargetDist)
                continue;

            // Find waypoint closest to target
            if (wpToTarget < bestWpToTarget)
            {
                bestWpToTarget = wpToTarget;
                best = &wp;
            }
        }

        return best;
    }

    bool ShouldUseWaypoint(
        Position const& botPos,
        Position const& targetPos,
        ApproachWaypoint const& wp)
    {
        // Don't use if already reached
        if (HasReachedWaypoint(botPos, wp))
            return false;

        float botToTarget = CalculateDistance(botPos, targetPos);
        float wpToTarget = CalculateDistance(wp.pos, targetPos);

        // Use waypoint if it's closer to target than we are
        return wpToTarget < botToTarget;
    }
};

TEST_F(ApproachWaypointTest, HasReachedWaypointInRadius)
{
    Position botPos{100, 100, 0};
    ApproachWaypoint wp{1, {105, 100, 0}, 10.0f};  // 5 units away, radius 10

    EXPECT_TRUE(HasReachedWaypoint(botPos, wp));
}

TEST_F(ApproachWaypointTest, HasNotReachedWaypointOutsideRadius)
{
    Position botPos{100, 100, 0};
    ApproachWaypoint wp{1, {150, 100, 0}, 10.0f};  // 50 units away, radius 10

    EXPECT_FALSE(HasReachedWaypoint(botPos, wp));
}

TEST_F(ApproachWaypointTest, FindsBestWaypointClosestToTarget)
{
    Position botPos{0, 0, 0};
    Position targetPos{100, 100, 0};

    std::vector<ApproachWaypoint> waypoints = {
        {1, {50, 50, 0}, 10.0f},    // ~70 units to target
        {2, {80, 80, 0}, 10.0f},    // ~28 units to target (closest)
        {3, {30, 30, 0}, 10.0f}     // ~99 units to target
    };

    auto* best = FindBestWaypoint(botPos, targetPos, waypoints);
    ASSERT_NE(nullptr, best);
    EXPECT_EQ(2u, best->waypointId);
}

TEST_F(ApproachWaypointTest, ReturnsNullWhenWaypointsTooFarFromTarget)
{
    Position botPos{0, 0, 0};
    Position targetPos{1000, 1000, 0};  // Very far target

    std::vector<ApproachWaypoint> waypoints = {
        {1, {50, 50, 0}, 10.0f}  // Waypoint too far from target
    };

    auto* best = FindBestWaypoint(botPos, targetPos, waypoints, 100.0f);
    EXPECT_EQ(nullptr, best);
}

TEST_F(ApproachWaypointTest, ShouldUseWaypointWhenCloserToTarget)
{
    Position botPos{0, 0, 0};
    Position targetPos{100, 100, 0};  // ~141 units from bot
    ApproachWaypoint wp{1, {80, 80, 0}, 10.0f};  // ~28 units from target

    EXPECT_TRUE(ShouldUseWaypoint(botPos, targetPos, wp));
}

TEST_F(ApproachWaypointTest, ShouldNotUseWaypointWhenAlreadyCloser)
{
    Position botPos{90, 90, 0};  // ~14 units from target
    Position targetPos{100, 100, 0};
    ApproachWaypoint wp{1, {50, 50, 0}, 10.0f};  // ~70 units from target

    EXPECT_FALSE(ShouldUseWaypoint(botPos, targetPos, wp));
}

TEST_F(ApproachWaypointTest, ShouldNotUseWaypointWhenAlreadyReached)
{
    Position botPos{78, 78, 0};  // Within radius of waypoint
    Position targetPos{100, 100, 0};
    ApproachWaypoint wp{1, {80, 80, 0}, 10.0f};

    EXPECT_FALSE(ShouldUseWaypoint(botPos, targetPos, wp));
}

/**
 * @brief Tests for quest item loot detection
 */
class QuestLootTest : public ::testing::Test
{
protected:
    struct LootItem
    {
        uint32_t itemId;
        bool isQuestItem;
    };

    struct QuestItemRequirement
    {
        uint32_t itemId;
        uint32_t required;
        uint32_t current;
    };

    bool NeedsQuestItem(uint32_t itemId, std::vector<QuestItemRequirement> const& requirements)
    {
        for (auto const& req : requirements)
        {
            if (req.itemId == itemId && req.current < req.required)
                return true;
        }
        return false;
    }

    bool CreatureHasNeededQuestLoot(
        std::vector<LootItem> const& lootTable,
        std::vector<QuestItemRequirement> const& requirements)
    {
        for (auto const& loot : lootTable)
        {
            if (loot.isQuestItem && NeedsQuestItem(loot.itemId, requirements))
                return true;
        }
        return false;
    }
};

TEST_F(QuestLootTest, NeedsItemWhenBelowRequired)
{
    std::vector<QuestItemRequirement> reqs = {{750, 8, 3}};  // Need 8, have 3
    EXPECT_TRUE(NeedsQuestItem(750, reqs));
}

TEST_F(QuestLootTest, DoesNotNeedItemWhenComplete)
{
    std::vector<QuestItemRequirement> reqs = {{750, 8, 8}};  // Need 8, have 8
    EXPECT_FALSE(NeedsQuestItem(750, reqs));
}

TEST_F(QuestLootTest, DoesNotNeedUnrelatedItem)
{
    std::vector<QuestItemRequirement> reqs = {{750, 8, 3}};
    EXPECT_FALSE(NeedsQuestItem(999, reqs));  // Different item
}

TEST_F(QuestLootTest, CreatureHasNeededLoot)
{
    std::vector<LootItem> lootTable = {
        {100, false},  // Not quest item
        {750, true}    // Quest item
    };
    std::vector<QuestItemRequirement> reqs = {{750, 8, 3}};

    EXPECT_TRUE(CreatureHasNeededQuestLoot(lootTable, reqs));
}

TEST_F(QuestLootTest, CreatureDoesNotHaveNeededLoot)
{
    std::vector<LootItem> lootTable = {
        {100, false},
        {200, true}  // Different quest item
    };
    std::vector<QuestItemRequirement> reqs = {{750, 8, 3}};

    EXPECT_FALSE(CreatureHasNeededQuestLoot(lootTable, reqs));
}

TEST_F(QuestLootTest, CreatureHasLootButAlreadyComplete)
{
    std::vector<LootItem> lootTable = {{750, true}};
    std::vector<QuestItemRequirement> reqs = {{750, 8, 8}};  // Already complete

    EXPECT_FALSE(CreatureHasNeededQuestLoot(lootTable, reqs));
}
