#include "Playerbots.h"
#include "StockadeTriggers.h"
#include "AiObject.h"
#include "AiObjectContext.h"

bool BazilSmokeBombTrigger::IsActive()
{
    // Check if bot is stunned by Smoke Bomb
    return bot->HasAura(SPELL_SMOKE_BOMB);
}
