/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "StuckTriggers.h"

#include "CellImpl.h"
#include "PathGenerator.h"
#include "Playerbots.h"
#include "MMapFactory.h"
#include "NewRpgInfo.h"

bool MoveStuckTrigger::IsActive()
{
    if (botAI->HasActivePlayerMaster())
        return false;

    if (!botAI->AllowActivity(ALL_ACTIVITY))
        return false;

    WorldPosition botPos(bot);

    LogCalculatedValue<WorldPosition>* posVal =
        dynamic_cast<LogCalculatedValue<WorldPosition>*>(context->GetUntypedValue("current position"));

    if (posVal->LastChangeDelay() > 5 * MINUTE)
    {
        // LOG_INFO("playerbots", "Bot {} {}:{} <{}> was in the same position for {} seconds",
        // bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
        // bot->GetName(), posVal->LastChangeDelay());

        return true;
    }

    bool longLog = false;

    for (auto tPos : posVal->ValueLog())
    {
        uint32 timePassed = time(0) - tPos.second;

        if (timePassed > 10 * MINUTE)
        {
            if (botPos.fDist(tPos.first) > 50.0f)
                return false;

            longLog = true;
        }
    }

    if (longLog)
    {
        // LOG_INFO("playerbots", "Bot {} {}:{} <{}> was in the same position for 10mins",
        // bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
        // bot->GetName(), posVal->LastChangeDelay());
    }

    return longLog;
}

bool MoveLongStuckTrigger::IsActive()
{
    if (botAI->HasActivePlayerMaster())
        return false;

    if (!botAI->AllowActivity(ALL_ACTIVITY))
        return false;

    WorldPosition botPos(bot);

    Cell cell(bot->GetPositionX(), bot->GetPositionY());

    GridCoord grid = botPos.getGridCoord();

    if (grid.x_coord < 0 || grid.x_coord >= MAX_NUMBER_OF_GRIDS)
    {
        // LOG_INFO("playerbots", "Bot {} {}:{} <{}> was in grid {},{} on map {}",
        // bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
        // bot->GetName(), grid.x_coord, grid.y_coord, botPos.getMapId());

        return true;
    }

    if (grid.y_coord < 0 || grid.y_coord >= MAX_NUMBER_OF_GRIDS)
    {
        // LOG_INFO("playerbots", "Bot {} {}:{} <{}> was in grid {},{} on map {}",
        // bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
        // bot->GetName(), grid.x_coord, grid.y_coord, botPos.getMapId());

        return true;
    }

    if (cell.GridX() > 0 && cell.GridY() > 0 &&
        !MMAP::MMapFactory::createOrGetMMapMgr()->loadMap(botPos.getMapId(), cell.GridX(), cell.GridY()))
    {
        // LOG_INFO("playerbots", "Bot {} {}:{} <{}> was in unloaded grid {},{} on map {}",
        // bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
        // bot->GetName(), grid.x_coord, grid.y_coord, botPos.getMapId());

        return true;
    }

    LogCalculatedValue<WorldPosition>* posVal =
        dynamic_cast<LogCalculatedValue<WorldPosition>*>(context->GetUntypedValue("current position"));

    if (posVal->LastChangeDelay() > 10 * MINUTE)
    {
        // LOG_INFO("playerbots", "Bot {} {}:{} <{}> was in the same position for {} seconds",
        // bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
        // bot->GetName(), posVal->LastChangeDelay());

        return true;
    }

    MemoryCalculatedValue<uint32>* expVal =
        dynamic_cast<MemoryCalculatedValue<uint32>*>(context->GetUntypedValue("experience"));

    if (expVal->LastChangeDelay() < 15 * MINUTE)
        return false;

    bool longLog = false;

    for (auto tPos : posVal->ValueLog())
    {
        uint32 timePassed = time(0) - tPos.second;

        if (timePassed > 15 * MINUTE)
        {
            if (botPos.fDist(tPos.first) > 50.0f)
                return false;

            longLog = true;
        }
    }

    if (longLog)
    {
        // LOG_INFO("playerbots", "Bot {} {}:{} <{}> was in the same position for 15mins",
        // bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
        // bot->GetName(), posVal->LastChangeDelay());
    }

    return longLog;
}

bool CombatStuckTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    if (botAI->HasActivePlayerMaster())
        return false;

    if (!botAI->AllowActivity(ALL_ACTIVITY))
        return false;

    WorldPosition botPos(bot);

    MemoryCalculatedValue<bool>* combatVal =
        dynamic_cast<MemoryCalculatedValue<bool>*>(context->GetUntypedValue("combat::self target"));

    if (combatVal->LastChangeDelay() > 5 * MINUTE)
    {
        // LOG_INFO("playerbots", "Bot {} {}:{} <{}> was in combat for {} seconds",
        // bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
        // bot->GetName(), posVal->LastChangeDelay());

        return true;
    }

    return false;
}

bool CombatLongStuckTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    if (botAI->HasActivePlayerMaster())
        return false;

    if (!botAI->AllowActivity(ALL_ACTIVITY))
        return false;

    WorldPosition botPos(bot);

    MemoryCalculatedValue<bool>* combatVal =
        dynamic_cast<MemoryCalculatedValue<bool>*>(context->GetUntypedValue("combat::self target"));

    if (combatVal->LastChangeDelay() > 15 * MINUTE)
    {
        // LOG_INFO("playerbots", "Bot {} {}:{} <{}> was in combat for {} seconds",
        // bot->GetGUID().ToString().c_str(), bot->GetTeamId() == TEAM_ALLIANCE ? "A" : "H", bot->GetLevel(),
        // bot->GetName(), posVal->LastChangeDelay());

        return true;
    }

    return false;
}

bool SelfbotQuestingStuckTrigger::IsActive()
{
    // This trigger is specifically for selfbots (when player controls the bot)
    if (!botAI->HasActivePlayerMaster())
        return false;

    // Only active when in questing mode and traveling to objective or turn-in
    if (botAI->rpgInfo.status != RPG_QUESTING)
        return false;

    QuestingSubStatus subStatus = botAI->rpgInfo.questing.subStatus;
    if (subStatus != QUESTING_TRAVELING_TO_OBJECTIVE && subStatus != QUESTING_TRAVELING_TO_TURNIN)
        return false;

    WorldPosition botPos(bot);

    LogCalculatedValue<WorldPosition>* posVal =
        dynamic_cast<LogCalculatedValue<WorldPosition>*>(context->GetUntypedValue("current position"));

    if (!posVal)
        return false;

    // Use shorter timeout for selfbots (90 seconds instead of 5+ minutes)
    // since a real player is watching and expects faster feedback
    if (posVal->LastChangeDelay() > 90)
    {
        LOG_DEBUG("playerbots", "SelfbotQuestingStuckTrigger: Bot {} stuck for {} seconds while questing",
            bot->GetName(), posVal->LastChangeDelay());

        return true;
    }

    // Also check if bot has been making no progress towards target
    // by checking position history over last 60 seconds
    bool noProgress = true;
    for (auto tPos : posVal->ValueLog())
    {
        uint32 timePassed = time(0) - tPos.second;

        if (timePassed > 60)
        {
            // If we've moved more than 20 yards in the last 60 seconds, we're not stuck
            if (botPos.fDist(tPos.first) > 20.0f)
            {
                noProgress = false;
                break;
            }
        }
    }

    if (noProgress && posVal->ValueLog().size() > 3)
    {
        LOG_DEBUG("playerbots", "SelfbotQuestingStuckTrigger: Bot {} has made no progress (< 20 yards) in 60+ seconds",
            bot->GetName());
        return true;
    }

    return false;
}
