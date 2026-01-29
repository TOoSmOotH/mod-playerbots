/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_MOCK_BETTERQUESTING_MANAGERS_H
#define _PLAYERBOT_MOCK_BETTERQUESTING_MANAGERS_H

#include <gmock/gmock.h>
#include "Bot/Interface/IBetterQuestingConfig.h"
#include "Bot/Interface/ICoreQuestDataMgr.h"
#include "Bot/Interface/IQuestGuideMgr.h"

/**
 * @brief Mock implementation of IBetterQuestingConfig for testing
 */
class MockBetterQuestingConfig : public IBetterQuestingConfig
{
public:
    MOCK_METHOD(void, LoadConfig, (bool reload), (override));
    MOCK_METHOD(bool, IsEnabled, (), (const, override));
    MOCK_METHOD(bool, IsPOIFallbackEnabled, (), (const, override));
    MOCK_METHOD(bool, IsPrerequisiteCheckEnabled, (), (const, override));
    MOCK_METHOD(bool, IsQuestChainAwarenessEnabled, (), (const, override));
    MOCK_METHOD(float, GetMaxSpawnDistance, (), (const, override));
    MOCK_METHOD(bool, IsLogFallbackEnabled, (), (const, override));
    MOCK_METHOD(bool, IsQuestPriorityBoostEnabled, (), (const, override));
    MOCK_METHOD(bool, IsPathValidationEnabled, (), (const, override));
    MOCK_METHOD(bool, IsQuestGuideEnabled, (), (const, override));
    MOCK_METHOD(bool, IsQuestGuideStrictMode, (), (const, override));
    MOCK_METHOD(int32, GetGuidePriorityBonus, (), (const, override));
    MOCK_METHOD(bool, IsAutoGuideSelectionEnabled, (), (const, override));
    MOCK_METHOD(bool, IsQuestGuideZoneTransitionEnabled, (), (const, override));
    MOCK_METHOD(bool, IsQuestGuideSkipUnavailableEnabled, (), (const, override));
    MOCK_METHOD(bool, IsMultiLevelZProbeEnabled, (), (const, override));
    MOCK_METHOD(float, GetMaxZProbeDepth, (), (const, override));
    MOCK_METHOD(bool, IsApproachWaypointsEnabled, (), (const, override));
    MOCK_METHOD(uint32, GetMaxApproachWaypoints, (), (const, override));

    // Helper methods to set up default expectations
    void SetupDefaults()
    {
        using ::testing::Return;
        ON_CALL(*this, IsEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, IsPOIFallbackEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, IsPrerequisiteCheckEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, IsQuestChainAwarenessEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, GetMaxSpawnDistance()).WillByDefault(Return(2500.0f));
        ON_CALL(*this, IsLogFallbackEnabled()).WillByDefault(Return(false));
        ON_CALL(*this, IsQuestPriorityBoostEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, IsPathValidationEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, IsQuestGuideEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, IsQuestGuideStrictMode()).WillByDefault(Return(true));
        ON_CALL(*this, GetGuidePriorityBonus()).WillByDefault(Return(50));
        ON_CALL(*this, IsAutoGuideSelectionEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, IsQuestGuideZoneTransitionEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, IsQuestGuideSkipUnavailableEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, IsMultiLevelZProbeEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, GetMaxZProbeDepth()).WillByDefault(Return(150.0f));
        ON_CALL(*this, IsApproachWaypointsEnabled()).WillByDefault(Return(true));
        ON_CALL(*this, GetMaxApproachWaypoints()).WillByDefault(Return(10));
    }
};

/**
 * @brief Mock implementation of ICoreQuestDataMgr for testing
 */
class MockCoreQuestDataMgr : public ICoreQuestDataMgr
{
public:
    MOCK_METHOD(void, LoadData, (), (override));
    MOCK_METHOD(bool, IsDataLoaded, (), (const, override));
    MOCK_METHOD(bool, GetQuestPOI, (uint32 questId, int32 objectiveIndex, uint32 mapId, std::vector<SpawnPoint>& positions), (override));
    MOCK_METHOD(std::vector<SpawnPoint>, GetNpcSpawns, (uint32 npcId, uint32 mapId), (override));
    MOCK_METHOD(std::vector<SpawnPoint>, GetObjectSpawns, (uint32 objectId, uint32 mapId), (override));
    MOCK_METHOD(std::vector<SpawnPoint>, GetQuestStarterLocations, (uint32 questId, uint32 mapId), (override));
    MOCK_METHOD(std::vector<SpawnPoint>, GetQuestEnderLocations, (uint32 questId, uint32 mapId), (override));
    MOCK_METHOD(bool, HasPrerequisites, (uint32 questId), (const, override));
    MOCK_METHOD(bool, ArePrerequisitesMet, (Player* player, uint32 questId), (const, override));
    MOCK_METHOD(std::vector<uint32>, GetPrerequisiteQuests, (uint32 questId), (const, override));
    MOCK_METHOD(uint32, GetNextQuestInChain, (uint32 questId), (const, override));
    MOCK_METHOD(std::vector<uint32>, GetRecommendedQuests, (Player* player, uint32 zoneId, uint32 limit), (override));
    MOCK_METHOD(std::vector<ApproachWaypoint>, GetApproachWaypoints, (ApproachWaypointTargetType type, uint32 targetId, uint32 mapId), (override));
    MOCK_METHOD(std::vector<ApproachWaypoint>, GetZoneApproachWaypoints, (uint32 zoneId, uint32 mapId), (override));
    MOCK_METHOD(std::vector<ApproachWaypoint>, GetNpcApproachWaypoints, (uint32 npcId, uint32 mapId), (override));
    MOCK_METHOD(std::vector<ApproachWaypoint>, GetObjectApproachWaypoints, (uint32 objectId, uint32 mapId), (override));
    MOCK_METHOD(std::vector<ApproachWaypoint>, GetQuestApproachWaypoints, (uint32 questId, uint32 mapId), (override));
    MOCK_METHOD(uint32, GetNpcSpawnCount, (), (const, override));
    MOCK_METHOD(uint32, GetObjectSpawnCount, (), (const, override));
    MOCK_METHOD(uint32, GetApproachWaypointCount, (), (const, override));

    // Helper methods to set up default expectations
    void SetupDefaults()
    {
        using ::testing::Return;
        using ::testing::_;
        ON_CALL(*this, IsDataLoaded()).WillByDefault(Return(true));
        ON_CALL(*this, GetNpcSpawns(_, _)).WillByDefault(Return(std::vector<SpawnPoint>{}));
        ON_CALL(*this, GetObjectSpawns(_, _)).WillByDefault(Return(std::vector<SpawnPoint>{}));
        ON_CALL(*this, GetQuestStarterLocations(_, _)).WillByDefault(Return(std::vector<SpawnPoint>{}));
        ON_CALL(*this, GetQuestEnderLocations(_, _)).WillByDefault(Return(std::vector<SpawnPoint>{}));
        ON_CALL(*this, HasPrerequisites(_)).WillByDefault(Return(false));
        ON_CALL(*this, ArePrerequisitesMet(_, _)).WillByDefault(Return(true));
        ON_CALL(*this, GetPrerequisiteQuests(_)).WillByDefault(Return(std::vector<uint32>{}));
        ON_CALL(*this, GetNextQuestInChain(_)).WillByDefault(Return(0));
        ON_CALL(*this, GetRecommendedQuests(_, _, _)).WillByDefault(Return(std::vector<uint32>{}));
        ON_CALL(*this, GetApproachWaypoints(_, _, _)).WillByDefault(Return(std::vector<ApproachWaypoint>{}));
        ON_CALL(*this, GetZoneApproachWaypoints(_, _)).WillByDefault(Return(std::vector<ApproachWaypoint>{}));
        ON_CALL(*this, GetNpcApproachWaypoints(_, _)).WillByDefault(Return(std::vector<ApproachWaypoint>{}));
        ON_CALL(*this, GetObjectApproachWaypoints(_, _)).WillByDefault(Return(std::vector<ApproachWaypoint>{}));
        ON_CALL(*this, GetQuestApproachWaypoints(_, _)).WillByDefault(Return(std::vector<ApproachWaypoint>{}));
        ON_CALL(*this, GetNpcSpawnCount()).WillByDefault(Return(0));
        ON_CALL(*this, GetObjectSpawnCount()).WillByDefault(Return(0));
        ON_CALL(*this, GetApproachWaypointCount()).WillByDefault(Return(0));
    }
};

/**
 * @brief Mock implementation of IQuestGuideMgr for testing
 */
class MockQuestGuideMgr : public IQuestGuideMgr
{
public:
    MOCK_METHOD(void, LoadData, (), (override));
    MOCK_METHOD(bool, IsDataLoaded, (), (const, override));
    MOCK_METHOD(QuestGuideData const*, GetGuide, (uint32 guideId), (const, override));
    MOCK_METHOD(QuestGuideData const*, GetBestGuideForPlayer, (Player* player), (const, override));
    MOCK_METHOD(std::vector<QuestGuideData const*>, GetGuidesForFaction, (uint8 faction), (const, override));
    MOCK_METHOD(QuestGuideStep const*, GetCurrentStep, (Player* player), (const, override));
    MOCK_METHOD(QuestGuideStep const*, GetNextStep, (Player* player), (const, override));
    MOCK_METHOD(std::vector<QuestGuideStep const*>, GetAvailableSteps, (Player* player, uint32 limit), (const, override));
    MOCK_METHOD(QuestGuideStep const*, GetStepByOrder, (uint32 guideId, uint32 stepOrder), (const, override));
    MOCK_METHOD(std::vector<QuestGuideStep const*>, GetGuideSteps, (uint32 guideId), (const, override));
    MOCK_METHOD(bool, ShouldTransitionZone, (Player* player, uint32& targetZoneId), (const, override));
    MOCK_METHOD(QuestGuideZoneTransition const*, GetNextTransition, (Player* player), (const, override));
    MOCK_METHOD(int32, GetGuideQuestPriorityBonus, (Player* player, uint32 questId), (const, override));
    MOCK_METHOD(bool, IsQuestInGuide, (uint32 guideId, uint32 questId), (const, override));
    MOCK_METHOD(bool, IsQuestInActiveGuide, (Player* player, uint32 questId), (const, override));
    MOCK_METHOD(QuestGuideProgress const*, GetPlayerProgress, (uint32 guid), (const, override));
    MOCK_METHOD(void, SetPlayerGuide, (Player* player, uint32 guideId), (override));
    MOCK_METHOD(void, AdvancePlayerStep, (Player* player), (override));
    MOCK_METHOD(void, SyncPlayerProgress, (Player* player), (override));
    MOCK_METHOD(void, SavePlayerProgress, (Player* player), (override));
    MOCK_METHOD(void, LoadPlayerProgress, (uint32 guid), (override));
    MOCK_METHOD(void, DeletePlayerProgress, (uint32 guid), (override));
    MOCK_METHOD(uint32, FindStepForQuest, (uint32 guideId, uint32 questId), (const, override));
    MOCK_METHOD(bool, IsStepAvailable, (Player* player, QuestGuideStep const* step), (const, override));
    MOCK_METHOD(uint32, GetGuideCount, (), (const, override));
    MOCK_METHOD(uint32, GetTotalStepCount, (), (const, override));

    // Helper methods to set up default expectations
    void SetupDefaults()
    {
        using ::testing::Return;
        using ::testing::_;
        ON_CALL(*this, IsDataLoaded()).WillByDefault(Return(true));
        ON_CALL(*this, GetGuide(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, GetBestGuideForPlayer(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, GetGuidesForFaction(_)).WillByDefault(Return(std::vector<QuestGuideData const*>{}));
        ON_CALL(*this, GetCurrentStep(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, GetNextStep(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, GetAvailableSteps(_, _)).WillByDefault(Return(std::vector<QuestGuideStep const*>{}));
        ON_CALL(*this, GetStepByOrder(_, _)).WillByDefault(Return(nullptr));
        ON_CALL(*this, GetGuideSteps(_)).WillByDefault(Return(std::vector<QuestGuideStep const*>{}));
        ON_CALL(*this, ShouldTransitionZone(_, _)).WillByDefault(Return(false));
        ON_CALL(*this, GetNextTransition(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, GetGuideQuestPriorityBonus(_, _)).WillByDefault(Return(0));
        ON_CALL(*this, IsQuestInGuide(_, _)).WillByDefault(Return(false));
        ON_CALL(*this, IsQuestInActiveGuide(_, _)).WillByDefault(Return(false));
        ON_CALL(*this, GetPlayerProgress(_)).WillByDefault(Return(nullptr));
        ON_CALL(*this, FindStepForQuest(_, _)).WillByDefault(Return(0));
        ON_CALL(*this, IsStepAvailable(_, _)).WillByDefault(Return(false));
        ON_CALL(*this, GetGuideCount()).WillByDefault(Return(0));
        ON_CALL(*this, GetTotalStepCount()).WillByDefault(Return(0));
    }
};

#endif // _PLAYERBOT_MOCK_BETTERQUESTING_MANAGERS_H
