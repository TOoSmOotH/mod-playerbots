/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_IQUEST_GUIDE_MGR_H
#define _PLAYERBOT_IQUEST_GUIDE_MGR_H

#include "Mgr/Quest/BetterQuestingTypes.h"
#include <vector>

class Player;

/**
 * @brief Interface for quest guide management
 *
 * This interface abstracts the quest guide system that provides
 * database-defined leveling paths similar to RestedXP guides.
 */
class IQuestGuideMgr
{
public:
    virtual ~IQuestGuideMgr() = default;

    // Data loading
    virtual void LoadData() = 0;
    virtual bool IsDataLoaded() const = 0;

    // Guide selection
    virtual QuestGuideData const* GetGuide(uint32 guideId) const = 0;
    virtual QuestGuideData const* GetBestGuideForPlayer(Player* player) const = 0;
    virtual std::vector<QuestGuideData const*> GetGuidesForFaction(uint8 faction) const = 0;

    // Step navigation
    virtual QuestGuideStep const* GetCurrentStep(Player* player) const = 0;
    virtual QuestGuideStep const* GetNextStep(Player* player) const = 0;
    virtual std::vector<QuestGuideStep const*> GetAvailableSteps(Player* player, uint32 limit = 5) const = 0;
    virtual QuestGuideStep const* GetStepByOrder(uint32 guideId, uint32 stepOrder) const = 0;
    virtual std::vector<QuestGuideStep const*> GetGuideSteps(uint32 guideId) const = 0;

    // Zone transitions
    virtual bool ShouldTransitionZone(Player* player, uint32& targetZoneId) const = 0;
    virtual QuestGuideZoneTransition const* GetNextTransition(Player* player) const = 0;

    // Scoring integration
    virtual int32 GetGuideQuestPriorityBonus(Player* player, uint32 questId) const = 0;
    virtual bool IsQuestInGuide(uint32 guideId, uint32 questId) const = 0;
    virtual bool IsQuestInActiveGuide(Player* player, uint32 questId) const = 0;

    // Progress management
    virtual QuestGuideProgress const* GetPlayerProgress(uint32 guid) const = 0;
    virtual void SetPlayerGuide(Player* player, uint32 guideId) = 0;
    virtual void AdvancePlayerStep(Player* player) = 0;
    virtual void SyncPlayerProgress(Player* player) = 0;
    virtual void SavePlayerProgress(Player* player) = 0;
    virtual void LoadPlayerProgress(uint32 guid) = 0;
    virtual void DeletePlayerProgress(uint32 guid) = 0;

    // Active quest state persistence (for logout/login continuity)
    virtual void SetActiveQuestState(Player* player, uint32 questId, uint8 subStatus) = 0;
    virtual void ClearActiveQuestState(Player* player) = 0;

    // Utility
    virtual uint32 FindStepForQuest(uint32 guideId, uint32 questId) const = 0;
    virtual bool IsStepAvailable(Player* player, QuestGuideStep const* step) const = 0;

    // Statistics
    virtual uint32 GetGuideCount() const = 0;
    virtual uint32 GetTotalStepCount() const = 0;
};

#endif // _PLAYERBOT_IQUEST_GUIDE_MGR_H
