#include "BotRoleService.h"
#include "Playerbots.h"
#include "DeadminesTriggers.h"
#include "AiObject.h"
#include "AiObjectContext.h"

bool MrSmiteStompTrigger::IsActive()
{
    // Check if bot has the Smite Stomp stun debuff
    return bot->HasAura(SPELL_SMITE_STOMP);
}

bool VanCleefAddsTrigger::IsActive()
{
    // Only DPS should switch to adds
    if (!BotRoleService::IsDpsStatic(bot)) { return false; }

    // Check if VanCleef is present (to ensure we're in the right fight)
    Unit* boss = AI_VALUE2(Unit*, "find target", "edwin vancleef");
    if (!boss) { return false; }

    // Search for Defias Blackguard adds
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");

    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->GetEntry() == NPC_DEFIAS_BLACKGUARD)
        {
            return true;
        }
    }
    return false;
}
