#ifndef _PLAYERBOT_CLASSICDUNGEONDMACTIONS_H
#define _PLAYERBOT_CLASSICDUNGEONDMACTIONS_H

#include "Action.h"
#include "AttackAction.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "DeadminesTriggers.h"

class AttackVanCleefAddAction : public AttackAction
{
public:
    AttackVanCleefAddAction(PlayerbotAI* ai) : AttackAction(ai, "attack vancleef add") {}
    bool Execute(Event event) override;
};

#endif
