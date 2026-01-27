#ifndef _PLAYERBOT_CLASSICDUNGEONDMACTIONCONTEXT_H
#define _PLAYERBOT_CLASSICDUNGEONDMACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "DeadminesActions.h"

class ClassicDungeonDMActionContext : public NamedObjectContext<Action>
{
    public:
        ClassicDungeonDMActionContext() {
            creators["attack vancleef add"] = &ClassicDungeonDMActionContext::attack_vancleef_add;
        }
    private:
        static Action* attack_vancleef_add(PlayerbotAI* ai) { return new AttackVanCleefAddAction(ai); }
};

#endif
