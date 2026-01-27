#include "StockadeStrategy.h"
#include "StockadeMultipliers.h"

void ClassicDungeonStocksStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // The Stockade is a simple, straightforward dungeon with no complex mechanics.
    // All bosses are basic tank-and-spank encounters.

    // Targorr the Dread
    // - Thrash: Extra attacks, just need good tank healing
    // - Enrage: Could be purged but not critical

    // Kam Deepfury
    // - Shield Slam: 2 sec stun on tank, healers handle naturally
    // - Improved Blocking: Defensive buff, just takes longer to kill

    // Hamhock
    // - Bloodlust: Buffs allies, could be purged but not critical

    // Dextren Ward
    // - Basic warrior abilities, tank and spank

    // Bazil Thredd
    // - Smoke Bomb: 4 sec AoE stun - handled by multiplier
    // - Battle Shout: Minor buff, not worth special handling
}

void ClassicDungeonStocksStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new BazilThreddMultiplier(botAI));
}
