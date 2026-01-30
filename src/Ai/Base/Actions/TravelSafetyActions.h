/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_TRAVELSAFETYACTIONS_H
#define _PLAYERBOT_TRAVELSAFETYACTIONS_H

#include "MovementActions.h"
#include "MobPackValue.h"

class PlayerbotAI;

/**
 * @brief Action to avoid mob packs by calculating and following a detour path
 *
 * This action scans for mob packs blocking the travel path and calculates
 * perpendicular detour waypoints to navigate around them safely.
 */
class AvoidMobPackAction : public MovementAction
{
public:
    AvoidMobPackAction(PlayerbotAI* botAI) : MovementAction(botAI, "avoid mob pack") {}

    bool Execute(Event event) override;
    bool isUseful() override;

private:
    /**
     * @brief Calculate a detour point to avoid a mob pack
     * @param pack The pack to avoid
     * @param destX Destination X coordinate
     * @param destY Destination Y coordinate
     * @param detourX Output detour X coordinate
     * @param detourY Output detour Y coordinate
     * @param detourZ Output detour Z coordinate
     * @return true if a valid detour point was found
     */
    bool CalculateDetourPoint(const MobPackInfo& pack, float destX, float destY,
                              float& detourX, float& detourY, float& detourZ);

    /**
     * @brief Check if a position is safe from all hostile mobs
     */
    bool IsPositionSafe(float x, float y, float z, float minDistance);
};

/**
 * @brief Action to pull a single mob from a pack using kiting strategy
 *
 * This action identifies the most isolated, lowest-level mob in a pack
 * and uses ranged abilities to pull it away from the pack for safer combat.
 */
class TravelPullAction : public MovementAction
{
public:
    TravelPullAction(PlayerbotAI* botAI) : MovementAction(botAI, "travel pull") {}

    bool Execute(Event event) override;
    bool isUseful() override;

private:
    /**
     * @brief Calculate the best position to kite the mob to
     * @param pullTarget The mob being pulled
     * @param pack The pack it belongs to
     * @param kiteX Output kite position X
     * @param kiteY Output kite position Y
     * @param kiteZ Output kite position Z
     * @return true if a valid kite position was found
     */
    bool CalculateKitePosition(Unit* pullTarget, const MobPackInfo& pack,
                               float& kiteX, float& kiteY, float& kiteZ);

    /**
     * @brief Get the best ranged pull spell for this class
     * @return Spell ID or 0 if no ranged ability available
     */
    uint32 GetPullSpellId();

    /**
     * @brief Attempt to cast the pull ability on target
     */
    bool CastPullSpell(Unit* target);
};

/**
 * @brief Action to flee toward travel destination when overwhelmed
 *
 * Unlike normal flee which picks optimal flee direction, this action
 * prefers fleeing toward the travel destination to make progress while evading.
 */
class FleeToTravelSafePointAction : public MovementAction
{
public:
    FleeToTravelSafePointAction(PlayerbotAI* botAI)
        : MovementAction(botAI, "flee to travel safe point") {}

    bool Execute(Event event) override;
    bool isUseful() override;

private:
    /**
     * @brief Calculate a safe flee point biased toward travel destination
     */
    bool CalculateSafeFleePoint(float& fleeX, float& fleeY, float& fleeZ);

    /**
     * @brief Increment the travel retry counter
     */
    void IncrementRetryCount();
};

#endif
