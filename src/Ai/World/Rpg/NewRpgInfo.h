#ifndef _PLAYERBOT_NEWRPGINFO_H
#define _PLAYERBOT_NEWRPGINFO_H

#include "Define.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "QuestDef.h"
#include "Strategy.h"
#include "Timer.h"
#include "TravelMgr.h"

using NewRpgStatusTransitionProb = std::vector<std::vector<int>>;

// Substatus for deterministic questing (RPG_QUESTING status)
enum QuestingSubStatus : uint8
{
    QUESTING_IDLE = 0,
    QUESTING_TRAVELING_TO_ACCEPT = 1,
    QUESTING_TRAVELING_TO_OBJECTIVE = 2,
    QUESTING_TRAVELING_TO_TURNIN = 3,
    QUESTING_SEARCHING = 4  // Wandering to find quest givers
};

struct NewRpgInfo
{
    NewRpgInfo() {}

    // RPG_GO_GRIND
    struct GoGrind
    {
        WorldPosition pos{};
    };
    // RPG_GO_CAMP
    struct GoCamp
    {
        WorldPosition pos{};
    };
    // RPG_WANDER_NPC
    struct WanderNpc
    {
        ObjectGuid npcOrGo{};
        uint32 lastReach{0};
    };
    // RPG_WANDER_RANDOM
    struct WanderRandom
    {
        WanderRandom() = default;
    };
    // RPG_DO_QUEST
    struct DoQuest
    {
        Quest const* quest{nullptr};
        uint32 questId{0};
        int32 objectiveIdx{0};
        WorldPosition pos{};
        uint32 lastReachPOI{0};
    };
    // RPG_TRAVEL_FLIGHT
    struct TravelFlight
    {
        ObjectGuid fromFlightMaster{};
        uint32 fromNode{0};
        uint32 toNode{0};
        bool inFlight{false};
    };
    // RPG_REST
    struct Rest
    {
        Rest() = default;
    };
    struct Idle
    {
    };
    // RPG_QUESTING (deterministic questing for +questing strategy)
    struct Questing
    {
        uint32 questId{0};
        Quest const* quest{nullptr};
        QuestingSubStatus subStatus{QUESTING_IDLE};
        WorldPosition targetPos{};
    };
    NewRpgStatus status{RPG_IDLE};

    uint32 startT{0};  // start timestamp of the current status

    // MOVE_FAR
    float nearestMoveFarDis{FLT_MAX};
    uint32 stuckTs{0};
    uint32 stuckAttempts{0};
    WorldPosition moveFarPos;
    // END MOVE_FAR

    union
    {
        GoGrind go_grind;
        GoCamp go_camp;
        WanderNpc wander_npc;
        WanderRandom WANDER_RANDOM;
        DoQuest do_quest;
        Rest rest;
        DoQuest quest;
        TravelFlight flight;
        Questing questing;
    };

    bool HasStatusPersisted(uint32 maxDuration) { return GetMSTimeDiffToNow(startT) > maxDuration; }
    void ChangeToGoGrind(WorldPosition pos);
    void ChangeToGoCamp(WorldPosition pos);
    void ChangeToWanderNpc();
    void ChangeToWanderRandom();
    void ChangeToDoQuest(uint32 questId, Quest const* quest);
    void ChangeToTravelFlight(ObjectGuid fromFlightMaster, uint32 fromNode, uint32 toNode);
    void ChangeToRest();
    void ChangeToIdle();
    void ChangeToQuesting(uint32 questId, Quest const* quest, QuestingSubStatus subStatus);
    bool CanChangeTo(NewRpgStatus status);
    void Reset();
    void SetMoveFarTo(WorldPosition pos);
    std::string ToString();
};

struct NewRpgStatistic
{
    uint32 questAccepted{0};
    uint32 questCompleted{0};
    uint32 questAbandoned{0};
    uint32 questRewarded{0};
    uint32 questDropped{0};
    NewRpgStatistic operator+(NewRpgStatistic const& other) const
    {
        NewRpgStatistic result;
        result.questAccepted = this->questAccepted + other.questAccepted;
        result.questCompleted = this->questCompleted + other.questCompleted;
        result.questAbandoned = this->questAbandoned + other.questAbandoned;
        result.questRewarded = this->questRewarded + other.questRewarded;
        result.questDropped = this->questDropped + other.questDropped;
        return result;
    }
    NewRpgStatistic& operator+=(NewRpgStatistic const& other)
    {
        this->questAccepted += other.questAccepted;
        this->questCompleted += other.questCompleted;
        this->questAbandoned += other.questAbandoned;
        this->questRewarded += other.questRewarded;
        this->questDropped += other.questDropped;
        return *this;
    }
};

// not sure is it necessary but keep it for now
#define RPG_INFO(x, y) botAI->rpgInfo.x.y

#endif