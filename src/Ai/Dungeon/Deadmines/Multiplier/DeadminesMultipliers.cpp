#include "BotRoleService.h"
#include "DeadminesMultipliers.h"
#include "DeadminesActions.h"
#include "GenericSpellActions.h"
#include "ChooseTargetActions.h"
#include "MovementActions.h"
#include "DeadminesTriggers.h"
#include "Action.h"

float MrSmiteMultiplier::GetValue(Action* action)
{
    // Check if bot is stunned by Smite Stomp
    if (bot->HasAura(SPELL_SMITE_STOMP))
    {
        // Disable all actions while stunned - can't do anything anyway
        // This prevents bots from queuing actions that will fail
        return 0.0f;
    }

    return 1.0f;
}

float VanCleefMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "edwin vancleef");
    if (!boss) { return 1.0f; }

    if (!BotRoleService::IsDpsStatic(bot)) { return 1.0f; }

    // Check if Defias Blackguard adds are present
    Unit* blackguard = nullptr;
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");
    for (auto& target : targets)
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->GetEntry() == NPC_DEFIAS_BLACKGUARD)
        {
            blackguard = unit;
            break;
        }
    }

    // Prevent auto-target acquisition away from adds when they're present
    if (blackguard && dynamic_cast<DpsAssistAction*>(action))
    {
        return 0.0f;
    }

    return 1.0f;
}
