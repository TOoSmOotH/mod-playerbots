/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <map>
#include <cstdint>

/**
 * @brief Tests for QuestGuideMgr step progression logic
 *
 * These tests validate the blocking behavior for non-optional quest guide steps.
 * The key invariant is: non-optional steps BLOCK progression until completed,
 * while optional steps can be skipped if unavailable.
 */

/**
 * @brief Mirrors QuestGuideStep from QuestGuideMgr.h
 */
struct TestQuestGuideStep
{
    uint32_t guideId;
    uint32_t stepOrder;
    uint32_t questId;
    uint8_t minLevel;
    uint8_t maxLevel;
    uint32_t zoneId;
    bool isOptional;
    bool skipIfCompleted;
};

/**
 * @brief Mirrors QuestGuideProgress from QuestGuideMgr.h
 */
struct TestQuestGuideProgress
{
    uint32_t guid;
    uint32_t guideId;
    uint32_t currentStep;
};

/**
 * @brief Test harness that mirrors QuestGuideMgr logic
 *
 * This implements the same algorithms as QuestGuideMgr but in a testable way
 * without requiring actual Player objects or database access.
 */
class QuestGuideMgrLogicTest : public ::testing::Test
{
protected:
    std::vector<TestQuestGuideStep> steps_;
    TestQuestGuideProgress progress_;
    std::map<uint32_t, bool> questCompleted_;  // questId -> completed
    std::map<uint32_t, bool> stepAvailable_;   // stepOrder -> available
    bool skipUnavailableEnabled_ = true;

    void SetUp() override
    {
        steps_.clear();
        progress_ = {1, 1, 0};  // guid=1, guideId=1, currentStep=0
        questCompleted_.clear();
        stepAvailable_.clear();
        skipUnavailableEnabled_ = true;
    }

    void AddStep(uint32_t stepOrder, uint32_t questId, bool isOptional, bool available = true)
    {
        TestQuestGuideStep step;
        step.guideId = 1;
        step.stepOrder = stepOrder;
        step.questId = questId;
        step.minLevel = 1;
        step.maxLevel = 60;
        step.zoneId = 0;
        step.isOptional = isOptional;
        step.skipIfCompleted = false;
        steps_.push_back(step);
        stepAvailable_[stepOrder] = available;
    }

    void SetQuestCompleted(uint32_t questId, bool completed)
    {
        questCompleted_[questId] = completed;
    }

    void SetStepAvailable(uint32_t stepOrder, bool available)
    {
        stepAvailable_[stepOrder] = available;
    }

    bool IsQuestCompleted(uint32_t questId) const
    {
        auto it = questCompleted_.find(questId);
        return it != questCompleted_.end() && it->second;
    }

    bool IsStepAvailable(uint32_t stepOrder) const
    {
        auto it = stepAvailable_.find(stepOrder);
        return it != stepAvailable_.end() && it->second;
    }

    TestQuestGuideStep const* GetStepByOrder(uint32_t stepOrder) const
    {
        for (const auto& step : steps_)
        {
            if (step.stepOrder == stepOrder)
                return &step;
        }
        return nullptr;
    }

    /**
     * @brief Implementation of GetNextStep logic
     *
     * Returns the next available step after currentStep.
     * BLOCKS on non-optional unavailable steps (returns nullptr).
     * SKIPS optional unavailable steps.
     */
    TestQuestGuideStep const* GetNextStep() const
    {
        uint32_t nextStepOrder = progress_.currentStep + 1;

        while (true)
        {
            TestQuestGuideStep const* step = GetStepByOrder(nextStepOrder);
            if (!step)
                return nullptr;  // No more steps

            if (IsStepAvailable(step->stepOrder))
                return step;

            // Only skip optional steps; non-optional block progression
            if (!step->isOptional)
                return nullptr;

            ++nextStepOrder;
        }
    }

    /**
     * @brief Implementation of GetAvailableSteps logic
     *
     * Returns up to `limit` available steps starting from currentStep.
     * BLOCKS at non-optional unavailable steps (stops looking further).
     * SKIPS optional unavailable steps.
     */
    std::vector<TestQuestGuideStep const*> GetAvailableSteps(uint32_t limit) const
    {
        std::vector<TestQuestGuideStep const*> result;
        uint32_t stepOrder = progress_.currentStep;

        while (result.size() < limit)
        {
            TestQuestGuideStep const* step = GetStepByOrder(stepOrder);
            if (!step)
                break;  // No more steps

            if (IsStepAvailable(step->stepOrder))
            {
                result.push_back(step);
            }
            else if (!step->isOptional)
            {
                // Non-optional unavailable step blocks further lookups
                break;
            }

            ++stepOrder;
        }

        return result;
    }

    /**
     * @brief Implementation of SyncPlayerProgress logic
     *
     * Finds the first incomplete step.
     * Only skips optional unavailable steps.
     * Non-optional steps always block (even if unavailable).
     */
    uint32_t SyncPlayerProgress() const
    {
        uint32_t stepOrder = 0;

        while (true)
        {
            TestQuestGuideStep const* step = GetStepByOrder(stepOrder);
            if (!step)
                break;  // End of guide

            // Check if player has completed this quest
            if (!IsQuestCompleted(step->questId))
            {
                // Only skip optional unavailable steps
                if (step->isOptional && !IsStepAvailable(step->stepOrder))
                {
                    if (skipUnavailableEnabled_)
                    {
                        ++stepOrder;
                        continue;
                    }
                }
                // Non-optional steps always block (even if unavailable)
                break;
            }

            ++stepOrder;
        }

        return stepOrder;
    }
};

// ============================================================================
// GetNextStep Tests
// ============================================================================

TEST_F(QuestGuideMgrLogicTest, GetNextStep_ReturnsNextAvailableStep)
{
    AddStep(0, 100, false, true);   // Step 0: non-optional, available (current)
    AddStep(1, 101, false, true);   // Step 1: non-optional, available
    progress_.currentStep = 0;

    auto* next = GetNextStep();
    ASSERT_NE(nullptr, next);
    EXPECT_EQ(1u, next->stepOrder);
    EXPECT_EQ(101u, next->questId);
}

TEST_F(QuestGuideMgrLogicTest, GetNextStep_BlocksOnNonOptionalUnavailable)
{
    AddStep(0, 100, false, true);   // Step 0: non-optional, available (current)
    AddStep(1, 101, false, false);  // Step 1: non-optional, UNAVAILABLE
    AddStep(2, 102, false, true);   // Step 2: non-optional, available
    progress_.currentStep = 0;

    auto* next = GetNextStep();
    EXPECT_EQ(nullptr, next);  // Should block, not skip to step 2
}

TEST_F(QuestGuideMgrLogicTest, GetNextStep_SkipsOptionalUnavailable)
{
    AddStep(0, 100, false, true);   // Step 0: non-optional, available (current)
    AddStep(1, 101, true, false);   // Step 1: OPTIONAL, unavailable
    AddStep(2, 102, false, true);   // Step 2: non-optional, available
    progress_.currentStep = 0;

    auto* next = GetNextStep();
    ASSERT_NE(nullptr, next);
    EXPECT_EQ(2u, next->stepOrder);  // Should skip step 1 and return step 2
}

TEST_F(QuestGuideMgrLogicTest, GetNextStep_SkipsMultipleOptionalUnavailable)
{
    AddStep(0, 100, false, true);   // Step 0: non-optional, available (current)
    AddStep(1, 101, true, false);   // Step 1: optional, unavailable
    AddStep(2, 102, true, false);   // Step 2: optional, unavailable
    AddStep(3, 103, true, false);   // Step 3: optional, unavailable
    AddStep(4, 104, false, true);   // Step 4: non-optional, available
    progress_.currentStep = 0;

    auto* next = GetNextStep();
    ASSERT_NE(nullptr, next);
    EXPECT_EQ(4u, next->stepOrder);  // Should skip steps 1-3 and return step 4
}

TEST_F(QuestGuideMgrLogicTest, GetNextStep_ReturnsNullAtEndOfGuide)
{
    AddStep(0, 100, false, true);   // Step 0: only step
    progress_.currentStep = 0;

    auto* next = GetNextStep();
    EXPECT_EQ(nullptr, next);  // No step after step 0
}

// ============================================================================
// GetAvailableSteps Tests
// ============================================================================

TEST_F(QuestGuideMgrLogicTest, GetAvailableSteps_ReturnsAvailableSteps)
{
    AddStep(0, 100, false, true);
    AddStep(1, 101, false, true);
    AddStep(2, 102, false, true);
    progress_.currentStep = 0;

    auto steps = GetAvailableSteps(3);
    ASSERT_EQ(3u, steps.size());
    EXPECT_EQ(0u, steps[0]->stepOrder);
    EXPECT_EQ(1u, steps[1]->stepOrder);
    EXPECT_EQ(2u, steps[2]->stepOrder);
}

TEST_F(QuestGuideMgrLogicTest, GetAvailableSteps_StopsAtNonOptionalUnavailable)
{
    AddStep(0, 100, false, true);   // available
    AddStep(1, 101, false, true);   // available
    AddStep(2, 102, false, false);  // NON-OPTIONAL, unavailable - BLOCKS
    AddStep(3, 103, false, true);   // available (should not be returned)
    progress_.currentStep = 0;

    auto steps = GetAvailableSteps(10);
    ASSERT_EQ(2u, steps.size());  // Only steps 0 and 1
    EXPECT_EQ(0u, steps[0]->stepOrder);
    EXPECT_EQ(1u, steps[1]->stepOrder);
}

TEST_F(QuestGuideMgrLogicTest, GetAvailableSteps_SkipsOptionalUnavailable)
{
    AddStep(0, 100, false, true);   // available
    AddStep(1, 101, true, false);   // OPTIONAL, unavailable - SKIP
    AddStep(2, 102, false, true);   // available
    progress_.currentStep = 0;

    auto steps = GetAvailableSteps(10);
    ASSERT_EQ(2u, steps.size());  // Steps 0 and 2 (skipped 1)
    EXPECT_EQ(0u, steps[0]->stepOrder);
    EXPECT_EQ(2u, steps[1]->stepOrder);
}

TEST_F(QuestGuideMgrLogicTest, GetAvailableSteps_RespectsLimit)
{
    AddStep(0, 100, false, true);
    AddStep(1, 101, false, true);
    AddStep(2, 102, false, true);
    AddStep(3, 103, false, true);
    progress_.currentStep = 0;

    auto steps = GetAvailableSteps(2);
    ASSERT_EQ(2u, steps.size());
    EXPECT_EQ(0u, steps[0]->stepOrder);
    EXPECT_EQ(1u, steps[1]->stepOrder);
}

// ============================================================================
// SyncPlayerProgress Tests
// ============================================================================

TEST_F(QuestGuideMgrLogicTest, SyncProgress_FindsFirstIncompleteStep)
{
    AddStep(0, 100, false, true);
    AddStep(1, 101, false, true);
    AddStep(2, 102, false, true);
    SetQuestCompleted(100, true);
    SetQuestCompleted(101, true);
    // 102 not completed

    uint32_t syncedStep = SyncPlayerProgress();
    EXPECT_EQ(2u, syncedStep);  // First incomplete is step 2
}

TEST_F(QuestGuideMgrLogicTest, SyncProgress_BlocksOnNonOptionalUnavailable)
{
    AddStep(0, 100, false, true);   // completed
    AddStep(1, 101, false, false);  // NON-OPTIONAL, unavailable, incomplete
    AddStep(2, 102, false, true);   // available, incomplete
    SetQuestCompleted(100, true);
    // 101 and 102 not completed

    uint32_t syncedStep = SyncPlayerProgress();
    EXPECT_EQ(1u, syncedStep);  // Blocks at step 1 (non-optional)
}

TEST_F(QuestGuideMgrLogicTest, SyncProgress_SkipsOptionalUnavailable)
{
    AddStep(0, 100, false, true);   // completed
    AddStep(1, 101, true, false);   // OPTIONAL, unavailable, incomplete
    AddStep(2, 102, false, true);   // available, incomplete
    SetQuestCompleted(100, true);
    // 101 and 102 not completed

    uint32_t syncedStep = SyncPlayerProgress();
    EXPECT_EQ(2u, syncedStep);  // Skips step 1 (optional), lands on step 2
}

TEST_F(QuestGuideMgrLogicTest, SyncProgress_AllComplete)
{
    AddStep(0, 100, false, true);
    AddStep(1, 101, false, true);
    AddStep(2, 102, false, true);
    SetQuestCompleted(100, true);
    SetQuestCompleted(101, true);
    SetQuestCompleted(102, true);

    uint32_t syncedStep = SyncPlayerProgress();
    EXPECT_EQ(3u, syncedStep);  // Past the last step
}

TEST_F(QuestGuideMgrLogicTest, SyncProgress_EmptyGuide)
{
    // No steps added
    uint32_t syncedStep = SyncPlayerProgress();
    EXPECT_EQ(0u, syncedStep);
}

// ============================================================================
// Scenario Tests - Real World Use Cases
// ============================================================================

TEST_F(QuestGuideMgrLogicTest, Scenario_HumanStartingZoneWithClassQuest)
{
    // Simulates: Human starting zone with a class-specific letter quest
    // Steps 0-17 are regular quests, step 18 onwards continues
    // Step 5 is an optional class letter quest that only Warriors can do

    // Regular quests before class quest
    for (uint32_t i = 0; i < 5; ++i)
        AddStep(i, 100 + i, false, true);

    // Class-specific quest (optional) - unavailable for non-Warriors
    AddStep(5, 105, true, false);

    // Regular quests after class quest
    for (uint32_t i = 6; i < 20; ++i)
        AddStep(i, 100 + i, false, true);

    // Complete steps 0-4
    for (uint32_t i = 0; i < 5; ++i)
        SetQuestCompleted(100 + i, true);

    // Sync should skip the optional class quest and land on step 6
    uint32_t syncedStep = SyncPlayerProgress();
    EXPECT_EQ(6u, syncedStep);

    // GetNextStep from step 5 should also skip to step 6
    progress_.currentStep = 5;
    auto* next = GetNextStep();
    ASSERT_NE(nullptr, next);
    EXPECT_EQ(6u, next->stepOrder);
}

TEST_F(QuestGuideMgrLogicTest, Scenario_BotMustCompletePrerequisites)
{
    // Simulates: Bot at step 18 but hasn't done steps 1-17
    // Non-optional steps must block

    for (uint32_t i = 0; i < 20; ++i)
        AddStep(i, 100 + i, false, true);

    // Nothing completed yet
    uint32_t syncedStep = SyncPlayerProgress();
    EXPECT_EQ(0u, syncedStep);  // Must start at step 0

    // Complete some steps out of order (e.g., step 18 somehow completed)
    SetQuestCompleted(118, true);
    syncedStep = SyncPlayerProgress();
    EXPECT_EQ(0u, syncedStep);  // Still blocked at step 0 because 100 not done

    // Complete step 0
    SetQuestCompleted(100, true);
    syncedStep = SyncPlayerProgress();
    EXPECT_EQ(1u, syncedStep);  // Now at step 1
}

TEST_F(QuestGuideMgrLogicTest, Scenario_MixedOptionalAndRequired)
{
    // Mix of optional and required steps
    AddStep(0, 100, false, true);   // required, available
    AddStep(1, 101, true, false);   // optional, unavailable
    AddStep(2, 102, false, true);   // required, available
    AddStep(3, 103, true, true);    // optional, available
    AddStep(4, 104, false, false);  // required, UNAVAILABLE - blocks
    AddStep(5, 105, false, true);   // required, available

    progress_.currentStep = 0;
    auto steps = GetAvailableSteps(10);

    // Should get: 0, 2, 3 (skip 1, blocked at 4)
    ASSERT_EQ(3u, steps.size());
    EXPECT_EQ(0u, steps[0]->stepOrder);
    EXPECT_EQ(2u, steps[1]->stepOrder);
    EXPECT_EQ(3u, steps[2]->stepOrder);
}
