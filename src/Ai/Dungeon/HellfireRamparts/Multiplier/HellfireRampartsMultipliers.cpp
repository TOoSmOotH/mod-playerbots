#include "BotRoleService.h"
#include "HellfireRampartsMultipliers.h"
#include "HellfireRampartsActions.h"
#include "GenericSpellActions.h"
#include "ChooseTargetActions.h"
#include "MovementActions.h"
#include "HellfireRampartsTriggers.h"
#include "Action.h"

float OmorMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "omor the unscarred");
    if (!boss) { return 1.0f; }

    // If bot has Treacherous Aura, prioritize movement to spread
    if (bot->HasAura(SPELL_TREACHEROUS_AURA))
    {
        // Disable other movement actions so spread action takes priority
        if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<AvoidTreacherousAuraAction*>(action))
        {
            return 0.0f;
        }
    }

    return 1.0f;
}

float NazanMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "nazan");
    if (!boss) { return 1.0f; }

    // If standing in fire, prioritize getting out
    if (bot->HasAura(SPELL_LIQUID_FIRE_AURA))
    {
        if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<AvoidLiquidFireAction*>(action))
        {
            return 0.0f;
        }
    }

    // If boss is casting flame breath and bot is in front, prioritize moving
    if (boss->FindCurrentSpellBySpellId(SPELL_CONE_OF_FIRE))
    {
        if (!BotRoleService::IsTankStatic(bot))
        {
            if (dynamic_cast<MovementAction*>(action) && !dynamic_cast<AvoidFlameBreathAction*>(action))
            {
                return 0.0f;
            }
        }
    }

    return 1.0f;
}
