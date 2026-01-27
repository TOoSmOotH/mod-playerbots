#include "StockadeMultipliers.h"
#include "StockadeTriggers.h"
#include "Action.h"

float BazilThreddMultiplier::GetValue(Action* action)
{
    // Check if bot is stunned by Smoke Bomb
    if (bot->HasAura(SPELL_SMOKE_BOMB))
    {
        // Disable all actions while stunned
        return 0.0f;
    }

    return 1.0f;
}
