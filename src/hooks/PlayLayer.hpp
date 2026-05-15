#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

class StartPosObject;

struct HookPlayLayer : geode::Modify<HookPlayLayer, PlayLayer> {
    struct Fields {
        std::vector<geode::Ref<GameObject>> m_startPosObjects = {};
        std::vector<geode::Ref<GameObject>> m_dualPortals = {};
        std::vector<geode::Ref<GameObject>> m_gamemodePortals = {};
        std::vector<geode::Ref<GameObject>> m_miniPortals = {};
        std::vector<geode::Ref<GameObject>> m_speedChanges = {};
        std::vector<int> m_guidedCompletedRoutes = {};
        std::vector<int> m_guidedAttemptRouteIDs = {};
        std::vector<int> m_guidedAttemptRouteCounts = {};
        int m_startPosIdx = 0;
        int m_activeRunStartIdx = 0;
        int m_guidedChainLength = 1;
        int m_guidedWindowStart = -1;
        bool m_activeRunAttemptCounted = false;
        bool m_guidedRunCleared = false;
        bool m_guidedStartPosPending = false;
        bool m_guidedSwitching = false;
        bool m_ignoreProgressUntilRunBegin = false;
        bool m_canSwitch = true;
    };

    void addObject(GameObject* obj);
    void createObjectsFromSetupFinished();
    void destroyPlayer(PlayerObject* player, GameObject* object);
    void levelComplete();
    void resetLevel();
    void updateProgressbar();

    void updateStartPos(int index);
    void setLearnerStartPos(int index, bool shouldReset);
    void syncLearnerStartPosMusic();
    void syncLearnerStartPosMusicDelayed(float);
    void queueLearnerStartPosMusicSync();
    void updateSmartStartPositions();
    void applySmartStartPos(StartPosObject* startPos);
    GameObject* getClosestSmartObject(std::vector<geode::Ref<GameObject>>& objects, StartPosObject* startPos);
    void beginLearnerRun();
    int getLearnerStartPercent(int index);
    int getLearnerClearTargetPercent(int index);
    void normalizeGuidedRoute();
    int getGuidedRouteLength(int startIndex, int phase);
    bool isGuidedRoutePlayable(int startIndex, int phase);
    int getGuidedRouteID(int startIndex, int phase);
    bool isGuidedRouteIDPlayable(int routeID);
    void cleanGuidedRouteData();
    int getGuidedCompletedStageCount();
    bool isGuidedRouteCompleted(int startIndex, int phase);
    void markGuidedRouteCompleted(int startIndex, int phase);
    int getGuidedRouteAttempts(int startIndex, int phase);
    void clearGuidedRouteAttempts(int startIndex, int phase);
    bool recordGuidedRouteAttempt();
    bool rotateGuidedRouteInPhase(int startIndex, int phase);
    bool advanceGuidedRouteToNextIncompletePhase();
    int getGuidedLastStartForPhase(int phase);
    int chooseGuidedStartForPhase(int phase, bool allowZeroChance);
    int getGuidedZeroChancePercent();
    int getGuidedPhaseIndex();
    int getGuidedPhaseCount();
    int getGuidedPhaseStageIndex();
    int getGuidedPhaseStageCount();
    int getGuidedStageIndex();
    int getGuidedStageCount();
    int getGuidedRunTargetPercent();
    bool updateGuidedProgress(bool completed = false);
    void advanceGuidedRoute();
    int chooseGuidedStartPos();
    bool applyGuidedStartPos(bool shouldReset = true, bool resetIfSame = false);
    void loadGuidedProgress();
    void saveGuidedProgress();
    std::string getLearnerSaveKey();
};
