/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_QUESTTARGETVALUE_H
#define _PLAYERBOT_QUESTTARGETVALUE_H

#include "TargetValue.h"

class PlayerbotAI;
class Unit;

/**
 * @brief Value that returns only mobs needed for the current quest being worked on
 *
 * This value checks the current questing quest ID from rpgInfo.questing and finds
 * mobs that match the quest's RequiredNpcOrGo entries and haven't been killed enough yet.
 */
class QuestTargetValue : public TargetValue
{
public:
    QuestTargetValue(PlayerbotAI* botAI, std::string const name = "quest target") : TargetValue(botAI, name) {}

    Unit* Calculate() override;

private:
    bool IsNeededForCurrentQuest(Unit* target, uint32 questId);
    bool IsQuestObjectiveComplete(uint32 questId);
};

#endif
