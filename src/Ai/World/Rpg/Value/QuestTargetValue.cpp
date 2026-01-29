/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "QuestTargetValue.h"

#include "Creature.h"
#include "LootMgr.h"
#include "NewRpgInfo.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "QuestDef.h"

Unit* QuestTargetValue::Calculate()
{
    // Only work when in questing status with an active quest
    if (botAI->rpgInfo.status != RPG_QUESTING)
        return nullptr;

    uint32 questId = botAI->rpgInfo.questing.questId;
    if (!questId)
        return nullptr;

    // Check if quest objective is already complete
    if (IsQuestObjectiveComplete(questId))
        return nullptr;

    // Get possible targets in range
    GuidVector targets = *context->GetValue<GuidVector>("possible targets");
    if (targets.empty())
        return nullptr;

    float closestDistance = FLT_MAX;
    Unit* result = nullptr;

    for (ObjectGuid const guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit)
            continue;

        if (!unit->IsInWorld() || unit->IsDuringRemoveFromWorld())
            continue;

        if (!unit->IsAlive())
            continue;

        // Check if this mob is needed for the current quest
        if (!IsNeededForCurrentQuest(unit, questId))
            continue;

        // Basic validity checks
        if (!bot->IsWithinLOSInMap(unit))
            continue;

        if (abs(bot->GetPositionZ() - unit->GetPositionZ()) > INTERACTION_DISTANCE)
            continue;

        // Check level difference (don't attack mobs much higher level)
        if ((int)unit->GetLevel() - (int)bot->GetLevel() > 4)
            continue;

        // Check for elite mobs if bot can't handle them
        if (Creature* creature = unit->ToCreature())
        {
            if (CreatureTemplate const* creatureTemplate = creature->GetCreatureTemplate())
            {
                if (creatureTemplate->rank > CREATURE_ELITE_NORMAL && !AI_VALUE(bool, "can fight elite"))
                    continue;
            }
        }

        // Find closest matching mob
        float distance = bot->GetDistance(unit);
        if (distance < closestDistance)
        {
            closestDistance = distance;
            result = unit;
        }
    }

    return result;
}

bool QuestTargetValue::IsNeededForCurrentQuest(Unit* target, uint32 questId)
{
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    QuestStatus status = bot->GetQuestStatus(questId);
    if (status != QUEST_STATUS_INCOMPLETE)
        return false;

    auto& questMap = bot->getQuestStatusMap();
    auto it = questMap.find(questId);
    if (it == questMap.end())
        return false;

    QuestStatusData const& questStatus = it->second;

    // Check kill objectives (RequiredNpcOrGo)
    for (int j = 0; j < QUEST_OBJECTIVES_COUNT; j++)
    {
        int32 entry = quest->RequiredNpcOrGo[j];

        // Only check creature entries (positive values)
        if (entry > 0)
        {
            int required = quest->RequiredNpcOrGoCount[j];
            int available = questStatus.CreatureOrGOCount[j];

            // If this objective needs more kills and the target matches
            if (required > 0 && available < required && target->GetEntry() == static_cast<uint32>(entry))
                return true;
        }
    }

    // Check item drop objectives - does this creature drop items we need?
    for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; i++)
    {
        uint32 itemId = quest->RequiredItemId[i];
        if (!itemId)
            continue;

        uint32 required = quest->RequiredItemCount[i];
        uint32 available = questStatus.ItemCount[i];

        // If we still need this item, check if creature drops it
        if (available < required)
        {
            if (Creature* creature = target->ToCreature())
            {
                if (CreatureTemplate const* creatureTemplate = creature->GetCreatureTemplate())
                {
                    if (uint32 lootId = creatureTemplate->lootid)
                    {
                        // Check if this creature's loot table contains the quest item
                        if (LootTemplates_Creature.HaveQuestLootFor(lootId))
                        {
                            // More specific check - does it have the specific item we need?
                            if (LootTemplates_Creature.HaveQuestLootForPlayer(lootId, bot))
                                return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool QuestTargetValue::IsQuestObjectiveComplete(uint32 questId)
{
    QuestStatus status = bot->GetQuestStatus(questId);

    // Quest is complete when status is QUEST_STATUS_COMPLETE
    return status == QUEST_STATUS_COMPLETE;
}
