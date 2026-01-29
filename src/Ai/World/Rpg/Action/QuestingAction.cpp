/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "QuestingAction.h"

#include "Bot/Core/ManagerRegistry.h"
#include "BotChatService.h"
#include "BotSpellService.h"
#include "ChatHelper.h"
#include "DBCStores.h"
#include "IVMapMgr.h"
#include "LootMgr.h"
#include "LootObjectStack.h"
#include "Mgr/Quest/BetterQuestingTypes.h"
#include "NewRpgInfo.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "QuestDef.h"
#include "Random.h"

void QuestingUpdateAction::WhisperStatus(std::string const& msg)
{
    // Avoid spamming the same message
    if (msg == lastWhisperedStatus)
        return;

    lastWhisperedStatus = msg;

    // Use TellMaster which handles both regular bots and selfbots
    botAI->GetServices().GetChatService().TellMasterNoFacing(msg);
}

void QuestingUpdateAction::SetQuestingState(uint32 questId, Quest const* quest, QuestingSubStatus subStatus)
{
    botAI->rpgInfo.ChangeToQuesting(questId, quest, subStatus);

    // Save the state for persistence across logout/login
    // Don't persist SEARCHING or IDLE states as they're transient
    if (questId > 0 && subStatus != QUESTING_SEARCHING && subStatus != QUESTING_IDLE)
    {
        if (sManagerRegistry.HasQuestGuideMgr())
        {
            sManagerRegistry.GetQuestGuideMgr().SetActiveQuestState(bot, questId, static_cast<uint8>(subStatus));
        }
    }
}

bool QuestingUpdateAction::TryRestoreSavedQuestState()
{
    // Only try to restore once per session
    if (hasAttemptedRestore)
        return false;

    hasAttemptedRestore = true;

    if (!sManagerRegistry.HasQuestGuideMgr())
        return false;

    auto& guideMgr = sManagerRegistry.GetQuestGuideMgr();
    auto* progress = guideMgr.GetPlayerProgress(bot->GetGUID().GetCounter());

    if (!progress || progress->activeQuestId == 0)
        return false;

    uint32 questId = progress->activeQuestId;
    QuestingSubStatus savedSubStatus = static_cast<QuestingSubStatus>(progress->activeSubStatus);

    // Validate quest is still in player's log
    QuestStatus questStatus = bot->GetQuestStatus(questId);

    if (questStatus == QUEST_STATUS_NONE)
    {
        // Quest was abandoned or never accepted - clear saved state
        LOG_DEBUG("module", "BetterQuesting: Quest {} no longer in log for {}, clearing saved state",
            questId, bot->GetName());
        guideMgr.ClearActiveQuestState(bot);
        return false;
    }

    // Get quest template
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
    {
        LOG_DEBUG("module", "BetterQuesting: Quest template {} not found for {}, clearing saved state",
            questId, bot->GetName());
        guideMgr.ClearActiveQuestState(bot);
        return false;
    }

    // Adjust substatus based on current quest state
    QuestingSubStatus restoredSubStatus = savedSubStatus;

    if (questStatus == QUEST_STATUS_COMPLETE)
    {
        // Quest is complete, should be turning in
        if (restoredSubStatus == QUESTING_TRAVELING_TO_ACCEPT ||
            restoredSubStatus == QUESTING_TRAVELING_TO_OBJECTIVE)
        {
            restoredSubStatus = QUESTING_TRAVELING_TO_TURNIN;
        }
    }
    else if (questStatus == QUEST_STATUS_INCOMPLETE)
    {
        // Quest is incomplete (in progress)
        if (restoredSubStatus == QUESTING_TRAVELING_TO_ACCEPT)
        {
            // Quest was accepted - should be working on objective
            restoredSubStatus = QUESTING_TRAVELING_TO_OBJECTIVE;
        }
        else if (restoredSubStatus == QUESTING_TRAVELING_TO_TURNIN)
        {
            // Quest objectives no longer complete - go back to objective
            restoredSubStatus = QUESTING_TRAVELING_TO_OBJECTIVE;
        }
    }

    // Restore the questing state
    std::string questLink = ChatHelper::FormatQuest(quest);
    std::string statusStr;

    switch (restoredSubStatus)
    {
        case QUESTING_TRAVELING_TO_ACCEPT:
            statusStr = "Resuming: Traveling to accept " + questLink;
            break;
        case QUESTING_TRAVELING_TO_OBJECTIVE:
            statusStr = "Resuming: Working on objective for " + questLink;
            break;
        case QUESTING_TRAVELING_TO_TURNIN:
            statusStr = "Resuming: Traveling to turn in " + questLink;
            break;
        case QUESTING_SEARCHING:
            // Don't restore searching state, let normal logic handle it
            guideMgr.ClearActiveQuestState(bot);
            return false;
        case QUESTING_IDLE:
        default:
            guideMgr.ClearActiveQuestState(bot);
            return false;
    }

    WhisperStatus(statusStr);
    botAI->rpgInfo.ChangeToQuesting(questId, quest, restoredSubStatus);

    LOG_INFO("module", "BetterQuesting: Restored questing state for {}: quest {} ({}), subStatus {}",
        bot->GetName(), questId, quest->GetTitle(), static_cast<uint8>(restoredSubStatus));

    // Update the saved state if we adjusted the substatus
    if (restoredSubStatus != savedSubStatus)
    {
        guideMgr.SetActiveQuestState(bot, questId, static_cast<uint8>(restoredSubStatus));
    }

    return true;
}

bool QuestingUpdateAction::TryAcceptQuestFromGuide()
{
    if (!sManagerRegistry.HasQuestGuideMgr())
        return false;

    auto& guideMgr = sManagerRegistry.GetQuestGuideMgr();
    auto* progress = guideMgr.GetPlayerProgress(bot->GetGUID().GetCounter());

    if (!progress || !progress->guideId)
        return false;

    // Find the lowest step order quest that needs accepting
    uint32 bestQuestId = 0;
    Quest const* bestQuest = nullptr;

    // Scan guide steps in order to find first quest we need to accept
    for (uint32 stepOrder = 1; stepOrder <= 100; ++stepOrder)  // Reasonable limit
    {
        QuestGuideStep const* step = guideMgr.GetStepByOrder(progress->guideId, stepOrder);
        if (!step)
            break;  // End of guide

        if (!step->questId)
            continue;

        QuestStatus status = bot->GetQuestStatus(step->questId);

        // If we already have this quest (incomplete or complete), skip to next
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE)
            continue;

        // If quest is rewarded, skip to next
        if (bot->GetQuestRewardStatus(step->questId))
            continue;

        // Quest needs to be accepted - check if we can take it
        Quest const* quest = sObjectMgr->GetQuestTemplate(step->questId);
        if (!quest)
            continue;

        // Skip optional quests we can't take
        if (step->isOptional && !bot->CanTakeQuest(quest, false))
            continue;

        // Non-optional quest we can't take blocks progression
        if (!step->isOptional && !bot->CanTakeQuest(quest, false))
            return false;  // Blocked - can't progress

        // Found a quest to accept
        bestQuestId = step->questId;
        bestQuest = quest;
        break;
    }

    if (!bestQuestId || !bestQuest)
        return false;

    // Set status to travel to accept
    std::string questLink = ChatHelper::FormatQuest(bestQuest);
    WhisperStatus("Traveling to accept: " + questLink);

    SetQuestingState(bestQuestId, bestQuest, QUESTING_TRAVELING_TO_ACCEPT);
    return true;
}

bool QuestingUpdateAction::TryFindNearbyQuestToAccept()
{
    // Check if guide is active
    uint32 guideId = 0;
    if (sManagerRegistry.HasQuestGuideMgr())
    {
        auto& guideMgr = sManagerRegistry.GetQuestGuideMgr();
        auto* progress = guideMgr.GetPlayerProgress(bot->GetGUID().GetCounter());
        if (progress && progress->guideId)
        {
            guideId = progress->guideId;
        }
    }

    // Look for nearby NPCs with available quests
    GuidVector possibleTargets = AI_VALUE(GuidVector, "possible new rpg targets");
    GuidVector possibleGameObjects = AI_VALUE(GuidVector, "possible new rpg game objects");

    // Track best quest to accept (lowest step order if guide active)
    uint32 bestQuestId = 0;
    uint32 bestStepOrder = UINT32_MAX;
    Quest const* bestQuest = nullptr;

    auto checkQuest = [&](uint32 questId, Quest const* quest) {
        if (!quest)
            return;

        QuestStatus const& status = bot->GetQuestStatus(questId);
        if (status != QUEST_STATUS_NONE || !bot->CanTakeQuest(quest, false) || !bot->CanAddQuest(quest, false))
            return;

        if (!IsQuestWorthDoing(quest) || !IsQuestCapableDoing(quest))
            return;

        if (guideId)
        {
            // Guide active - only accept guide quests, prefer lowest step order
            uint32 stepOrder = sManagerRegistry.GetQuestGuideMgr().FindStepForQuest(guideId, questId);
            if (stepOrder > 0 && stepOrder < bestStepOrder)
            {
                bestQuestId = questId;
                bestStepOrder = stepOrder;
                bestQuest = quest;
            }
        }
        else if (!bestQuestId)
        {
            // No guide - accept first valid quest
            bestQuestId = questId;
            bestQuest = quest;
        }
    };

    // Check NPCs
    for (ObjectGuid& guid : possibleTargets)
    {
        WorldObject* object = ObjectAccessor::GetWorldObject(*bot, guid);
        if (!object || !object->IsInWorld())
            continue;

        if (!CanInteractWithQuestGiver(object))
            continue;

        if (!HasQuestToAcceptOrReward(object))
            continue;

        bot->PrepareQuestMenu(guid);
        QuestMenu const& menu = bot->PlayerTalkClass->GetQuestMenu();

        for (uint8 idx = 0; idx < menu.GetMenuItemCount(); idx++)
        {
            QuestMenuItem const& item = menu.GetItem(idx);
            Quest const* quest = sObjectMgr->GetQuestTemplate(item.QuestId);
            checkQuest(item.QuestId, quest);
        }
    }

    // Check GameObjects
    for (ObjectGuid& guid : possibleGameObjects)
    {
        WorldObject* object = ObjectAccessor::GetWorldObject(*bot, guid);
        if (!object || !object->IsInWorld())
            continue;

        if (!CanInteractWithQuestGiver(object))
            continue;

        if (!HasQuestToAcceptOrReward(object))
            continue;

        bot->PrepareQuestMenu(guid);
        QuestMenu const& menu = bot->PlayerTalkClass->GetQuestMenu();

        for (uint8 idx = 0; idx < menu.GetMenuItemCount(); idx++)
        {
            QuestMenuItem const& item = menu.GetItem(idx);
            Quest const* quest = sObjectMgr->GetQuestTemplate(item.QuestId);
            checkQuest(item.QuestId, quest);
        }
    }

    if (bestQuestId && bestQuest)
    {
        std::string questLink = ChatHelper::FormatQuest(bestQuest);
        WhisperStatus("Found nearby quest: " + questLink);

        SetQuestingState(bestQuestId, bestQuest, QUESTING_TRAVELING_TO_ACCEPT);
        return true;
    }

    return false;
}

bool QuestingUpdateAction::TryDoIncompleteQuest()
{
    // When guide is active, find the incomplete quest with the lowest step order
    // But first check if there's a COMPLETE quest with even lower step order (needs turn-in first)
    if (sManagerRegistry.HasQuestGuideMgr())
    {
        auto& guideMgr = sManagerRegistry.GetQuestGuideMgr();
        auto* progress = guideMgr.GetPlayerProgress(bot->GetGUID().GetCounter());

        if (progress && progress->guideId)
        {
            uint32 bestIncompleteQuestId = 0;
            uint32 bestIncompleteStepOrder = UINT32_MAX;
            Quest const* bestIncompleteQuest = nullptr;
            uint32 lowestCompleteStepOrder = UINT32_MAX;

            // Scan quest log for guide quests
            for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
            {
                uint32 questId = bot->GetQuestSlotQuestId(slot);
                if (!questId)
                    continue;

                // Check if this quest is in the active guide
                uint32 stepOrder = guideMgr.FindStepForQuest(progress->guideId, questId);
                if (stepOrder == 0)
                    continue;

                QuestStatus status = bot->GetQuestStatus(questId);
                if (status == QUEST_STATUS_INCOMPLETE)
                {
                    if (stepOrder < bestIncompleteStepOrder)
                    {
                        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                        if (quest && IsQuestCapableDoing(quest))
                        {
                            bestIncompleteQuestId = questId;
                            bestIncompleteStepOrder = stepOrder;
                            bestIncompleteQuest = quest;
                        }
                    }
                }
                else if (status == QUEST_STATUS_COMPLETE)
                {
                    // Track lowest complete quest that needs turn-in
                    if (stepOrder < lowestCompleteStepOrder)
                    {
                        lowestCompleteStepOrder = stepOrder;
                    }
                }
            }

            // If there's a complete quest with lower step order, turn that in first
            if (lowestCompleteStepOrder < bestIncompleteStepOrder)
            {
                // Let TryTurnInCompletedQuest handle it
                return false;
            }

            // Check if there's a lower step quest that needs to be accepted first
            // (not in log yet, but should be done before our best incomplete)
            if (bestIncompleteStepOrder < UINT32_MAX)
            {
                for (uint32 checkStep = 1; checkStep < bestIncompleteStepOrder; ++checkStep)
                {
                    QuestGuideStep const* step = guideMgr.GetStepByOrder(progress->guideId, checkStep);
                    if (!step || !step->questId)
                        continue;

                    // Skip optional steps
                    if (step->isOptional)
                        continue;

                    QuestStatus status = bot->GetQuestStatus(step->questId);
                    if (status == QUEST_STATUS_NONE)
                    {
                        // There's an earlier quest we need to accept first
                        // Let TryAcceptQuestFromGuide handle it
                        return false;
                    }
                }
            }

            // Work on the incomplete quest with lowest step order
            if (bestIncompleteQuestId && bestIncompleteQuest)
            {
                std::string questLink = ChatHelper::FormatQuest(bestIncompleteQuest);
                WhisperStatus("Working on objective for: " + questLink);

                SetQuestingState(bestIncompleteQuestId, bestIncompleteQuest, QUESTING_TRAVELING_TO_OBJECTIVE);
                return true;
            }

            // No guide quests in log - let accept logic handle it
            return false;
        }
    }

    // Fall back to any incomplete quest in log (only when guide is not active)
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        QuestStatus status = bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_INCOMPLETE)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (quest && IsQuestCapableDoing(quest))
            {
                std::string questLink = ChatHelper::FormatQuest(quest);
                WhisperStatus("Working on objective for: " + questLink);

                SetQuestingState(questId, quest, QUESTING_TRAVELING_TO_OBJECTIVE);
                return true;
            }
        }
    }

    return false;
}

bool QuestingUpdateAction::TryTurnInCompletedQuest()
{
    // When guide is active, find the completed quest with the lowest step order
    if (sManagerRegistry.HasQuestGuideMgr())
    {
        auto& guideMgr = sManagerRegistry.GetQuestGuideMgr();
        auto* progress = guideMgr.GetPlayerProgress(bot->GetGUID().GetCounter());

        if (progress && progress->guideId)
        {
            uint32 bestQuestId = 0;
            uint32 bestStepOrder = UINT32_MAX;
            Quest const* bestQuest = nullptr;

            // Scan quest log for completed quests that are in the guide
            for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
            {
                uint32 questId = bot->GetQuestSlotQuestId(slot);
                if (!questId)
                    continue;

                QuestStatus status = bot->GetQuestStatus(questId);
                if (status != QUEST_STATUS_COMPLETE)
                    continue;

                // Check if this quest is in the active guide
                uint32 stepOrder = guideMgr.FindStepForQuest(progress->guideId, questId);
                if (stepOrder > 0 && stepOrder < bestStepOrder)
                {
                    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
                    if (quest)
                    {
                        bestQuestId = questId;
                        bestStepOrder = stepOrder;
                        bestQuest = quest;
                    }
                }
            }

            // Turn in the quest with lowest step order
            if (bestQuestId && bestQuest)
            {
                std::string questLink = ChatHelper::FormatQuest(bestQuest);
                WhisperStatus("Traveling to turn in: " + questLink);

                SetQuestingState(bestQuestId, bestQuest, QUESTING_TRAVELING_TO_TURNIN);
                return true;
            }
        }
    }

    // Fall back to any completed quest in log (only when guide is not active)
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        QuestStatus status = bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_COMPLETE)
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (quest)
            {
                std::string questLink = ChatHelper::FormatQuest(quest);
                WhisperStatus("Traveling to turn in: " + questLink);

                SetQuestingState(questId, quest, QUESTING_TRAVELING_TO_TURNIN);
                return true;
            }
        }
    }

    return false;
}

bool QuestingUpdateAction::Execute(Event /*event*/)
{
    NewRpgInfo& info = botAI->rpgInfo;

    // If we're already in QUESTING status, check if we need to transition
    if (info.status == RPG_QUESTING)
    {
        uint32 questId = info.questing.questId;
        QuestStatus status = bot->GetQuestStatus(questId);

        switch (info.questing.subStatus)
        {
            case QUESTING_TRAVELING_TO_ACCEPT:
                // Check if we now have the quest
                if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE)
                {
                    // Quest accepted, move to objective or turn in
                    if (status == QUEST_STATUS_COMPLETE)
                        return TryTurnInCompletedQuest();
                    else
                        return TryDoIncompleteQuest();
                }
                break;

            case QUESTING_TRAVELING_TO_OBJECTIVE:
                // Check if quest is now complete
                if (status == QUEST_STATUS_COMPLETE)
                {
                    return TryTurnInCompletedQuest();
                }
                // Check if quest was abandoned or otherwise unavailable
                else if (status == QUEST_STATUS_NONE)
                {
                    info.ChangeToIdle();
                    return true;
                }
                break;

            case QUESTING_TRAVELING_TO_TURNIN:
                // Check if quest was turned in (status becomes NONE or REWARDED)
                if (status == QUEST_STATUS_NONE || status == QUEST_STATUS_REWARDED)
                {
                    // Advance guide progress if using quest guide
                    if (sManagerRegistry.HasQuestGuideMgr())
                    {
                        sManagerRegistry.GetQuestGuideMgr().AdvancePlayerStep(bot);
                    }

                    // Look for next quest
                    info.ChangeToIdle();
                    // Fall through to find next quest below
                }
                break;

            case QUESTING_SEARCHING:
                // Continue searching - fall through to find quest logic below
                break;

            case QUESTING_IDLE:
            default:
                break;
        }
    }

    // If idle, searching, or need new quest, find one
    if (info.status != RPG_QUESTING || info.questing.subStatus == QUESTING_IDLE || info.questing.subStatus == QUESTING_SEARCHING)
    {
        // Priority 0: Try to restore saved quest state from previous session
        if (TryRestoreSavedQuestState())
            return true;

        // Priority 1: Turn in completed quests
        if (TryTurnInCompletedQuest())
            return true;

        // Priority 2: Accept new quests from guide
        if (TryAcceptQuestFromGuide())
            return true;

        // Priority 3: Work on incomplete quests
        if (TryDoIncompleteQuest())
            return true;

        // Priority 4: Find nearby NPCs with available quests (fallback for fresh characters)
        if (TryFindNearbyQuestToAccept())
            return true;

        // No quests available - enter searching mode to wander and find quest givers
        WhisperStatus("Searching for quests...");
        info.ChangeToQuesting(0, nullptr, QUESTING_SEARCHING);
        return true;
    }

    return false;
}

// QuestingExecuteAction implementation

bool QuestingExecuteAction::DoTravelToAccept()
{
    uint32 questId = botAI->rpgInfo.questing.questId;

    // Try to interact with nearby quest givers
    if (SearchQuestGiverAndAcceptOrReward())
        return true;

    // Get POI for quest acceptance
    std::vector<POIInfo> poiInfo;
    if (!GetQuestPOIPosAndObjectiveIdx(questId, poiInfo, false))
    {
        // Can't find where to accept, go idle
        botAI->rpgInfo.ChangeToIdle();
        return true;
    }

    if (poiInfo.empty())
    {
        botAI->rpgInfo.ChangeToIdle();
        return true;
    }

    // Move to quest giver
    G3D::Vector2 poi = poiInfo[0].pos;
    float dz = std::max(bot->GetMap()->GetHeight(poi.x, poi.y, MAX_HEIGHT), bot->GetMap()->GetWaterLevel(poi.x, poi.y));

    if (dz == INVALID_HEIGHT || dz == VMAP_INVALID_HEIGHT_VALUE)
    {
        botAI->rpgInfo.ChangeToIdle();
        return true;
    }

    WorldPosition targetPos(bot->GetMapId(), poi.x, poi.y, dz);
    botAI->rpgInfo.questing.targetPos = targetPos;

    return MoveFarTo(targetPos);
}

bool QuestingExecuteAction::DoTravelToObjective()
{
    uint32 questId = botAI->rpgInfo.questing.questId;

    // Try to interact with nearby quest givers for turn-in if quest is complete
    if (SearchQuestGiverAndAcceptOrReward())
        return true;

    // Check if we need to find/update POI
    if (botAI->rpgInfo.questing.targetPos == WorldPosition())
    {
        std::vector<POIInfo> poiInfo;
        if (!GetQuestPOIPosAndObjectiveIdx(questId, poiInfo, false))
        {
            // Can't find objective location, go idle
            botAI->rpgInfo.ChangeToIdle();
            return true;
        }

        if (poiInfo.empty())
        {
            botAI->rpgInfo.ChangeToIdle();
            return true;
        }

        // Pick a random POI from available ones
        uint32 rndIdx = urand(0, poiInfo.size() - 1);
        G3D::Vector2 poi = poiInfo[rndIdx].pos;
        float dz = std::max(bot->GetMap()->GetHeight(poi.x, poi.y, MAX_HEIGHT), bot->GetMap()->GetWaterLevel(poi.x, poi.y));

        if (dz == INVALID_HEIGHT || dz == VMAP_INVALID_HEIGHT_VALUE)
        {
            botAI->rpgInfo.ChangeToIdle();
            return true;
        }

        botAI->rpgInfo.questing.targetPos = WorldPosition(bot->GetMapId(), poi.x, poi.y, dz);
    }

    WorldPosition& targetPos = botAI->rpgInfo.questing.targetPos;

    // Check for approach waypoints (for caves, etc.)
    if (sManagerRegistry.HasCoreQuestDataMgr())
    {
        auto& questDataMgr = sManagerRegistry.GetCoreQuestDataMgr();
        bool logFallback = sManagerRegistry.HasBetterQuestingConfig() &&
                          sManagerRegistry.GetBetterQuestingConfig().IsLogFallbackEnabled();

        // Try quest-specific waypoints first, then fall back to zone-based
        std::vector<ApproachWaypoint> waypoints = questDataMgr.GetQuestApproachWaypoints(questId, bot->GetMapId());

        if (waypoints.empty())
        {
            // Fall back to zone-based waypoints
            uint32 zoneId = bot->GetZoneId();
            waypoints = questDataMgr.GetZoneApproachWaypoints(zoneId, bot->GetMapId());

            // If still empty, try area ID (may differ from zone ID in sub-areas)
            if (waypoints.empty())
            {
                uint32 areaId = bot->GetAreaId();
                if (areaId != zoneId)
                {
                    waypoints = questDataMgr.GetZoneApproachWaypoints(areaId, bot->GetMapId());
                    if (logFallback && !waypoints.empty())
                    {
                        LOG_INFO("playerbots", "BetterQuesting: Found {} waypoints using area ID {} instead of zone ID {} for quest {}",
                            waypoints.size(), areaId, zoneId, questId);
                    }
                }
            }

            // If still empty, try parent zone (for sub-areas like Northshire Valley which is zone 9 inside Elwynn Forest zone 12)
            if (waypoints.empty())
            {
                AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(zoneId);
                if (areaEntry && areaEntry->zone != 0)
                {
                    // This zone has a parent zone - try that
                    uint32 parentZoneId = areaEntry->zone;
                    waypoints = questDataMgr.GetZoneApproachWaypoints(parentZoneId, bot->GetMapId());
                    if (logFallback && !waypoints.empty())
                    {
                        LOG_INFO("playerbots", "BetterQuesting: Found {} waypoints using parent zone ID {} for quest {} (current zone {})",
                            waypoints.size(), parentZoneId, questId, zoneId);
                    }
                }
            }

            if (logFallback && waypoints.empty())
            {
                LOG_DEBUG("playerbots", "BetterQuesting: No approach waypoints found for quest {} in zone {} (area {})",
                    questId, bot->GetZoneId(), bot->GetAreaId());
            }
        }
        else if (logFallback)
        {
            LOG_DEBUG("playerbots", "BetterQuesting: Found {} quest-specific waypoints for quest {}",
                waypoints.size(), questId);
        }

        if (!waypoints.empty())
        {
            // Find the waypoint closest to our target using 2D distance
            // (Z comparison doesn't work well for caves where target Z is surface height)
            float bestWpToTarget2D = FLT_MAX;
            const ApproachWaypoint* bestWaypoint = nullptr;

            for (const ApproachWaypoint& wp : waypoints)
            {
                float wpToTarget2D = std::sqrt(
                    std::pow(wp.x - targetPos.GetPositionX(), 2) +
                    std::pow(wp.y - targetPos.GetPositionY(), 2)
                );

                // Only consider waypoints within reasonable 2D range of target (400 yards)
                // This is typically a cave entrance near the quest objective
                // Increased from 300 to handle larger cave systems
                if (wpToTarget2D < bestWpToTarget2D && wpToTarget2D < 400.0f)
                {
                    bestWpToTarget2D = wpToTarget2D;
                    bestWaypoint = &wp;
                }
            }

            if (bestWaypoint)
            {
                float distToWaypoint2D = bot->GetDistance2d(bestWaypoint->x, bestWaypoint->y);
                float distToTarget2D = bot->GetDistance2d(targetPos.GetPositionX(), targetPos.GetPositionY());
                float zDiff = std::abs(bot->GetPositionZ() - bestWaypoint->z);

                // Check if we've truly reached the waypoint - must be close in both 2D AND Z
                // This prevents the bot from thinking it "reached" a cave entrance when it's
                // actually on top of the hill above the cave (close in 2D but far in Z)
                bool reachedWaypoint = (distToWaypoint2D <= bestWaypoint->radius) && (zDiff < 15.0f);

                // Use approach waypoint if we haven't reached it yet and it's roughly
                // between us and the target (using 2D distance to handle cave Z issues)
                if (!reachedWaypoint && bestWpToTarget2D < distToTarget2D + 50.0f)
                {
                    if (logFallback)
                    {
                        LOG_DEBUG("playerbots", "BetterQuesting: {} using approach waypoint at ({}, {}, {}) for quest {} - dist2D: {}, zDiff: {}",
                            bot->GetName(), bestWaypoint->x, bestWaypoint->y, bestWaypoint->z, questId, distToWaypoint2D, zDiff);
                    }
                    WorldPosition wpPos(bot->GetMapId(), bestWaypoint->x, bestWaypoint->y, bestWaypoint->z);
                    return MoveFarTo(wpPos);
                }
            }
        }
    }

    // If we're near the objective, look for quest objects to loot directly
    if (bot->GetDistance(targetPos) < 30.0f)
    {
        uint32 questId = botAI->rpgInfo.questing.questId;
        Quest const* qInfo = sObjectMgr->GetQuestTemplate(questId);

        if (!qInfo)
            return MoveRandomNear(20.0f);

        // Look for quest objects nearby that we can loot - find the CLOSEST one
        GuidVector gos = AI_VALUE(GuidVector, "nearest game objects no los");

        GameObject* closestGo = nullptr;
        ObjectGuid closestGuid;
        float closestDist = 1000.0f;

        for (ObjectGuid& guid : gos)
        {
            GameObject* go = botAI->GetGameObject(guid);
            if (!go || !go->isSpawned())
                continue;

            // Check if this object has quest items we need
            GameObjectQuestItemList const* items = sObjectMgr->GetGameObjectQuestItemList(go->GetEntry());
            if (!items || items->empty())
                continue;

            bool needsThisObject = false;
            for (size_t i = 0; i < items->size() && i < MAX_GAMEOBJECT_QUEST_ITEMS; i++)
            {
                uint32 itemId = (*items)[i];
                if (!itemId)
                    continue;

                for (int j = 0; j < QUEST_ITEM_OBJECTIVES_COUNT; ++j)
                {
                    if (qInfo->RequiredItemId[j] == itemId)
                    {
                        auto& questMap = bot->getQuestStatusMap();
                        auto it = questMap.find(questId);
                        if (it != questMap.end() && qInfo->RequiredItemCount[j] > it->second.ItemCount[j])
                        {
                            needsThisObject = true;
                            break;
                        }
                    }
                }
                if (needsThisObject)
                    break;
            }

            if (!needsThisObject)
                continue;

            // Check if this is closer than our current best
            float dist = bot->GetDistance(go);
            if (dist < closestDist)
            {
                closestDist = dist;
                closestGo = go;
                closestGuid = guid;
            }
        }

        // If we found a quest object, move to it or loot it
        if (closestGo)
        {
            // Use a conservative distance (3.0) to ensure we're definitely close enough
            float lootDist = 3.0f;

            if (closestDist <= lootDist)
            {
                // We're close enough - loot it!
                LOG_INFO("playerbots", "QuestingAction: {} LOOTING {} (dist {})",
                    bot->GetName(), closestGo->GetName(), closestDist);

                // Dismount if mounted
                if (bot->IsMounted())
                {
                    bot->Dismount();
                    botAI->SetNextCheckDelay(sPlayerbotAIConfig->lootDelay);
                    return true;
                }

                // Stop moving before interacting
                if (bot->isMoving())
                    bot->StopMoving();

                bot->SendLoot(closestGuid, LOOT_CORPSE);
                botAI->SetNextCheckDelay(sPlayerbotAIConfig->lootDelay);
                return true;
            }
            else
            {
                // Move to the closest object
                LOG_INFO("playerbots", "QuestingAction: {} moving to {} (dist {})",
                    bot->GetName(), closestGo->GetName(), closestDist);
                WorldPosition goPos(closestGo->GetMapId(), closestGo->GetPositionX(),
                    closestGo->GetPositionY(), closestGo->GetPositionZ());
                return MoveFarTo(goPos);
            }
        }

        // No quest objects found, wander
        return MoveRandomNear(20.0f);
    }

    return MoveFarTo(targetPos);
}

bool QuestingExecuteAction::DoTravelToTurnIn()
{
    uint32 questId = botAI->rpgInfo.questing.questId;

    // Try to interact with nearby quest givers for turn-in
    if (SearchQuestGiverAndAcceptOrReward())
        return true;

    // Get POI for quest turn-in
    std::vector<POIInfo> poiInfo;
    if (!GetQuestPOIPosAndObjectiveIdx(questId, poiInfo, true))
    {
        // Can't find turn-in location, go idle
        botAI->rpgInfo.ChangeToIdle();
        return true;
    }

    if (poiInfo.empty())
    {
        botAI->rpgInfo.ChangeToIdle();
        return true;
    }

    // Move to quest turn-in NPC
    G3D::Vector2 poi = poiInfo[0].pos;
    float dz = std::max(bot->GetMap()->GetHeight(poi.x, poi.y, MAX_HEIGHT), bot->GetMap()->GetWaterLevel(poi.x, poi.y));

    if (dz == INVALID_HEIGHT || dz == VMAP_INVALID_HEIGHT_VALUE)
    {
        botAI->rpgInfo.ChangeToIdle();
        return true;
    }

    WorldPosition targetPos(bot->GetMapId(), poi.x, poi.y, dz);
    botAI->rpgInfo.questing.targetPos = targetPos;

    return MoveFarTo(targetPos);
}

bool QuestingExecuteAction::DoSearchForQuests()
{
    // Try to interact with nearby quest givers
    if (SearchQuestGiverAndAcceptOrReward())
        return true;

    // Wander around to find quest givers
    return MoveRandomNear(30.0f);
}

bool QuestingExecuteAction::Execute(Event /*event*/)
{
    NewRpgInfo& info = botAI->rpgInfo;

    if (info.status != RPG_QUESTING)
        return false;

    switch (info.questing.subStatus)
    {
        case QUESTING_TRAVELING_TO_ACCEPT:
            return DoTravelToAccept();

        case QUESTING_TRAVELING_TO_OBJECTIVE:
            return DoTravelToObjective();

        case QUESTING_TRAVELING_TO_TURNIN:
            return DoTravelToTurnIn();

        case QUESTING_SEARCHING:
            return DoSearchForQuests();

        case QUESTING_IDLE:
        default:
            return false;
    }
}

// AttackQuestTargetAction implementation

bool AttackQuestTargetAction::isUseful()
{
    // Only useful when in questing status traveling to objective
    if (botAI->rpgInfo.status != RPG_QUESTING)
        return false;

    if (botAI->rpgInfo.questing.subStatus != QUESTING_TRAVELING_TO_OBJECTIVE)
        return false;

    // Check if we have a quest target available
    Unit* target = AI_VALUE(Unit*, "quest target");
    return target != nullptr;
}

// QuestingRecoverStuckAction implementation

bool QuestingRecoverStuckAction::Execute(Event /*event*/)
{
    // Increment stuck attempt counter
    botAI->rpgInfo.stuckAttempts++;

    LOG_INFO("playerbots", "QuestingRecoverStuckAction: {} is stuck (attempt {}), attempting recovery",
        bot->GetName(), botAI->rpgInfo.stuckAttempts);

    // Notify the player
    std::ostringstream msg;
    msg << "I'm stuck (attempt " << botAI->rpgInfo.stuckAttempts << "), trying to recover...";
    botAI->GetServices().GetChatService().TellMasterNoFacing(msg.str());

    // Strategy depends on stuck attempt count
    if (botAI->rpgInfo.stuckAttempts <= 2)
    {
        // First attempts: Try random movement to break pathing issues
        LOG_DEBUG("playerbots", "QuestingRecoverStuckAction: {} trying random movement", bot->GetName());

        // Move in a random direction
        float angle = frand(0.0f, 2.0f * M_PI);
        float dist = frand(10.0f, 25.0f);
        float x = bot->GetPositionX() + dist * cos(angle);
        float y = bot->GetPositionY() + dist * sin(angle);
        float z = bot->GetMap()->GetHeight(x, y, bot->GetPositionZ() + 5.0f);

        if (z != INVALID_HEIGHT && z != VMAP_INVALID_HEIGHT_VALUE)
        {
            return MoveNear(bot->GetMapId(), x, y, z, 0);
        }
    }
    else if (botAI->rpgInfo.stuckAttempts <= 4)
    {
        // Middle attempts: Try to find approach waypoint and go there
        LOG_DEBUG("playerbots", "QuestingRecoverStuckAction: {} trying to return to approach waypoint", bot->GetName());

        uint32 questId = botAI->rpgInfo.questing.questId;
        if (questId && sManagerRegistry.HasCoreQuestDataMgr())
        {
            auto& questDataMgr = sManagerRegistry.GetCoreQuestDataMgr();

            // Try quest-specific waypoints first
            std::vector<ApproachWaypoint> waypoints = questDataMgr.GetQuestApproachWaypoints(questId, bot->GetMapId());

            if (waypoints.empty())
            {
                // Fall back to zone-based waypoints
                uint32 zoneId = bot->GetZoneId();
                waypoints = questDataMgr.GetZoneApproachWaypoints(zoneId, bot->GetMapId());
            }

            if (!waypoints.empty())
            {
                // Pick a random waypoint to try
                uint32 idx = urand(0, waypoints.size() - 1);
                const ApproachWaypoint& wp = waypoints[idx];

                botAI->GetServices().GetChatService().TellMasterNoFacing("Returning to approach waypoint...");
                return MoveFarTo(WorldPosition(bot->GetMapId(), wp.x, wp.y, wp.z));
            }
        }

        // No waypoints found - try larger random movement
        float angle = frand(0.0f, 2.0f * M_PI);
        float dist = frand(30.0f, 50.0f);
        float x = bot->GetPositionX() + dist * cos(angle);
        float y = bot->GetPositionY() + dist * sin(angle);
        float z = bot->GetMap()->GetHeight(x, y, bot->GetPositionZ() + 5.0f);

        if (z != INVALID_HEIGHT && z != VMAP_INVALID_HEIGHT_VALUE)
        {
            return MoveNear(bot->GetMapId(), x, y, z, 0);
        }
    }
    else
    {
        // Many attempts: Reset quest state and try a different objective
        LOG_DEBUG("playerbots", "QuestingRecoverStuckAction: {} giving up on current target, resetting", bot->GetName());

        botAI->GetServices().GetChatService().TellMasterNoFacing("Too many stuck attempts, trying different approach...");

        // Reset the target position to force re-selection
        botAI->rpgInfo.questing.targetPos = WorldPosition();
        botAI->rpgInfo.stuckAttempts = 0;

        // If we've been stuck too many times, consider abandoning quest objective temporarily
        if (botAI->rpgInfo.stuckAttempts > 6)
        {
            botAI->rpgInfo.ChangeToIdle();
            botAI->GetServices().GetChatService().TellMasterNoFacing("Resetting quest state, will re-evaluate shortly.");
        }
    }

    return true;
}
