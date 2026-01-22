/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#include "BetterQuesting.h"
#include "BetterQuestingConfig.h"
#include "CoreQuestDataMgr.h"
#include "QuestGuideMgr.h"
#include "ScriptMgr.h"
#include "PlayerbotAIConfig.h"
#include "Log.h"
#include "Player.h"
#include "QuestDef.h"

// WorldScript to build quest data indexes on server startup
class BetterQuestingWorldScript : public WorldScript
{
public:
    BetterQuestingWorldScript() : WorldScript("BetterQuestingWorldScript", {
        WORLDHOOK_ON_STARTUP
    }) { }

    void OnStartup() override
    {
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
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        if (!BetterQuesting::IsEnabled() || !quest)
            return;

        // Log quest chain info when player completes a quest
        if (sBetterQuestingConfig->IsLogFallbackEnabled())
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
    new BetterQuestingWorldScript();
    new BetterQuestingPlayerScript();
}
