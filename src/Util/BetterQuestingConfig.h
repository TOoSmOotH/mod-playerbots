/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#ifndef MOD_BETTERQUESTING_CONFIG_H
#define MOD_BETTERQUESTING_CONFIG_H

#include "Bot/Interface/IBetterQuestingConfig.h"

class BetterQuestingConfig : public IBetterQuestingConfig
{
public:
    static BetterQuestingConfig* instance();

    void LoadConfig(bool reload = false) override;

    bool IsEnabled() const override { return _enabled; }
    bool IsPOIFallbackEnabled() const override { return _poiFallback; }
    bool IsPrerequisiteCheckEnabled() const override { return _prerequisiteCheck; }
    bool IsQuestChainAwarenessEnabled() const override { return _questChainAwareness; }
    float GetMaxSpawnDistance() const override { return _maxSpawnDistance; }
    bool IsLogFallbackEnabled() const override { return _logFallback; }
    bool IsQuestPriorityBoostEnabled() const override { return _questPriorityBoost; }
    bool IsPathValidationEnabled() const override { return _pathValidation; }

    // Quest Guide system
    bool IsQuestGuideEnabled() const override { return _questGuideEnabled; }
    bool IsQuestGuideStrictMode() const override { return _guideStrictMode; }
    int32 GetGuidePriorityBonus() const override { return _guidePriorityBonus; }
    bool IsAutoGuideSelectionEnabled() const override { return _autoGuideSelection; }
    bool IsQuestGuideZoneTransitionEnabled() const override { return _guideZoneTransitionEnabled; }
    bool IsQuestGuideSkipUnavailableEnabled() const override { return _guideSkipUnavailableEnabled; }

    // Multi-level Z probing (cave navigation)
    bool IsMultiLevelZProbeEnabled() const override { return _multiLevelZProbe; }
    float GetMaxZProbeDepth() const override { return _maxZProbeDepth; }

    // Approach waypoints system
    bool IsApproachWaypointsEnabled() const override { return _approachWaypointsEnabled; }
    uint32 GetMaxApproachWaypoints() const override { return _maxApproachWaypoints; }

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
