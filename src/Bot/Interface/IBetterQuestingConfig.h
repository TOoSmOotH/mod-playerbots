/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_IBETTERQUESTING_CONFIG_H
#define _PLAYERBOT_IBETTERQUESTING_CONFIG_H

#include "Common.h"

/**
 * @brief Interface for BetterQuesting configuration access
 *
 * This interface abstracts access to BetterQuesting configuration settings,
 * allowing for dependency injection and easier testing.
 */
class IBetterQuestingConfig
{
public:
    virtual ~IBetterQuestingConfig() = default;

    // Load/reload configuration
    virtual void LoadConfig(bool reload = false) = 0;

    // General settings
    virtual bool IsEnabled() const = 0;
    virtual bool IsPOIFallbackEnabled() const = 0;
    virtual bool IsPrerequisiteCheckEnabled() const = 0;
    virtual bool IsQuestChainAwarenessEnabled() const = 0;
    virtual float GetMaxSpawnDistance() const = 0;
    virtual bool IsLogFallbackEnabled() const = 0;
    virtual bool IsQuestPriorityBoostEnabled() const = 0;
    virtual bool IsPathValidationEnabled() const = 0;

    // Quest Guide system
    virtual bool IsQuestGuideEnabled() const = 0;
    virtual bool IsQuestGuideStrictMode() const = 0;
    virtual int32 GetGuidePriorityBonus() const = 0;
    virtual bool IsAutoGuideSelectionEnabled() const = 0;
    virtual bool IsQuestGuideZoneTransitionEnabled() const = 0;
    virtual bool IsQuestGuideSkipUnavailableEnabled() const = 0;

    // Multi-level Z probing (cave navigation)
    virtual bool IsMultiLevelZProbeEnabled() const = 0;
    virtual float GetMaxZProbeDepth() const = 0;

    // Approach waypoints system
    virtual bool IsApproachWaypointsEnabled() const = 0;
    virtual uint32 GetMaxApproachWaypoints() const = 0;
};

#endif // _PLAYERBOT_IBETTERQUESTING_CONFIG_H
