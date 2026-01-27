#include "Playerbots.h"
#include "BotRoleService.h"
#include "DeadminesActions.h"

bool AttackVanCleefAddAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "edwin vancleef");
    if (!boss) { return false; }

    // Search for Defias Blackguard adds
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->GetEntry() == NPC_DEFIAS_BLACKGUARD)
        {
            Unit* currentTarget = AI_VALUE(Unit*, "current target");
            // Only switch if not already targeting a Blackguard
            if (!currentTarget || currentTarget->GetEntry() != NPC_DEFIAS_BLACKGUARD)
            {
                return Attack(unit);
            }
        }
    }

    return false;
}
