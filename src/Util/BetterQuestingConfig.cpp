/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>
 * Released under GNU AGPL v3 license
 */

#include "BetterQuestingConfig.h"
#include "Config.h"
#include "Log.h"

BetterQuestingConfig* BetterQuestingConfig::instance()
{
    static BetterQuestingConfig instance;
    return &instance;
}

void BetterQuestingConfig::LoadConfig(bool reload)
{
    _enabled = sConfigMgr->GetOption<bool>("BetterQuesting.Enabled", true);
    _poiFallback = sConfigMgr->GetOption<bool>("BetterQuesting.POIFallback", true);
    _prerequisiteCheck = sConfigMgr->GetOption<bool>("BetterQuesting.PrerequisiteCheck", true);
    _questChainAwareness = sConfigMgr->GetOption<bool>("BetterQuesting.QuestChainAwareness", true);
    _maxSpawnDistance = sConfigMgr->GetOption<float>("BetterQuesting.MaxSpawnDistance", 2500.0f);
    _logFallback = sConfigMgr->GetOption<bool>("BetterQuesting.LogFallback", false);
    _questPriorityBoost = sConfigMgr->GetOption<bool>("BetterQuesting.QuestPriorityBoost", true);
    _pathValidation = sConfigMgr->GetOption<bool>("BetterQuesting.PathValidation", true);

    // Quest Guide system
    _questGuideEnabled = sConfigMgr->GetOption<bool>("BetterQuesting.QuestGuide.Enabled", true);
    _guideStrictMode = sConfigMgr->GetOption<bool>("BetterQuesting.QuestGuide.StrictMode", true);
    _guidePriorityBonus = sConfigMgr->GetOption<int32>("BetterQuesting.QuestGuide.PriorityBonus", 50);
    _autoGuideSelection = sConfigMgr->GetOption<bool>("BetterQuesting.QuestGuide.AutoSelect", true);
    _guideZoneTransitionEnabled = sConfigMgr->GetOption<bool>("BetterQuesting.QuestGuide.AllowZoneTransition", true);
    _guideSkipUnavailableEnabled = sConfigMgr->GetOption<bool>("BetterQuesting.QuestGuide.SkipUnavailable", true);

    // Multi-level Z probing (cave navigation)
    _multiLevelZProbe = sConfigMgr->GetOption<bool>("BetterQuesting.MultiLevelZProbe.Enabled", true);
    _maxZProbeDepth = sConfigMgr->GetOption<float>("BetterQuesting.MultiLevelZProbe.MaxDepth", 150.0f);

    // Approach waypoints system
    _approachWaypointsEnabled = sConfigMgr->GetOption<bool>("BetterQuesting.ApproachWaypoints.Enabled", true);
    _maxApproachWaypoints = sConfigMgr->GetOption<uint32>("BetterQuesting.ApproachWaypoints.MaxWaypoints", 10);

    if (reload)
    {
        LOG_INFO("module", "BetterQuesting: Configuration reloaded");
    }
    else
    {
        LOG_INFO("module", "BetterQuesting: Module {} (POI Fallback: {}, Prereq Check: {}, Chain Awareness: {})",
            _enabled ? "enabled" : "disabled",
            _poiFallback ? "enabled" : "disabled",
            _prerequisiteCheck ? "enabled" : "disabled",
            _questChainAwareness ? "enabled" : "disabled");
        LOG_INFO("module", "BetterQuesting: Quest Guide {} (Strict Mode: {}, Priority Bonus: {}, Auto Select: {})",
            _questGuideEnabled ? "enabled" : "disabled",
            _guideStrictMode ? "yes" : "no",
            _guidePriorityBonus,
            _autoGuideSelection ? "yes" : "no");
        LOG_INFO("module", "BetterQuesting: Cave Navigation - Z Probe: {} (max depth {}), Approach Waypoints: {}",
            _multiLevelZProbe ? "enabled" : "disabled",
            _maxZProbeDepth,
            _approachWaypointsEnabled ? "enabled" : "disabled");
    }
}
