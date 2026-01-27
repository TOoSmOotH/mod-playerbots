#ifndef _PLAYERBOT_TBCDUNGEONHRACTIONCONTEXT_H
#define _PLAYERBOT_TBCDUNGEONHRACTIONCONTEXT_H

#include "Action.h"
#include "NamedObjectContext.h"
#include "HellfireRampartsActions.h"

class TbcDungeonHRActionContext : public NamedObjectContext<Action>
{
    public:
        TbcDungeonHRActionContext() {
            creators["avoid treacherous aura"] = &TbcDungeonHRActionContext::avoid_treacherous_aura;
            creators["avoid liquid fire"] = &TbcDungeonHRActionContext::avoid_liquid_fire;
            creators["avoid flame breath"] = &TbcDungeonHRActionContext::avoid_flame_breath;
        }
    private:
        static Action* avoid_treacherous_aura(PlayerbotAI* ai) { return new AvoidTreacherousAuraAction(ai); }
        static Action* avoid_liquid_fire(PlayerbotAI* ai) { return new AvoidLiquidFireAction(ai); }
        static Action* avoid_flame_breath(PlayerbotAI* ai) { return new AvoidFlameBreathAction(ai); }
};

#endif
