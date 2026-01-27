#ifndef _PLAYERBOT_TBCDUNGEONHRTRIGGERCONTEXT_H
#define _PLAYERBOT_TBCDUNGEONHRTRIGGERCONTEXT_H

#include "NamedObjectContext.h"
#include "AiObjectContext.h"
#include "HellfireRampartsTriggers.h"

class TbcDungeonHRTriggerContext : public NamedObjectContext<Trigger>
{
    public:
        TbcDungeonHRTriggerContext()
        {
            creators["omor treacherous aura"] = &TbcDungeonHRTriggerContext::omor_treacherous_aura;
            creators["nazan liquid fire"] = &TbcDungeonHRTriggerContext::nazan_liquid_fire;
            creators["nazan flame breath"] = &TbcDungeonHRTriggerContext::nazan_flame_breath;
        }
    private:
        static Trigger* omor_treacherous_aura(PlayerbotAI* ai) { return new OmorTreacherousAuraTrigger(ai); }
        static Trigger* nazan_liquid_fire(PlayerbotAI* ai) { return new NazanLiquidFireTrigger(ai); }
        static Trigger* nazan_flame_breath(PlayerbotAI* ai) { return new NazanFlameBreathTrigger(ai); }
};

#endif
