#include "PlayLayer.hpp"
#include "Geode/binding/FMODAudioEngine.hpp"
#include "Geode/binding/StartPosObject.hpp"
#include "UILayer.hpp"
#include "../ModManager.hpp"

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
    int percentFromX(float x, float levelLength) {
        if (levelLength <= 0.f) {
            return 0;
        }

        auto percent = static_cast<int>(std::round(x / levelLength * 100.f));
        return std::clamp(percent, 0, 100);
    }

    uint64_t stableHash(std::string const& value) {
        auto hash = uint64_t(14695981039346656037ull);
        for (auto ch : value) {
            hash ^= static_cast<unsigned char>(ch);
            hash *= 1099511628211ull;
        }
        return hash;
    }
}

void HookPlayLayer::addObject(GameObject* obj) {
    auto fields = m_fields.self();
    if (obj->m_objectID == 31) {
        if(!static_cast<StartPosObject*>(obj)->m_startSettings->m_disableStartPos || !ModManager::sharedState()->m_ignoreDisabled)
            fields->m_startPosObjects.push_back(obj);
    }

    switch (obj->m_objectID) {
        case 12:
        case 13:
        case 47:
        case 111:
        case 660:
        case 745:
        case 1331:
        case 1933:
            fields->m_gamemodePortals.push_back(obj);
            break;
        case 99:
        case 101:
            fields->m_miniPortals.push_back(obj);
            break;
        case 200:
        case 201:
        case 202:
        case 203:
        case 1334:
            fields->m_speedChanges.push_back(obj);
            break;
        case 286:
        case 287:
            fields->m_dualPortals.push_back(obj);
            break;
        default:
            break;
    }
    PlayLayer::addObject(obj);
}

void HookPlayLayer::updateStartPos(int idx) {
    setLearnRStartPos(idx, true);
}

void HookPlayLayer::setLearnRStartPos(int idx, bool shouldReset) {
    auto fields = m_fields.self();

    if (fields->m_startPosObjects.size() == 0)
        return;

    if(idx < 0) idx = fields->m_startPosObjects.size();
    if(idx > fields->m_startPosObjects.size()) idx = 0;

    
    if(idx == 0) {
        m_isTestMode = false;
        updateTestModeLabel();
    } else {
        m_isTestMode = true;
        updateTestModeLabel();
    }

    m_currentCheckpoint = nullptr;
    fields->m_startPosIdx = idx;

    GameObject* object = nullptr;

    if(true) {
        object = idx > 0 ? fields->m_startPosObjects[idx - 1] : nullptr;
    } else {
        auto rand = std::rand() % fields->m_startPosObjects.size() + 1;
        object = idx > 0 ? fields->m_startPosObjects[rand] : nullptr;
    }
    auto startPos = static_cast<StartPosObject*>(object);
    applySmartStartPos(startPos);
    setStartPosObject(startPos);

    if (!shouldReset) {
        static_cast<HookUILayer*>(m_uiLayer)->updateUI();
        return;
    }

    if(m_isPracticeMode)
        resetLevelFromStart();

    resetLevel();
    startMusic();
    syncLearnRStartPosMusic();

    static_cast<HookUILayer*>(m_uiLayer)->updateUI();
}

void HookPlayLayer::syncLearnRStartPosMusic() {
    if (!m_startPosObject || !m_startPosObject->m_startSettings) {
        return;
    }

    auto settings = m_startPosObject->m_startSettings;
    auto levelSongOffset = m_levelSettings ? m_levelSettings->m_songOffset : 0.f;
    auto startPosSongOffset = settings->m_songOffset;
    if (std::abs(startPosSongOffset - levelSongOffset) < 0.001f) {
        startPosSongOffset = 0.f;
    }

    auto songTime = timeForPos(
        m_startPosObject->getPosition(),
        settings->m_targetOrder,
        settings->m_targetChannel,
        true,
        0
    ) + levelSongOffset + startPosSongOffset;
    auto songTimeMS = static_cast<unsigned int>(std::max(0.f, songTime) * 1000.f);
    FMODAudioEngine::get()->setMusicTimeMS(songTimeMS, true, 0);
}

void HookPlayLayer::syncLearnRStartPosMusicDelayed(float) {
    syncLearnRStartPosMusic();
}

void HookPlayLayer::queueLearnRStartPosMusicSync() {
    if (!m_startPosObject) {
        return;
    }

    syncLearnRStartPosMusic();
    unschedule(schedule_selector(HookPlayLayer::syncLearnRStartPosMusicDelayed));
    scheduleOnce(schedule_selector(HookPlayLayer::syncLearnRStartPosMusicDelayed), 0.05f);
}

GameObject* HookPlayLayer::getClosestSmartObject(std::vector<geode::Ref<GameObject>>& objects, StartPosObject* startPos) {
    if (!startPos) {
        return nullptr;
    }

    std::sort(objects.begin(), objects.end(), [](auto a, auto b) {
        return a->getPositionX() < b->getPositionX();
    });

    auto closest = static_cast<GameObject*>(nullptr);
    for (auto object : objects) {
        if (object->getPositionX() - 10.f > startPos->getPositionX()) {
            break;
        }
        if (object->getPositionX() - 10.f < startPos->getPositionX()) {
            closest = object;
        }
    }
    return closest;
}

void HookPlayLayer::applySmartStartPos(StartPosObject* startPos) {
    auto mm = ModManager::sharedState();
    if (!mm->m_smartStartpos || !startPos || !startPos->m_startSettings || !m_levelSettings) {
        return;
    }

    auto fields = m_fields.self();
    auto startSettings = startPos->m_startSettings;
    startSettings->m_startDual = m_levelSettings->m_startDual;
    startSettings->m_startMode = m_levelSettings->m_startMode;
    startSettings->m_startMini = m_levelSettings->m_startMini;
    startSettings->m_startSpeed = m_levelSettings->m_startSpeed;

    if (auto object = getClosestSmartObject(fields->m_dualPortals, startPos)) {
        startSettings->m_startDual = object->m_objectID == 286;
    }

    if (auto object = getClosestSmartObject(fields->m_gamemodePortals, startPos)) {
        switch (object->m_objectID) {
            case 12:
                startSettings->m_startMode = 0;
                break;
            case 13:
                startSettings->m_startMode = 1;
                break;
            case 47:
                startSettings->m_startMode = 2;
                break;
            case 111:
                startSettings->m_startMode = 3;
                break;
            case 660:
                startSettings->m_startMode = 4;
                break;
            case 745:
                startSettings->m_startMode = 5;
                break;
            case 1331:
                startSettings->m_startMode = 6;
                break;
            case 1933:
                startSettings->m_startMode = 7;
                break;
            default:
                break;
        }
    }

    if (auto object = getClosestSmartObject(fields->m_miniPortals, startPos)) {
        startSettings->m_startMini = object->m_objectID == 101;
    }

    if (auto object = getClosestSmartObject(fields->m_speedChanges, startPos)) {
        switch (object->m_objectID) {
            case 200:
                startSettings->m_startSpeed = Speed::Slow;
                break;
            case 201:
                startSettings->m_startSpeed = Speed::Normal;
                break;
            case 202:
                startSettings->m_startSpeed = Speed::Fast;
                break;
            case 203:
                startSettings->m_startSpeed = Speed::Faster;
                break;
            case 1334:
                startSettings->m_startSpeed = Speed::Fastest;
                break;
            default:
                break;
        }
    }
}

void HookPlayLayer::updateSmartStartPositions() {
    if (!ModManager::sharedState()->m_smartStartpos) {
        return;
    }

    for (auto object : m_fields->m_startPosObjects) {
        applySmartStartPos(static_cast<StartPosObject*>(object.data()));
    }
}

void HookPlayLayer::createObjectsFromSetupFinished() {
    PlayLayer::createObjectsFromSetupFinished();
    auto fields = m_fields.self();

    std::sort(fields->m_startPosObjects.begin(), fields->m_startPosObjects.end(), [](auto a, auto b) { return a->getPositionX() < b->getPositionX(); });

    if(this->m_startPosObject) {
        auto currentIdx = find(fields->m_startPosObjects.begin(), fields->m_startPosObjects.end(), this->m_startPosObject) - fields->m_startPosObjects.begin();
        fields->m_startPosIdx = currentIdx + 1;
    }

    ModManager::sharedState()->loadLevelSettings(getLearnRSaveKey());
    updateSmartStartPositions();
    loadGuidedProgress();
    applyGuidedStartPos(false);
    beginLearnRRun();

    static_cast<HookUILayer*>(m_uiLayer)->updateUI();
}

void HookPlayLayer::resetLevel() {
    if (m_fields->m_guidedStartPosPending) {
        m_fields->m_guidedStartPosPending = false;
        applyGuidedStartPos(false);
    }
    updateSmartStartPositions();
    PlayLayer::resetLevel();
    if (m_startPosObject) {
        prepareMusic(false);
        startMusic();
        queueLearnRStartPosMusicSync();
    } else {
        unschedule(schedule_selector(HookPlayLayer::syncLearnRStartPosMusicDelayed));
    }
    beginLearnRRun();
}

void HookPlayLayer::updateProgressbar() {
    PlayLayer::updateProgressbar();
    updateGuidedProgress();
    static_cast<HookUILayer*>(m_uiLayer)->updateGuidedChanceLabel();
}

void HookPlayLayer::destroyPlayer(PlayerObject* player, GameObject* object) {
    auto guidedAlreadyCleared = m_fields->m_guidedRunCleared;
    auto guidedCleared = updateGuidedProgress();
    if (!guidedAlreadyCleared && !guidedCleared) {
        if (!m_fields->m_activeRunAttemptCounted) {
            m_fields->m_activeRunAttemptCounted = true;
            recordGuidedRouteAttempt();
        }
    }
    PlayLayer::destroyPlayer(player, object);
}

void HookPlayLayer::levelComplete() {
    auto guidedCleared = updateGuidedProgress(true);
    (void)guidedCleared;
    PlayLayer::levelComplete();
}

void HookPlayLayer::beginLearnRRun() {
    auto fields = m_fields.self();
    fields->m_activeRunStartIdx = fields->m_startPosIdx;
    fields->m_activeRunAttemptCounted = false;
    fields->m_guidedRunCleared = false;
    fields->m_ignoreProgressUntilRunBegin = false;
    normalizeGuidedRoute();
}

int HookPlayLayer::getLearnRStartPercent(int index) {
    if (index <= 0) {
        return 0;
    }

    auto fields = m_fields.self();
    auto startPosIdx = static_cast<size_t>(index - 1);
    if (startPosIdx >= fields->m_startPosObjects.size()) {
        return 0;
    }

    return percentFromX(fields->m_startPosObjects[startPosIdx]->getPositionX(), m_levelLength);
}

int HookPlayLayer::getLearnRClearTargetPercent(int index) {
    auto fields = m_fields.self();
    auto nextStartPosIdx = static_cast<size_t>(index);
    if (nextStartPosIdx < fields->m_startPosObjects.size()) {
        return percentFromX(fields->m_startPosObjects[nextStartPosIdx]->getPositionX(), m_levelLength);
    }

    return 100;
}

void HookPlayLayer::normalizeGuidedRoute() {
    auto fields = m_fields.self();
    auto sectionCount = static_cast<int>(fields->m_startPosObjects.size() + 1);
    if (sectionCount <= 0) {
        fields->m_guidedChainLength = 1;
        fields->m_guidedWindowStart = 0;
        return;
    }

    cleanGuidedRouteData();
    fields->m_guidedChainLength = std::clamp(fields->m_guidedChainLength, 1, getGuidedPhaseCount());
    fields->m_guidedWindowStart = std::clamp(fields->m_guidedWindowStart, 0, sectionCount - 1);

    if (
        isGuidedRoutePlayable(fields->m_guidedWindowStart, fields->m_guidedChainLength) &&
        !isGuidedRouteCompleted(fields->m_guidedWindowStart, fields->m_guidedChainLength)
    ) {
        return;
    }

    auto tryPhase = [&](int phase) -> bool {
        auto start = chooseGuidedStartForPhase(phase, true);
        if (start < 0) {
            return false;
        }
        fields->m_guidedChainLength = phase;
        fields->m_guidedWindowStart = start;
        return true;
    };

    for (auto phase = fields->m_guidedChainLength; phase <= getGuidedPhaseCount(); phase++) {
        if (tryPhase(phase)) {
            return;
        }
    }

    for (auto phase = 1; phase < fields->m_guidedChainLength; phase++) {
        if (tryPhase(phase)) {
            return;
        }
    }

    fields->m_guidedChainLength = getGuidedPhaseCount();
    fields->m_guidedWindowStart = std::clamp(fields->m_guidedWindowStart, 0, sectionCount - 1);
}

int HookPlayLayer::getGuidedRouteLength(int startIndex, int phase) {
    auto fields = m_fields.self();
    auto sectionCount = static_cast<int>(fields->m_startPosObjects.size() + 1);
    if (startIndex < 0 || startIndex >= sectionCount) {
        return 0;
    }

    auto offset = 0;
    if (startIndex == 0) {
        offset = 2;
    } else if (getLearnRStartPercent(startIndex) < ModManager::sharedState()->m_guidedLateThreshold) {
        offset = 1;
    }

    auto length = phase - offset;
    auto maxLength = sectionCount - startIndex;
    if (length < 1 || length > maxLength) {
        return 0;
    }
    return length;
}

bool HookPlayLayer::isGuidedRoutePlayable(int startIndex, int phase) {
    return getGuidedRouteLength(startIndex, phase) > 0;
}

int HookPlayLayer::getGuidedRouteID(int startIndex, int phase) {
    return phase * 10000 + startIndex;
}

bool HookPlayLayer::isGuidedRouteIDPlayable(int routeID) {
    auto phase = routeID / 10000;
    auto start = routeID % 10000;
    return phase >= 1 && phase <= getGuidedPhaseCount() && isGuidedRoutePlayable(start, phase);
}

void HookPlayLayer::cleanGuidedRouteData() {
    auto fields = m_fields.self();

    auto cleanedRoutes = std::vector<int>();
    for (auto routeID : fields->m_guidedCompletedRoutes) {
        if (!isGuidedRouteIDPlayable(routeID)) {
            continue;
        }
        if (std::find(cleanedRoutes.begin(), cleanedRoutes.end(), routeID) == cleanedRoutes.end()) {
            cleanedRoutes.push_back(routeID);
        }
    }
    fields->m_guidedCompletedRoutes = cleanedRoutes;

    auto cleanedAttemptIDs = std::vector<int>();
    auto cleanedAttemptCounts = std::vector<int>();
    for (auto idx = 0u; idx < fields->m_guidedAttemptRouteIDs.size(); idx++) {
        auto routeID = fields->m_guidedAttemptRouteIDs[idx];
        if (
            !isGuidedRouteIDPlayable(routeID) ||
            std::find(fields->m_guidedCompletedRoutes.begin(), fields->m_guidedCompletedRoutes.end(), routeID) != fields->m_guidedCompletedRoutes.end()
        ) {
            continue;
        }

        auto count = idx < fields->m_guidedAttemptRouteCounts.size() ? fields->m_guidedAttemptRouteCounts[idx] : 0;
        if (count <= 0 || std::find(cleanedAttemptIDs.begin(), cleanedAttemptIDs.end(), routeID) != cleanedAttemptIDs.end()) {
            continue;
        }

        cleanedAttemptIDs.push_back(routeID);
        cleanedAttemptCounts.push_back(count);
    }
    fields->m_guidedAttemptRouteIDs = cleanedAttemptIDs;
    fields->m_guidedAttemptRouteCounts = cleanedAttemptCounts;
}

int HookPlayLayer::getGuidedCompletedStageCount() {
    auto count = 0;
    auto sectionCount = static_cast<int>(m_fields->m_startPosObjects.size() + 1);
    for (auto phase = 1; phase <= getGuidedPhaseCount(); phase++) {
        for (auto start = sectionCount - 1; start >= 0; start--) {
            if (isGuidedRoutePlayable(start, phase) && isGuidedRouteCompleted(start, phase)) {
                count++;
            }
        }
    }
    return count;
}

bool HookPlayLayer::isGuidedRouteCompleted(int startIndex, int phase) {
    auto id = getGuidedRouteID(startIndex, phase);
    auto& routes = m_fields->m_guidedCompletedRoutes;
    return std::find(routes.begin(), routes.end(), id) != routes.end();
}

void HookPlayLayer::markGuidedRouteCompleted(int startIndex, int phase) {
    if (!isGuidedRoutePlayable(startIndex, phase) || isGuidedRouteCompleted(startIndex, phase)) {
        return;
    }
    m_fields->m_guidedCompletedRoutes.push_back(getGuidedRouteID(startIndex, phase));
    clearGuidedRouteAttempts(startIndex, phase);
}

int HookPlayLayer::getGuidedRouteAttempts(int startIndex, int phase) {
    auto id = getGuidedRouteID(startIndex, phase);
    auto fields = m_fields.self();
    for (auto idx = 0u; idx < fields->m_guidedAttemptRouteIDs.size(); idx++) {
        if (fields->m_guidedAttemptRouteIDs[idx] == id && idx < fields->m_guidedAttemptRouteCounts.size()) {
            return fields->m_guidedAttemptRouteCounts[idx];
        }
    }
    return 0;
}

void HookPlayLayer::clearGuidedRouteAttempts(int startIndex, int phase) {
    auto id = getGuidedRouteID(startIndex, phase);
    auto fields = m_fields.self();
    for (auto idx = 0u; idx < fields->m_guidedAttemptRouteIDs.size(); idx++) {
        if (fields->m_guidedAttemptRouteIDs[idx] == id) {
            fields->m_guidedAttemptRouteIDs.erase(fields->m_guidedAttemptRouteIDs.begin() + idx);
            if (idx < fields->m_guidedAttemptRouteCounts.size()) {
                fields->m_guidedAttemptRouteCounts.erase(fields->m_guidedAttemptRouteCounts.begin() + idx);
            }
            return;
        }
    }
}

bool HookPlayLayer::recordGuidedRouteAttempt() {
    auto fields = m_fields.self();
    if (!ModManager::sharedState()->m_guidedMode || fields->m_startPosObjects.empty()) {
        return false;
    }

    normalizeGuidedRoute();
    auto start = fields->m_activeRunStartIdx;
    auto phase = fields->m_guidedChainLength;
    if (start != fields->m_guidedWindowStart || !isGuidedRoutePlayable(start, phase) || isGuidedRouteCompleted(start, phase)) {
        return false;
    }

    auto id = getGuidedRouteID(start, phase);
    auto attempts = 1;
    auto found = false;
    for (auto idx = 0u; idx < fields->m_guidedAttemptRouteIDs.size(); idx++) {
        if (fields->m_guidedAttemptRouteIDs[idx] == id) {
            if (idx >= fields->m_guidedAttemptRouteCounts.size()) {
                fields->m_guidedAttemptRouteCounts.resize(idx + 1, 0);
            }
            fields->m_guidedAttemptRouteCounts[idx]++;
            attempts = fields->m_guidedAttemptRouteCounts[idx];
            found = true;
            break;
        }
    }

    if (!found) {
        fields->m_guidedAttemptRouteIDs.push_back(id);
        fields->m_guidedAttemptRouteCounts.push_back(1);
    }

    if (attempts > ModManager::sharedState()->m_guidedAttemptLimit) {
        if (rotateGuidedRouteInPhase(start, phase) || advanceGuidedRouteToNextIncompletePhase()) {
            fields->m_guidedStartPosPending = true;
            saveGuidedProgress();
            return true;
        }
    }

    saveGuidedProgress();
    return false;
}

bool HookPlayLayer::rotateGuidedRouteInPhase(int startIndex, int phase) {
    auto fields = m_fields.self();
    auto candidates = std::vector<int>();
    auto sectionCount = static_cast<int>(fields->m_startPosObjects.size() + 1);
    for (auto start = sectionCount - 1; start >= 0; start--) {
        if (start != 0 && start != startIndex && isGuidedRoutePlayable(start, phase) && !isGuidedRouteCompleted(start, phase)) {
            candidates.push_back(start);
        }
    }

    auto zeroAvailable = startIndex != 0 &&
        isGuidedRoutePlayable(0, phase) &&
        !isGuidedRouteCompleted(0, phase);
    if (candidates.empty() && !zeroAvailable) {
        return false;
    }

    clearGuidedRouteAttempts(startIndex, phase);
    if (zeroAvailable && (candidates.empty() || std::rand() % 100 < getGuidedZeroChancePercent())) {
        fields->m_guidedWindowStart = 0;
    } else {
        fields->m_guidedWindowStart = candidates[std::rand() % candidates.size()];
    }
    return true;
}

bool HookPlayLayer::advanceGuidedRouteToNextIncompletePhase() {
    auto fields = m_fields.self();
    for (auto phase = fields->m_guidedChainLength + 1; phase <= getGuidedPhaseCount(); phase++) {
        auto start = chooseGuidedStartForPhase(phase, true);
        if (start >= 0) {
            fields->m_guidedChainLength = phase;
            fields->m_guidedWindowStart = start;
            return true;
        }
    }
    for (auto phase = 1; phase <= fields->m_guidedChainLength; phase++) {
        auto start = chooseGuidedStartForPhase(phase, true);
        if (start >= 0) {
            fields->m_guidedChainLength = phase;
            fields->m_guidedWindowStart = start;
            return true;
        }
    }
    return false;
}

int HookPlayLayer::getGuidedLastStartForPhase(int phase) {
    auto sectionCount = static_cast<int>(m_fields->m_startPosObjects.size() + 1);
    for (auto start = sectionCount - 1; start >= 0; start--) {
        if (isGuidedRoutePlayable(start, phase) && !isGuidedRouteCompleted(start, phase)) {
            return start;
        }
    }
    return -1;
}

int HookPlayLayer::chooseGuidedStartForPhase(int phase, bool allowZeroChance) {
    if (
        allowZeroChance &&
        isGuidedRoutePlayable(0, phase) &&
        !isGuidedRouteCompleted(0, phase) &&
        std::rand() % 100 < getGuidedZeroChancePercent()
    ) {
        return 0;
    }
    return getGuidedLastStartForPhase(phase);
}

int HookPlayLayer::getGuidedZeroChancePercent() {
    auto total = getGuidedStageCount();
    if (total <= 0) {
        return 0;
    }
    auto completed = getGuidedCompletedStageCount();
    auto currentStage = std::clamp(completed + (completed < total ? 1 : 0), 1, total);
    return std::clamp(static_cast<int>(std::round(static_cast<float>(currentStage) / total * 100.f)), 0, 100);
}

int HookPlayLayer::getGuidedPhaseIndex() {
    normalizeGuidedRoute();
    return m_fields->m_guidedChainLength;
}

int HookPlayLayer::getGuidedPhaseCount() {
    auto sectionCount = static_cast<int>(m_fields->m_startPosObjects.size() + 1);
    return sectionCount;
}

int HookPlayLayer::getGuidedPhaseStageIndex() {
    auto fields = m_fields.self();
    normalizeGuidedRoute();

    auto stage = 0;
    auto sectionCount = static_cast<int>(fields->m_startPosObjects.size() + 1);
    for (auto start = sectionCount - 1; start >= 0; start--) {
        if (isGuidedRoutePlayable(start, fields->m_guidedChainLength) && isGuidedRouteCompleted(start, fields->m_guidedChainLength)) {
            stage++;
        }
    }
    auto total = getGuidedPhaseStageCount();
    return std::clamp(stage + (stage < total ? 1 : 0), 1, total);
}

int HookPlayLayer::getGuidedPhaseStageCount() {
    auto count = 0;
    auto sectionCount = static_cast<int>(m_fields->m_startPosObjects.size() + 1);
    for (auto start = sectionCount - 1; start >= 0; start--) {
        if (isGuidedRoutePlayable(start, m_fields->m_guidedChainLength)) {
            count++;
        }
    }
    return std::max(1, count);
}

int HookPlayLayer::getGuidedStageIndex() {
    normalizeGuidedRoute();

    auto total = getGuidedStageCount();
    auto completed = getGuidedCompletedStageCount();
    return std::clamp(completed + (completed < total ? 1 : 0), 1, total);
}

int HookPlayLayer::getGuidedStageCount() {
    auto count = 0;
    auto sectionCount = static_cast<int>(m_fields->m_startPosObjects.size() + 1);
    for (auto phase = 1; phase <= getGuidedPhaseCount(); phase++) {
        for (auto start = sectionCount - 1; start >= 0; start--) {
            if (isGuidedRoutePlayable(start, phase)) {
                count++;
            }
        }
    }
    return std::max(1, count);
}

int HookPlayLayer::getGuidedRunTargetPercent() {
    auto fields = m_fields.self();
    normalizeGuidedRoute();

    auto routeLength = getGuidedRouteLength(fields->m_guidedWindowStart, fields->m_guidedChainLength);
    if (routeLength <= 0) {
        return 100;
    }

    auto endSection = fields->m_guidedWindowStart + routeLength - 1;
    return getLearnRClearTargetPercent(endSection);
}

bool HookPlayLayer::updateGuidedProgress(bool completed) {
    auto fields = m_fields.self();
    if (!ModManager::sharedState()->m_guidedMode || fields->m_startPosObjects.empty() || fields->m_ignoreProgressUntilRunBegin) {
        return false;
    }

    auto runStart = fields->m_activeRunStartIdx;
    auto sectionCount = static_cast<int>(fields->m_startPosObjects.size() + 1);
    if (runStart < 0 || runStart >= sectionCount) {
        return false;
    }

    normalizeGuidedRoute();

    auto percent = completed ? 100 : percentFromX(m_player1 ? m_player1->getPositionX() : 0.f, m_levelLength);
    auto changed = false;
    for (auto start = runStart; start < sectionCount; start++) {
        for (auto phase = 1; phase <= getGuidedPhaseCount(); phase++) {
            auto length = getGuidedRouteLength(start, phase);
            if (length <= 0 || isGuidedRouteCompleted(start, phase)) {
                continue;
            }

            auto target = getLearnRClearTargetPercent(start + length - 1);
            if (!completed && target >= 100) {
                continue;
            }
            if (percent >= target) {
                markGuidedRouteCompleted(start, phase);
                changed = true;
            }
        }
    }

    if (!changed) {
        return false;
    }

    fields->m_guidedRunCleared = true;
    fields->m_guidedStartPosPending = true;
    normalizeGuidedRoute();
    saveGuidedProgress();
    return true;
}

void HookPlayLayer::advanceGuidedRoute() {
    auto fields = m_fields.self();
    normalizeGuidedRoute();

    if (getGuidedStageCount() <= 1) {
        fields->m_guidedChainLength = 1;
        fields->m_guidedWindowStart = chooseGuidedStartForPhase(1, true);
        return;
    }

    auto nextStart = fields->m_guidedWindowStart - 1;
    while (
        nextStart >= 0 &&
        (!isGuidedRoutePlayable(nextStart, fields->m_guidedChainLength) || isGuidedRouteCompleted(nextStart, fields->m_guidedChainLength))
    ) {
        nextStart--;
    }
    if (nextStart >= 0) {
        fields->m_guidedWindowStart = nextStart;
        return;
    }

    while (fields->m_guidedChainLength < getGuidedPhaseCount()) {
        fields->m_guidedChainLength++;
        auto start = chooseGuidedStartForPhase(fields->m_guidedChainLength, true);
        if (start >= 0) {
            fields->m_guidedWindowStart = start;
            return;
        }
    }
}

int HookPlayLayer::chooseGuidedStartPos() {
    auto fields = m_fields.self();
    normalizeGuidedRoute();
    if (
        !isGuidedRoutePlayable(fields->m_guidedWindowStart, fields->m_guidedChainLength) ||
        isGuidedRouteCompleted(fields->m_guidedWindowStart, fields->m_guidedChainLength)
    ) {
        return fields->m_startPosIdx;
    }
    return fields->m_guidedWindowStart;
}

bool HookPlayLayer::applyGuidedStartPos(bool shouldReset, bool resetIfSame) {
    auto fields = m_fields.self();
    if (!ModManager::sharedState()->m_guidedMode || fields->m_guidedSwitching || fields->m_startPosObjects.empty()) {
        return false;
    }

    auto target = chooseGuidedStartPos();
    target = std::clamp(target, 0, static_cast<int>(fields->m_startPosObjects.size()));
    if (target == fields->m_startPosIdx && !resetIfSame) {
        return false;
    }

    fields->m_guidedSwitching = true;
    setLearnRStartPos(target, shouldReset);
    if (!shouldReset) {
        fields->m_activeRunStartIdx = target;
        fields->m_activeRunAttemptCounted = false;
        fields->m_guidedRunCleared = false;
        fields->m_ignoreProgressUntilRunBegin = true;
    }
    fields->m_guidedSwitching = false;
    return true;
}

void HookPlayLayer::loadGuidedProgress() {
    auto fields = m_fields.self();
    auto sectionCount = static_cast<int>(fields->m_startPosObjects.size() + 1);
    auto defaultStart = std::max(0, sectionCount - 1);
    auto mm = ModManager::sharedState();
    auto key = getLearnRSaveKey();

    auto it = mm->m_sessionGuidedProgress.find(key);
    if (it == mm->m_sessionGuidedProgress.end()) {
        fields->m_guidedChainLength = 1;
        fields->m_guidedWindowStart = defaultStart;
        fields->m_guidedCompletedRoutes.clear();
        fields->m_guidedAttemptRouteIDs.clear();
        fields->m_guidedAttemptRouteCounts.clear();
    } else {
        auto const& progress = it->second;
        fields->m_guidedChainLength = progress.m_chainLength;
        fields->m_guidedWindowStart = progress.m_windowStart;
        fields->m_guidedCompletedRoutes = progress.m_completedRoutes;
        fields->m_guidedAttemptRouteIDs = progress.m_attemptRouteIDs;
        fields->m_guidedAttemptRouteCounts = progress.m_attemptRouteCounts;
    }

    cleanGuidedRouteData();
    normalizeGuidedRoute();
}

void HookPlayLayer::saveGuidedProgress() {
    cleanGuidedRouteData();
    normalizeGuidedRoute();
    auto& progress = ModManager::sharedState()->m_sessionGuidedProgress[getLearnRSaveKey()];
    progress.m_chainLength = m_fields->m_guidedChainLength;
    progress.m_windowStart = m_fields->m_guidedWindowStart;
    progress.m_completedRoutes = m_fields->m_guidedCompletedRoutes;
    progress.m_attemptRouteIDs = m_fields->m_guidedAttemptRouteIDs;
    progress.m_attemptRouteCounts = m_fields->m_guidedAttemptRouteCounts;
}

std::string HookPlayLayer::getLearnRSaveKey() {
    if (!m_level) {
        return "learnr:unknown";
    }

    auto levelID = static_cast<int>(m_level->m_levelID);
    if (levelID > 0) {
        return fmt::format("learnr:id:{}", levelID);
    }

    auto levelName = std::string(m_level->m_levelName);
    auto levelString = std::string(m_level->m_levelString);
    return fmt::format("learnr:local:{}", stableHash(levelName + ":" + levelString));
}
