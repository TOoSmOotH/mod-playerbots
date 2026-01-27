#ifndef _PLAYERBOT_CLASSICDUNGEONDMTRIGGERCONTEXT_H
#define _PLAYERBOT_CLASSICDUNGEONDMTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "AiObjectContext.h"
#include "DeadminesTriggers.h"

class ClassicDungeonDMTriggerContext : public NamedObjectContext<Trigger>
{
    public:
        ClassicDungeonDMTriggerContext()
        {
            creators["mr. smite stomp"] = &ClassicDungeonDMTriggerContext::mr_smite_stomp;
            creators["vancleef adds"] = &ClassicDungeonDMTriggerContext::vancleef_adds;
        }
    private:
        static Trigger* mr_smite_stomp(PlayerbotAI* ai) { return new MrSmiteStompTrigger(ai); }
        static Trigger* vancleef_adds(PlayerbotAI* ai) { return new VanCleefAddsTrigger(ai); }
};

#endif
