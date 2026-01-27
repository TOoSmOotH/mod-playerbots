#include "BotRoleService.h"
#include "Playerbots.h"
#include "HellfireRampartsTriggers.h"
#include "AiObject.h"
#include "AiObjectContext.h"

bool OmorTreacherousAuraTrigger::IsActive()
{
    // Check if bot has the Treacherous Aura curse
    // If so, bot needs to move away from allies
    return bot->HasAura(SPELL_TREACHEROUS_AURA);
}

bool NazanLiquidFireTrigger::IsActive()
{
    // Check if bot is standing in Liquid Fire
    return bot->HasAura(SPELL_LIQUID_FIRE_AURA);
}

bool NazanFlameBreathTrigger::IsActive()
{
    // Only non-tanks need to worry about being in front
    if (BotRoleService::IsTankStatic(bot)) { return false; }

    Unit* boss = AI_VALUE2(Unit*, "find target", "nazan");
    if (!boss) { return false; }

    // Check if boss is casting Cone of Fire / Flame Breath
    if (boss->FindCurrentSpellBySpellId(SPELL_CONE_OF_FIRE))
    {
        // Check if bot is in front of the boss (within 90 degree cone)
        float angle = boss->GetAbsoluteAngle(bot);
        float facing = boss->GetOrientation();
        float diff = fabs(angle - facing);
        if (diff > M_PI) diff = 2 * M_PI - diff;

        // If within frontal cone (45 degrees either side)
        return diff < M_PI / 4;
    }

    return false;
}
