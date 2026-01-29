/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#ifndef MOD_QUEST_GUIDE_MGR_H
#define MOD_QUEST_GUIDE_MGR_H

#include "Bot/Interface/IQuestGuideMgr.h"
#include <unordered_map>
#include <map>

class Player;

class QuestGuideMgr : public IQuestGuideMgr
{
public:
    static QuestGuideMgr* instance();

    void LoadData() override;
    bool IsDataLoaded() const override { return _dataLoaded; }

    // Guide selection
    QuestGuideData const* GetGuide(uint32 guideId) const override;
    QuestGuideData const* GetBestGuideForPlayer(Player* player) const override;
    std::vector<QuestGuideData const*> GetGuidesForFaction(uint8 faction) const override;

    // Step navigation
    QuestGuideStep const* GetCurrentStep(Player* player) const override;
    QuestGuideStep const* GetNextStep(Player* player) const override;
    std::vector<QuestGuideStep const*> GetAvailableSteps(Player* player, uint32 limit = 5) const override;
    QuestGuideStep const* GetStepByOrder(uint32 guideId, uint32 stepOrder) const override;
    std::vector<QuestGuideStep const*> GetGuideSteps(uint32 guideId) const override;

    // Zone transitions
    bool ShouldTransitionZone(Player* player, uint32& targetZoneId) const override;
    QuestGuideZoneTransition const* GetNextTransition(Player* player) const override;

    // Scoring integration
    int32 GetGuideQuestPriorityBonus(Player* player, uint32 questId) const override;
    bool IsQuestInGuide(uint32 guideId, uint32 questId) const override;
    bool IsQuestInActiveGuide(Player* player, uint32 questId) const override;

    // Progress management
    QuestGuideProgress const* GetPlayerProgress(uint32 guid) const override;
    void SetPlayerGuide(Player* player, uint32 guideId) override;
    void AdvancePlayerStep(Player* player) override;
    void SyncPlayerProgress(Player* player) override;
    void SavePlayerProgress(Player* player) override;
    void LoadPlayerProgress(uint32 guid) override;
    void DeletePlayerProgress(uint32 guid) override;

    // Active quest state persistence (for logout/login continuity)
    void SetActiveQuestState(Player* player, uint32 questId, uint8 subStatus) override;
    void ClearActiveQuestState(Player* player) override;

    // Utility
    uint32 FindStepForQuest(uint32 guideId, uint32 questId) const override;
    bool IsStepAvailable(Player* player, QuestGuideStep const* step) const override;

    // Statistics
    uint32 GetGuideCount() const override { return static_cast<uint32>(_guides.size()); }
    uint32 GetTotalStepCount() const override { return _totalStepCount; }

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
