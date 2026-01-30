/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_TRAVELSAFETYTRIGGERS_H
#define _PLAYERBOT_TRAVELSAFETYTRIGGERS_H

#include "Trigger.h"

class PlayerbotAI;

/**
 * @brief Trigger that fires when a dangerous mob pack is blocking the travel path
 */
class MobPackInPathTrigger : public Trigger
{
public:
    MobPackInPathTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mob pack in path", 2) {}

    bool IsActive() override;
};

/**
 * @brief Trigger that fires when we can safely pull a single mob from a pack
 */
class CanPullFromPackTrigger : public Trigger
{
public:
    CanPullFromPackTrigger(PlayerbotAI* botAI) : Trigger(botAI, "can pull from pack", 2) {}

    bool IsActive() override;
};

/**
 * @brief Trigger that fires when travel has failed too many times and should be abandoned
 */
class TravelRetryExhaustedTrigger : public Trigger
{
public:
    TravelRetryExhaustedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "travel retry exhausted", 5) {}

    bool IsActive() override;
};

/**
 * @brief Trigger for when the bot should attempt to avoid mobs by detour
 */
class ShouldAvoidMobsTrigger : public Trigger
{
public:
    ShouldAvoidMobsTrigger(PlayerbotAI* botAI) : Trigger(botAI, "should avoid mobs", 2) {}

    bool IsActive() override;
};

/**
 * @brief Trigger for when the bot is being chased after a failed avoidance attempt
 */
class BeingChasedDuringTravelTrigger : public Trigger
{
public:
    BeingChasedDuringTravelTrigger(PlayerbotAI* botAI) : Trigger(botAI, "being chased during travel", 1) {}

    bool IsActive() override;
};

#endif
