#ifndef _PLAYERBOT_TBCDUNGEONHRACTIONS_H
#define _PLAYERBOT_TBCDUNGEONHRACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "HellfireRampartsTriggers.h"

class AvoidTreacherousAuraAction : public MovementAction
{
public:
    AvoidTreacherousAuraAction(PlayerbotAI* ai) : MovementAction(ai, "avoid treacherous aura") {}
    bool Execute(Event event) override;
};

class AvoidLiquidFireAction : public MovementAction
{
public:
    AvoidLiquidFireAction(PlayerbotAI* ai) : MovementAction(ai, "avoid liquid fire") {}
    bool Execute(Event event) override;
};

class AvoidFlameBreathAction : public MovementAction
{
public:
    AvoidFlameBreathAction(PlayerbotAI* ai) : MovementAction(ai, "avoid flame breath") {}
    bool Execute(Event event) override;
};

#endif
