/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_QUESTINGACTION_H
#define _PLAYERBOT_QUESTINGACTION_H

#include "AttackAction.h"
#include "NewRpgBaseAction.h"
#include "NewRpgInfo.h"

/**
 * @brief Action that updates the questing status and selects next quest to work on
 *
 * This action is responsible for:
 * 1. Checking Quest Guide for current step
 * 2. Falling back to quest log for incomplete quests
 * 3. Setting the appropriate questing substatus
 * 4. Whispering status updates to the owner
 */
class QuestingUpdateAction : public NewRpgBaseAction
{
public:
    QuestingUpdateAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "questing update") {}

    bool Execute(Event event) override;

protected:
    void WhisperStatus(std::string const& msg);
    void SetQuestingState(uint32 questId, Quest const* quest, QuestingSubStatus subStatus);
    bool TryRestoreSavedQuestState();
    bool TryAcceptQuestFromGuide();
    bool TryFindNearbyQuestToAccept();
    bool TryDoIncompleteQuest();
    bool TryTurnInCompletedQuest();

    std::string lastWhisperedStatus;
    bool hasAttemptedRestore{false};
};

/**
 * @brief Action that executes the current questing task
 *
 * This action handles the actual movement and interaction
 * for accepting, completing, and turning in quests.
 */
class QuestingExecuteAction : public NewRpgBaseAction
{
public:
    QuestingExecuteAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "questing execute") {}

    bool Execute(Event event) override;

protected:
    bool DoTravelToAccept();
    bool DoTravelToObjective();
    bool DoTravelToTurnIn();
    bool DoSearchForQuests();

    const uint32 poiStayTime = 5 * 60 * 1000;
};

/**
 * @brief Action that attacks only quest-specific mobs
 *
 * This action targets only mobs needed for the current quest being worked on.
 * It stops attacking once the quest objective is complete.
 */
class AttackQuestTargetAction : public AttackAction
{
public:
    AttackQuestTargetAction(PlayerbotAI* botAI) : AttackAction(botAI, "attack quest target") {}

    std::string const GetTargetName() override { return "quest target"; }
    bool isUseful() override;
};

/**
 * @brief Action that recovers from being stuck during questing
 *
 * This action is triggered when the selfbot questing stuck trigger fires.
 * It attempts various recovery strategies:
 * 1. Try alternative spawn point for objective
 * 2. Try returning to approach waypoint
 * 3. Random movement to break stuck state
 */
class QuestingRecoverStuckAction : public NewRpgBaseAction
{
public:
    QuestingRecoverStuckAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "questing recover stuck") {}

    bool Execute(Event event) override;
};

#endif
