/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#include "QuestGuideMgr.h"
#include "BetterQuestingConfig.h"
#include "Bot/Core/ManagerRegistry.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "QuestDef.h"
#include <algorithm>

// Faction constants
constexpr uint8 FACTION_ALLIANCE = 0;
constexpr uint8 FACTION_HORDE = 1;
constexpr uint8 FACTION_BOTH = 2;

QuestGuideMgr* QuestGuideMgr::instance()
{
    static QuestGuideMgr instance;
    return &instance;
}

void QuestGuideMgr::LoadData()
{
    if (!sManagerRegistry.HasBetterQuestingConfig())
    {
        LOG_INFO("module", "BetterQuesting: Quest Guide system disabled (no config), skipping data load");
        return;
    }

    auto& config = sManagerRegistry.GetBetterQuestingConfig();
    if (!config.IsEnabled() || !config.IsQuestGuideEnabled())
    {
        LOG_INFO("module", "BetterQuesting: Quest Guide system disabled, skipping data load");
        return;
    }

    uint32 oldMSTime = getMSTime();

    LoadGuides();
    LoadGuideSteps();
    LoadZoneTransitions();

    _dataLoaded = true;

    LOG_INFO("module", "BetterQuesting: Loaded {} quest guides with {} total steps in {} ms",
        GetGuideCount(), GetTotalStepCount(), GetMSTimeDiffToNow(oldMSTime));
}

void QuestGuideMgr::LoadGuides()
{
    QueryResult result = WorldDatabase.Query(
        "SELECT guideId, name, description, faction, startingRace, minLevel, maxLevel, priority, enabled "
        "FROM playerbots_questing_quest_guides WHERE enabled = 1");

    if (!result)
    {
        LOG_WARN("module", "BetterQuesting: No quest guides found in playerbots_questing_quest_guides");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        QuestGuideData guide;
        guide.guideId = fields[0].Get<uint32>();
        guide.name = fields[1].Get<std::string>();
        guide.description = fields[2].Get<std::string>();
        guide.faction = fields[3].Get<uint8>();
        guide.startingRace = fields[4].Get<uint8>();
        guide.minLevel = fields[5].Get<uint8>();
        guide.maxLevel = fields[6].Get<uint8>();
        guide.priority = fields[7].Get<uint8>();
        guide.enabled = fields[8].Get<uint8>() != 0;

        _guides[guide.guideId] = guide;
    }
    while (result->NextRow());

    LOG_DEBUG("module", "BetterQuesting: Loaded {} quest guide definitions", _guides.size());
}

void QuestGuideMgr::LoadGuideSteps()
{
    QueryResult result = WorldDatabase.Query(
        "SELECT guideId, stepOrder, questId, minLevel, maxLevel, zoneId, isOptional, skipIfCompleted "
        "FROM playerbots_questing_guide_steps ORDER BY guideId, stepOrder");

    if (!result)
    {
        LOG_WARN("module", "BetterQuesting: No guide steps found in playerbots_questing_guide_steps");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        QuestGuideStep step;
        step.guideId = fields[0].Get<uint32>();
        step.stepOrder = fields[1].Get<uint32>();
        step.questId = fields[2].Get<uint32>();
        step.minLevel = fields[3].Get<uint8>();
        step.maxLevel = fields[4].Get<uint8>();
        step.zoneId = fields[5].Get<uint32>();
        step.isOptional = fields[6].Get<uint8>() != 0;
        step.skipIfCompleted = fields[7].Get<uint8>() != 0;

        auto key = std::make_pair(step.guideId, step.stepOrder);
        _guideSteps[key] = step;

        // Build quest-to-step lookup
        _questToStep[step.guideId][step.questId] = step.stepOrder;

        ++_totalStepCount;
    }
    while (result->NextRow());

    LOG_DEBUG("module", "BetterQuesting: Loaded {} guide step entries", _totalStepCount);
}

void QuestGuideMgr::LoadZoneTransitions()
{
    QueryResult result = WorldDatabase.Query(
        "SELECT guideId, fromZoneId, toZoneId, triggerLevel, triggerQuestComplete, priority "
        "FROM playerbots_questing_zone_transitions ORDER BY guideId, priority DESC");

    if (!result)
    {
        LOG_DEBUG("module", "BetterQuesting: No zone transitions found in playerbots_questing_zone_transitions");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        QuestGuideZoneTransition transition;
        transition.guideId = fields[0].Get<uint32>();
        transition.fromZoneId = fields[1].Get<uint32>();
        transition.toZoneId = fields[2].Get<uint32>();
        transition.triggerLevel = fields[3].Get<uint8>();
        transition.triggerQuestComplete = fields[4].Get<uint32>();
        transition.priority = fields[5].Get<uint8>();

        _zoneTransitions.emplace(transition.guideId, transition);
        ++count;
    }
    while (result->NextRow());

    LOG_DEBUG("module", "BetterQuesting: Loaded {} zone transition entries", count);
}

QuestGuideData const* QuestGuideMgr::GetGuide(uint32 guideId) const
{
    auto it = _guides.find(guideId);
    if (it != _guides.end())
        return &it->second;
    return nullptr;
}

QuestGuideData const* QuestGuideMgr::GetBestGuideForPlayer(Player* player) const
{
    if (!player || !_dataLoaded)
        return nullptr;

    uint8 playerFaction = player->GetTeamId() == TEAM_ALLIANCE ? FACTION_ALLIANCE : FACTION_HORDE;
    uint8 playerRace = player->getRace();
    uint8 playerLevel = player->GetLevel();

    QuestGuideData const* bestGuide = nullptr;
    uint8 bestPriority = 0;

    for (const auto& pair : _guides)
    {
        const QuestGuideData& guide = pair.second;

        if (!guide.enabled)
            continue;

        // Check faction match
        if (guide.faction != FACTION_BOTH && guide.faction != playerFaction)
            continue;

        // Check level range
        if (playerLevel < guide.minLevel || playerLevel > guide.maxLevel)
            continue;

        // Check race match (0 means any race for this faction)
        bool raceMatch = (guide.startingRace == 0 || guide.startingRace == playerRace);

        // Calculate effective priority
        uint8 effectivePriority = guide.priority;
        if (raceMatch && guide.startingRace != 0)
            effectivePriority += 50;  // Bonus for exact race match

        if (effectivePriority > bestPriority)
        {
            bestPriority = effectivePriority;
            bestGuide = &guide;
        }
    }

    return bestGuide;
}

std::vector<QuestGuideData const*> QuestGuideMgr::GetGuidesForFaction(uint8 faction) const
{
    std::vector<QuestGuideData const*> result;

    for (const auto& pair : _guides)
    {
        if (pair.second.enabled &&
            (pair.second.faction == faction || pair.second.faction == FACTION_BOTH))
        {
            result.push_back(&pair.second);
        }
    }

    // Sort by priority
    std::sort(result.begin(), result.end(),
        [](QuestGuideData const* a, QuestGuideData const* b) {
            return a->priority > b->priority;
        });

    return result;
}

QuestGuideStep const* QuestGuideMgr::GetCurrentStep(Player* player) const
{
    if (!player)
        return nullptr;

    auto progressIt = _playerProgress.find(player->GetGUID().GetCounter());
    if (progressIt == _playerProgress.end())
        return nullptr;

    const QuestGuideProgress& progress = progressIt->second;
    return GetStepByOrder(progress.guideId, progress.currentStep);
}

QuestGuideStep const* QuestGuideMgr::GetNextStep(Player* player) const
{
    if (!player)
        return nullptr;

    auto progressIt = _playerProgress.find(player->GetGUID().GetCounter());
    if (progressIt == _playerProgress.end())
        return nullptr;

    const QuestGuideProgress& progress = progressIt->second;
    uint32 nextStepOrder = progress.currentStep + 1;

    // Find next available step
    while (true)
    {
        QuestGuideStep const* step = GetStepByOrder(progress.guideId, nextStepOrder);
        if (!step)
            return nullptr;  // No more steps

        if (IsStepAvailable(player, step))
            return step;

        // Only skip optional steps; non-optional block progression
        if (!step->isOptional)
            return nullptr;

        ++nextStepOrder;
    }
}

std::vector<QuestGuideStep const*> QuestGuideMgr::GetAvailableSteps(Player* player, uint32 limit) const
{
    std::vector<QuestGuideStep const*> result;

    if (!player)
        return result;

    auto progressIt = _playerProgress.find(player->GetGUID().GetCounter());
    if (progressIt == _playerProgress.end())
        return result;

    const QuestGuideProgress& progress = progressIt->second;
    uint32 stepOrder = progress.currentStep;

    while (result.size() < limit)
    {
        QuestGuideStep const* step = GetStepByOrder(progress.guideId, stepOrder);
        if (!step)
            break;  // No more steps

        if (IsStepAvailable(player, step))
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

QuestGuideStep const* QuestGuideMgr::GetStepByOrder(uint32 guideId, uint32 stepOrder) const
{
    auto key = std::make_pair(guideId, stepOrder);
    auto it = _guideSteps.find(key);
    if (it != _guideSteps.end())
        return &it->second;
    return nullptr;
}

std::vector<QuestGuideStep const*> QuestGuideMgr::GetGuideSteps(uint32 guideId) const
{
    std::vector<QuestGuideStep const*> result;

    for (const auto& pair : _guideSteps)
    {
        if (pair.first.first == guideId)
            result.push_back(&pair.second);
    }

    return result;
}

bool QuestGuideMgr::ShouldTransitionZone(Player* player, uint32& targetZoneId) const
{
    if (!player)
        return false;

    if (!sManagerRegistry.HasBetterQuestingConfig() ||
        !sManagerRegistry.GetBetterQuestingConfig().IsQuestGuideZoneTransitionEnabled())
        return false;

    auto progressIt = _playerProgress.find(player->GetGUID().GetCounter());
    if (progressIt == _playerProgress.end())
        return false;

    const QuestGuideProgress& progress = progressIt->second;
    uint32 currentZone = player->GetZoneId();
    uint8 playerLevel = player->GetLevel();

    auto range = _zoneTransitions.equal_range(progress.guideId);
    for (auto it = range.first; it != range.second; ++it)
    {
        const QuestGuideZoneTransition& transition = it->second;

        // Check if we're in the right zone
        if (transition.fromZoneId != currentZone)
            continue;

        // Check level trigger
        if (playerLevel < transition.triggerLevel)
            continue;

        // Check quest completion trigger
        if (transition.triggerQuestComplete != 0 &&
            !player->GetQuestRewardStatus(transition.triggerQuestComplete))
            continue;

        targetZoneId = transition.toZoneId;
        return true;
    }

    return false;
}

QuestGuideZoneTransition const* QuestGuideMgr::GetNextTransition(Player* player) const
{
    if (!player)
        return nullptr;

    auto progressIt = _playerProgress.find(player->GetGUID().GetCounter());
    if (progressIt == _playerProgress.end())
        return nullptr;

    const QuestGuideProgress& progress = progressIt->second;
    uint32 currentZone = player->GetZoneId();

    auto range = _zoneTransitions.equal_range(progress.guideId);
    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second.fromZoneId == currentZone)
            return &it->second;
    }

    return nullptr;
}

int32 QuestGuideMgr::GetGuideQuestPriorityBonus(Player* player, uint32 questId) const
{
    if (!player || !_dataLoaded)
        return 0;

    if (!sManagerRegistry.HasBetterQuestingConfig())
        return 0;

    auto progressIt = _playerProgress.find(player->GetGUID().GetCounter());
    if (progressIt == _playerProgress.end())
        return 0;

    const QuestGuideProgress& progress = progressIt->second;
    int32 baseBonus = sManagerRegistry.GetBetterQuestingConfig().GetGuidePriorityBonus();

    // Check if quest is in the active guide
    auto guideQuestsIt = _questToStep.find(progress.guideId);
    if (guideQuestsIt == _questToStep.end())
        return 0;

    auto questIt = guideQuestsIt->second.find(questId);
    if (questIt == guideQuestsIt->second.end())
        return 0;

    uint32 questStepOrder = questIt->second;

    // Calculate bonus based on position relative to current step
    int32 stepDiff = static_cast<int32>(questStepOrder) - static_cast<int32>(progress.currentStep);

    if (stepDiff < 0)
    {
        // Quest is before current step (might be skipped or already done)
        return 0;
    }
    else if (stepDiff == 0)
    {
        // Current step - highest bonus
        return baseBonus + 25;
    }
    else if (stepDiff <= 3)
    {
        // Next few steps - diminishing bonus
        return baseBonus + (25 - (stepDiff * 5));  // +20, +15, +10
    }
    else
    {
        // Further steps - base bonus
        return baseBonus;
    }
}

bool QuestGuideMgr::IsQuestInGuide(uint32 guideId, uint32 questId) const
{
    auto guideQuestsIt = _questToStep.find(guideId);
    if (guideQuestsIt == _questToStep.end())
        return false;

    return guideQuestsIt->second.find(questId) != guideQuestsIt->second.end();
}

bool QuestGuideMgr::IsQuestInActiveGuide(Player* player, uint32 questId) const
{
    if (!player)
        return false;

    auto progressIt = _playerProgress.find(player->GetGUID().GetCounter());
    if (progressIt == _playerProgress.end())
        return false;

    return IsQuestInGuide(progressIt->second.guideId, questId);
}

QuestGuideProgress const* QuestGuideMgr::GetPlayerProgress(uint32 guid) const
{
    auto it = _playerProgress.find(guid);
    if (it != _playerProgress.end())
        return &it->second;
    return nullptr;
}

void QuestGuideMgr::SetPlayerGuide(Player* player, uint32 guideId)
{
    if (!player)
        return;

    uint32 guid = player->GetGUID().GetCounter();
    QuestGuideProgress& progress = _playerProgress[guid];
    progress.guid = guid;
    progress.guideId = guideId;
    progress.currentStep = 0;

    SavePlayerProgress(player);

    LOG_DEBUG("module", "BetterQuesting: Set player {} to guide {}", player->GetName(), guideId);
}

void QuestGuideMgr::AdvancePlayerStep(Player* player)
{
    if (!player)
        return;

    uint32 guid = player->GetGUID().GetCounter();
    auto it = _playerProgress.find(guid);
    if (it == _playerProgress.end())
        return;

    ++it->second.currentStep;
    SavePlayerProgress(player);

    LOG_DEBUG("module", "BetterQuesting: Advanced player {} to step {}",
        player->GetName(), it->second.currentStep);
}

void QuestGuideMgr::SyncPlayerProgress(Player* player)
{
    if (!player)
        return;

    if (!sManagerRegistry.HasBetterQuestingConfig())
        return;

    uint32 guid = player->GetGUID().GetCounter();
    auto it = _playerProgress.find(guid);
    if (it == _playerProgress.end())
        return;

    QuestGuideProgress& progress = it->second;

    // Find the first step that the player hasn't completed
    uint32 stepOrder = 0;
    while (true)
    {
        QuestGuideStep const* step = GetStepByOrder(progress.guideId, stepOrder);
        if (!step)
            break;  // End of guide

        // Check if player has completed this quest
        if (!player->GetQuestRewardStatus(step->questId))
        {
            // Only skip optional unavailable steps
            if (step->isOptional && !IsStepAvailable(player, step))
            {
                if (sManagerRegistry.GetBetterQuestingConfig().IsQuestGuideSkipUnavailableEnabled())
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

    if (progress.currentStep != stepOrder)
    {
        progress.currentStep = stepOrder;
        SavePlayerProgress(player);
        LOG_DEBUG("module", "BetterQuesting: Synced player {} progress to step {}",
            player->GetName(), stepOrder);
    }
}

void QuestGuideMgr::SavePlayerProgress(Player* player)
{
    if (!player)
        return;

    uint32 guid = player->GetGUID().GetCounter();
    auto it = _playerProgress.find(guid);
    if (it == _playerProgress.end())
        return;

    const QuestGuideProgress& progress = it->second;

    CharacterDatabase.Execute(
        "REPLACE INTO playerbots_questing_player_guide_progress (guid, guideId, currentStep, activeQuestId, activeSubStatus) VALUES ({}, {}, {}, {}, {})",
        progress.guid, progress.guideId, progress.currentStep, progress.activeQuestId, progress.activeSubStatus);
}

void QuestGuideMgr::LoadPlayerProgress(uint32 guid)
{
    QueryResult result = CharacterDatabase.Query(
        "SELECT guideId, currentStep, activeQuestId, activeSubStatus FROM playerbots_questing_player_guide_progress WHERE guid = {}",
        guid);

    if (!result)
        return;

    Field* fields = result->Fetch();
    QuestGuideProgress progress;
    progress.guid = guid;
    progress.guideId = fields[0].Get<uint32>();
    progress.currentStep = fields[1].Get<uint32>();
    progress.activeQuestId = fields[2].Get<uint32>();
    progress.activeSubStatus = fields[3].Get<uint8>();

    _playerProgress[guid] = progress;

    LOG_DEBUG("module", "BetterQuesting: Loaded progress for guid {}: guide {}, step {}, activeQuest {}, subStatus {}",
        guid, progress.guideId, progress.currentStep, progress.activeQuestId, progress.activeSubStatus);
}

void QuestGuideMgr::DeletePlayerProgress(uint32 guid)
{
    _playerProgress.erase(guid);
    CharacterDatabase.Execute(
        "DELETE FROM playerbots_questing_player_guide_progress WHERE guid = {}", guid);
}

void QuestGuideMgr::SetActiveQuestState(Player* player, uint32 questId, uint8 subStatus)
{
    if (!player)
        return;

    uint32 guid = player->GetGUID().GetCounter();
    auto it = _playerProgress.find(guid);
    if (it == _playerProgress.end())
    {
        // Create progress entry if it doesn't exist
        QuestGuideProgress progress;
        progress.guid = guid;
        progress.guideId = 0;
        progress.currentStep = 0;
        progress.activeQuestId = questId;
        progress.activeSubStatus = subStatus;
        _playerProgress[guid] = progress;
    }
    else
    {
        it->second.activeQuestId = questId;
        it->second.activeSubStatus = subStatus;
    }

    SavePlayerProgress(player);

    LOG_DEBUG("module", "BetterQuesting: Saved active quest state for {}: quest {}, subStatus {}",
        player->GetName(), questId, subStatus);
}

void QuestGuideMgr::ClearActiveQuestState(Player* player)
{
    if (!player)
        return;

    uint32 guid = player->GetGUID().GetCounter();
    auto it = _playerProgress.find(guid);
    if (it == _playerProgress.end())
        return;

    it->second.activeQuestId = 0;
    it->second.activeSubStatus = 0;

    SavePlayerProgress(player);

    LOG_DEBUG("module", "BetterQuesting: Cleared active quest state for {}", player->GetName());
}

uint32 QuestGuideMgr::FindStepForQuest(uint32 guideId, uint32 questId) const
{
    auto guideQuestsIt = _questToStep.find(guideId);
    if (guideQuestsIt == _questToStep.end())
        return 0;

    auto questIt = guideQuestsIt->second.find(questId);
    if (questIt == guideQuestsIt->second.end())
        return 0;

    return questIt->second;
}

bool QuestGuideMgr::IsStepAvailable(Player* player, QuestGuideStep const* step) const
{
    if (!player || !step)
        return false;

    uint8 playerLevel = player->GetLevel();

    // Check level range
    if (playerLevel < step->minLevel || playerLevel > step->maxLevel)
        return false;

    // Check if already completed (and skipIfCompleted is set)
    if (step->skipIfCompleted && player->GetQuestRewardStatus(step->questId))
        return false;

    // Check if player already has the quest
    QuestStatus status = player->GetQuestStatus(step->questId);
    if (status == QUEST_STATUS_COMPLETE || status == QUEST_STATUS_INCOMPLETE)
        return true;  // Already in progress

    // For not-yet-accepted quests, check if available
    if (status == QUEST_STATUS_NONE)
    {
        // Could add more checks here (prerequisites, etc.)
        return true;
    }

    return false;
}
