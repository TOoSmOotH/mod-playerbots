#include "DeadminesStrategy.h"
#include "DeadminesMultipliers.h"

void ClassicDungeonDMStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Rhahk'Zor
    // - Slam stun is brief (3 sec) and unavoidable, no special handling needed

    // Sneed's Shredder / Sneed
    // - Two-phase fight, but bots handle phase transitions automatically
    // - Terrify (fear) cannot be avoided, healers/dispellers handle naturally

    // Gilnid
    // - Molten Metal is a DoT, healers handle naturally

    // Mr. Smite
    // - Smite Stomp: 10 sec AoE stun during phase transitions
    // - Multiplier handles disabling actions during stun
    // (No trigger needed since we can't take any action during stun anyway)

    // Captain Greenskin
    // - No special abilities, tank and spank

    // Edwin VanCleef
    // - Spawns Defias Blackguard adds at 50% health
    triggers.push_back(new TriggerNode("vancleef adds",
        { NextAction("attack vancleef add", ACTION_RAID + 4) }));

    // Cookie (optional boss)
    // - Easy fight, self-heal and minor AoE, no special handling needed
}

void ClassicDungeonDMStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new MrSmiteMultiplier(botAI));
    multipliers.push_back(new VanCleefMultiplier(botAI));
}
