/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#ifndef MOD_BETTERQUESTING_CONFIG_H
#define MOD_BETTERQUESTING_CONFIG_H

#include "Common.h"

class BetterQuestingConfig
{
public:
    static BetterQuestingConfig* instance();

    void LoadConfig(bool reload = false);

    bool IsEnabled() const { return _enabled; }
    bool IsPOIFallbackEnabled() const { return _poiFallback; }
    bool IsPrerequisiteCheckEnabled() const { return _prerequisiteCheck; }
    bool IsQuestChainAwarenessEnabled() const { return _questChainAwareness; }
    float GetMaxSpawnDistance() const { return _maxSpawnDistance; }
    bool IsLogFallbackEnabled() const { return _logFallback; }
    bool IsQuestPriorityBoostEnabled() const { return _questPriorityBoost; }
    bool IsPathValidationEnabled() const { return _pathValidation; }

    // Quest Guide system
    // Quest Guide system
    bool IsQuestGuideEnabled() const { return _questGuideEnabled; }
    bool IsQuestGuideStrictMode() const { return _guideStrictMode; }
    int32 GetGuidePriorityBonus() const { return _guidePriorityBonus; }
    bool IsAutoGuideSelectionEnabled() const { return _autoGuideSelection; }
    bool IsQuestGuideZoneTransitionEnabled() const { return _guideZoneTransitionEnabled; }
    bool IsQuestGuideSkipUnavailableEnabled() const { return _guideSkipUnavailableEnabled; }

    // Multi-level Z probing (cave navigation)
    bool IsMultiLevelZProbeEnabled() const { return _multiLevelZProbe; }
    float GetMaxZProbeDepth() const { return _maxZProbeDepth; }

    // Approach waypoints system
    bool IsApproachWaypointsEnabled() const { return _approachWaypointsEnabled; }
    uint32 GetMaxApproachWaypoints() const { return _maxApproachWaypoints; }

private:
    BetterQuestingConfig() = default;
    ~BetterQuestingConfig() = default;

    bool _enabled = false;
    bool _poiFallback = true;
    bool _prerequisiteCheck = true;
    bool _questChainAwareness = true;
    float _maxSpawnDistance = 2500.0f;
    bool _logFallback = false;
    bool _questPriorityBoost = true;
    bool _pathValidation = true;

    // Quest Guide system
    bool _questGuideEnabled = true;
    bool _guideStrictMode = true;
    int32 _guidePriorityBonus = 50;
    bool _autoGuideSelection = true;
    bool _guideZoneTransitionEnabled = true;
    bool _guideSkipUnavailableEnabled = true;

    // Multi-level Z probing (cave navigation)
    bool _multiLevelZProbe = true;
    float _maxZProbeDepth = 150.0f;

    // Approach waypoints system
    bool _approachWaypointsEnabled = true;
    uint32 _maxApproachWaypoints = 10;
};

#define sBetterQuestingConfig BetterQuestingConfig::instance()

#endif // MOD_BETTERQUESTING_CONFIG_H
