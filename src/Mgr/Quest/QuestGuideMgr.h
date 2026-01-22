/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#ifndef MOD_QUEST_GUIDE_MGR_H
#define MOD_QUEST_GUIDE_MGR_H

#include "Common.h"
#include <unordered_map>
#include <vector>
#include <map>

class Player;

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
};

class QuestGuideMgr
{
public:
    static QuestGuideMgr* instance();

    void LoadData();
    bool IsDataLoaded() const { return _dataLoaded; }

    // Guide selection
    QuestGuideData const* GetGuide(uint32 guideId) const;
    QuestGuideData const* GetBestGuideForPlayer(Player* player) const;
    std::vector<QuestGuideData const*> GetGuidesForFaction(uint8 faction) const;

    // Step navigation
    QuestGuideStep const* GetCurrentStep(Player* player) const;
    QuestGuideStep const* GetNextStep(Player* player) const;
    std::vector<QuestGuideStep const*> GetAvailableSteps(Player* player, uint32 limit = 5) const;
    QuestGuideStep const* GetStepByOrder(uint32 guideId, uint32 stepOrder) const;
    std::vector<QuestGuideStep const*> GetGuideSteps(uint32 guideId) const;

    // Zone transitions
    bool ShouldTransitionZone(Player* player, uint32& targetZoneId) const;
    QuestGuideZoneTransition const* GetNextTransition(Player* player) const;

    // Scoring integration
    int32 GetGuideQuestPriorityBonus(Player* player, uint32 questId) const;
    bool IsQuestInGuide(uint32 guideId, uint32 questId) const;
    bool IsQuestInActiveGuide(Player* player, uint32 questId) const;

    // Progress management
    QuestGuideProgress const* GetPlayerProgress(uint32 guid) const;
    void SetPlayerGuide(Player* player, uint32 guideId);
    void AdvancePlayerStep(Player* player);
    void SyncPlayerProgress(Player* player);
    void SavePlayerProgress(Player* player);
    void LoadPlayerProgress(uint32 guid);
    void DeletePlayerProgress(uint32 guid);

    // Utility
    uint32 FindStepForQuest(uint32 guideId, uint32 questId) const;
    bool IsStepAvailable(Player* player, QuestGuideStep const* step) const;

    // Statistics
    uint32 GetGuideCount() const { return static_cast<uint32>(_guides.size()); }
    uint32 GetTotalStepCount() const { return _totalStepCount; }

private:
    QuestGuideMgr() = default;
    ~QuestGuideMgr() = default;

    void LoadGuides();
    void LoadGuideSteps();
    void LoadZoneTransitions();

    bool _dataLoaded = false;
    uint32 _totalStepCount = 0;

    // Guide data indexed by guideId
    std::unordered_map<uint32, QuestGuideData> _guides;

    // Guide steps indexed by (guideId, stepOrder)
    std::map<std::pair<uint32, uint32>, QuestGuideStep> _guideSteps;

    // Quest to step mapping for quick lookup (guideId -> questId -> stepOrder)
    std::unordered_map<uint32, std::unordered_map<uint32, uint32>> _questToStep;

    // Zone transitions indexed by guideId
    std::unordered_multimap<uint32, QuestGuideZoneTransition> _zoneTransitions;

    // Player progress (in-memory cache, guid -> progress)
    std::unordered_map<uint32, QuestGuideProgress> _playerProgress;
};

#define sQuestGuideMgr QuestGuideMgr::instance()

#endif // MOD_QUEST_GUIDE_MGR_H
