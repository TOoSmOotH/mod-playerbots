#include "Playerbots.h"
#include "BotRoleService.h"
#include "HellfireRampartsActions.h"

bool AvoidTreacherousAuraAction::Execute(Event /*event*/)
{
    // Bot has Treacherous Aura - need to move away from allies
    // The aura deals shadow damage to nearby friendlies within 15 yards

    Unit* boss = AI_VALUE2(Unit*, "find target", "omor the unscarred");
    if (!boss) { return false; }

    // Find nearest party member to move away from
    Group* group = bot->GetGroup();
    if (!group) { return false; }

    float spreadRadius = 15.0f;
    float safeDistance = 18.0f;  // Move a bit further than the radius

    Unit* nearestAlly = nullptr;
    float nearestDist = spreadRadius;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive())
            continue;

        float dist = bot->GetExactDist2d(member);
        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearestAlly = member;
        }
    }

    if (nearestAlly)
    {
        return MoveAway(nearestAlly, safeDistance - nearestDist);
    }

    return false;
}

bool AvoidLiquidFireAction::Execute(Event /*event*/)
{
    // Move away from current position (out of fire)
    // The fire pools are stationary, so just need to move away

    float moveDistance = 5.0f;

    // Try to move towards the boss (fire pools are usually around the edges)
    Unit* boss = AI_VALUE2(Unit*, "find target", "nazan");
    if (!boss)
    {
        boss = AI_VALUE2(Unit*, "find target", "vazruden");
    }

    if (boss)
    {
        float distance = bot->GetExactDist2d(boss);
        if (distance > 10.0f)
        {
            // Move towards boss to get out of fire
            return MoveNear(boss, 8.0f);
        }
        else
        {
            // Already close, move away from fire in any direction
            return MoveAway(bot, moveDistance);
        }
    }

    // No boss found, just move away from current position
    return MoveAway(bot, moveDistance);
}

bool AvoidFlameBreathAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "nazan");
    if (!boss) { return false; }

    // Move to the side/behind the boss
    float radius = 8.0f;
    return MoveAway(boss, radius);
}
