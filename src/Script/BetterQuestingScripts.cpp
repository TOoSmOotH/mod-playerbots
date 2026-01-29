/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#include "Mgr/Quest/BetterQuesting.h"
#include "Mgr/Quest/CoreQuestDataMgr.h"
#include "Mgr/Quest/QuestGuideMgr.h"
#include "BetterQuestingConfig.h"
#include "Bot/Core/ManagerRegistry.h"
#include "ScriptMgr.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "Log.h"
#include "Player.h"
#include "QuestDef.h"

// WorldScript to load config on startup
class BetterQuestingConfigScript : public WorldScript
{
public:
    BetterQuestingConfigScript() : WorldScript("BetterQuestingConfigScript", {
        WORLDHOOK_ON_AFTER_CONFIG_LOAD
    }) { }

    void OnAfterConfigLoad(bool reload) override
    {
        sBetterQuestingConfig->LoadConfig(reload);
    }
};

// WorldScript to build quest data indexes on server startup
class BetterQuestingWorldScript : public WorldScript
{
public:
    BetterQuestingWorldScript() : WorldScript("BetterQuestingWorldScript", {
        WORLDHOOK_ON_STARTUP
    }) { }

    void OnStartup() override
    {
        // Register BetterQuesting services with ManagerRegistry
        // Using no-op deleter since these are singletons
        sManagerRegistry.SetBetterQuestingConfig(
            std::shared_ptr<IBetterQuestingConfig>(sBetterQuestingConfig, [](IBetterQuestingConfig*){}));
        sManagerRegistry.SetCoreQuestDataMgr(
            std::shared_ptr<ICoreQuestDataMgr>(sCoreQuestDataMgr, [](ICoreQuestDataMgr*){}));
        sManagerRegistry.SetQuestGuideMgr(
            std::shared_ptr<IQuestGuideMgr>(sQuestGuideMgr, [](IQuestGuideMgr*){}));

        LOG_INFO("module", "BetterQuesting: Registered services with ManagerRegistry");

        // Load quest data
        sCoreQuestDataMgr->LoadData();

        // Load quest guide data
        sQuestGuideMgr->LoadData();

        // Boost quest priority when BetterQuesting is enabled
        if (sBetterQuestingConfig->IsEnabled() && sBetterQuestingConfig->IsQuestPriorityBoostEnabled())
        {
            // Make quests the dominant activity (92% effective weight)
            sPlayerbotAIConfig->RpgStatusProbWeight[RPG_DO_QUEST] = 100;
            sPlayerbotAIConfig->RpgStatusProbWeight[RPG_WANDER_RANDOM] = 2;
            sPlayerbotAIConfig->RpgStatusProbWeight[RPG_WANDER_NPC] = 2;
            sPlayerbotAIConfig->RpgStatusProbWeight[RPG_GO_GRIND] = 1;
            sPlayerbotAIConfig->RpgStatusProbWeight[RPG_GO_CAMP] = 1;
            sPlayerbotAIConfig->RpgStatusProbWeight[RPG_TRAVEL_FLIGHT] = 2;
            sPlayerbotAIConfig->RpgStatusProbWeight[RPG_REST] = 1;

            LOG_INFO("module", "BetterQuesting: Quest priority boosted (weight 100 vs others 1-2)");
        }
    }
};

// PlayerScript for guide initialization and quest tracking
class BetterQuestingPlayerScript : public PlayerScript
{
public:
    BetterQuestingPlayerScript() : PlayerScript("BetterQuestingPlayerScript", {
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_LOGOUT,
        PLAYERHOOK_ON_PLAYER_COMPLETE_QUEST,
        PLAYERHOOK_ON_LEVEL_CHANGED
    }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (!BetterQuesting::IsEnabled())
            return;

        // Load player's guide progress from database
        sQuestGuideMgr->LoadPlayerProgress(player->GetGUID().GetCounter());

        // Initialize guide if needed (auto-select or sync progress)
        BetterQuesting::InitializePlayerGuide(player);

        // Restore questing activity state if saved
        RestoreQuestingState(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (!BetterQuesting::IsEnabled())
            return;

        // Save questing activity state if player is in questing mode
        SaveQuestingState(player);
    }

private:
    void SaveQuestingState(Player* player)
    {
        if (!player)
            return;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
        if (!botAI)
            return;

        // Check if player is in RPG_QUESTING status
        if (botAI->rpgInfo.status == RPG_QUESTING)
        {
            uint32 questId = botAI->rpgInfo.questing.questId;
            uint8 subStatus = static_cast<uint8>(botAI->rpgInfo.questing.subStatus);

            if (questId > 0)
            {
                sQuestGuideMgr->SetActiveQuestState(player, questId, subStatus);
                LOG_DEBUG("module", "BetterQuesting: Saved questing state on logout for {}: quest {}, subStatus {}",
                    player->GetName(), questId, subStatus);
            }
        }
        else
        {
            // Clear saved state if not questing
            sQuestGuideMgr->ClearActiveQuestState(player);
        }
    }

    void RestoreQuestingState(Player* player)
    {
        if (!player)
            return;

        PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
        if (!botAI)
            return;

        QuestGuideProgress const* progress = sQuestGuideMgr->GetPlayerProgress(player->GetGUID().GetCounter());
        if (!progress || progress->activeQuestId == 0)
            return;

        uint32 questId = progress->activeQuestId;
        uint8 savedSubStatus = progress->activeSubStatus;

        // Validate quest is still valid for this player
        QuestStatus questStatus = player->GetQuestStatus(questId);

        // Handle edge cases where quest state changed while offline
        QuestingSubStatus restoredSubStatus = static_cast<QuestingSubStatus>(savedSubStatus);

        if (questStatus == QUEST_STATUS_NONE)
        {
            // Quest was abandoned or never accepted - clear saved state
            LOG_DEBUG("module", "BetterQuesting: Quest {} no longer in log for {}, clearing saved state",
                questId, player->GetName());
            sQuestGuideMgr->ClearActiveQuestState(player);
            return;
        }
        else if (questStatus == QUEST_STATUS_COMPLETE)
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
                // Quest was accepted offline - should be working on objective
                restoredSubStatus = QUESTING_TRAVELING_TO_OBJECTIVE;
            }
            else if (restoredSubStatus == QUESTING_TRAVELING_TO_TURNIN)
            {
                // Quest objectives no longer complete - go back to objective
                restoredSubStatus = QUESTING_TRAVELING_TO_OBJECTIVE;
            }
        }

        // Get quest template to restore state
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
        {
            LOG_DEBUG("module", "BetterQuesting: Quest template {} not found for {}, clearing saved state",
                questId, player->GetName());
            sQuestGuideMgr->ClearActiveQuestState(player);
            return;
        }

        // Note: We don't actually restore the rpgInfo state here because
        // the bot AI needs to be fully initialized first. Instead, we log
        // that we have saved state available. The questing strategy will
        // pick this up when it initializes and use it to resume.
        LOG_INFO("module", "BetterQuesting: Restored questing state for {}: quest {} ({}), subStatus {}",
            player->GetName(), questId, quest->GetTitle(), static_cast<uint8>(restoredSubStatus));

        // Update the saved state if we adjusted the substatus
        if (restoredSubStatus != static_cast<QuestingSubStatus>(savedSubStatus))
        {
            sQuestGuideMgr->SetActiveQuestState(player, questId, static_cast<uint8>(restoredSubStatus));
        }
    }

public:

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (!BetterQuesting::IsEnabled() || !quest)
            return;

        // Log quest chain info when player completes a quest
        if (sManagerRegistry.HasBetterQuestingConfig() &&
            sManagerRegistry.GetBetterQuestingConfig().IsLogFallbackEnabled())
        {
            uint32 nextQuest = sCoreQuestDataMgr->GetNextQuestInChain(quest->GetQuestId());
            if (nextQuest > 0)
            {
                LOG_DEBUG("module", "BetterQuesting: Player {} completed quest {}, next in chain: {}",
                    player->GetName(), quest->GetQuestId(), nextQuest);
            }
        }

        // Update guide progress if quest was current guide step
        if (BetterQuesting::IsQuestGuideEnabled())
        {
            QuestGuideStep const* currentStep = sQuestGuideMgr->GetCurrentStep(player);
            if (currentStep && currentStep->questId == quest->GetQuestId())
            {
                // Advance to next step
                sQuestGuideMgr->AdvancePlayerStep(player);
            }

            // Sync progress anyway to be safe
            sQuestGuideMgr->SyncPlayerProgress(player);
        }
    }

    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override
    {
        if (!BetterQuesting::IsQuestGuideEnabled())
            return;

        // Check if we need to transition zones based on new level
        uint32 targetZone = 0;
        if (sQuestGuideMgr->ShouldTransitionZone(player, targetZone))
        {
            // Simply re-syncing might trigger new guide selection if implemented
            sQuestGuideMgr->SyncPlayerProgress(player);
        }
    }
};

void AddSC_BetterQuestingHooks()
{
    new BetterQuestingConfigScript();
    new BetterQuestingWorldScript();
    new BetterQuestingPlayerScript();
}
