#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class ModManager {
public:
    struct LevelSettings {
        bool m_guidedMode = false;
        bool m_showStartposSwitcher = true;
        bool m_showGuidedPercent = true;
        bool m_smartStartpos = false;
        int m_guidedLateThreshold = 50;
        int m_guidedAttemptLimit = 20;
    };

    struct GuidedProgress {
        int m_chainLength = 1;
        int m_windowStart = -1;
        std::vector<int> m_completedRoutes = {};
        std::vector<int> m_attemptRouteIDs = {};
        std::vector<int> m_attemptRouteCounts = {};
    };

    static ModManager* sharedState();

    ModManager();
    void resetLevelSettingsToDefaults();
    void loadLevelSettings(std::string const& key);
    void saveLevelSettings();
    void saveLevelSettings(std::string const& key);

    bool m_dontFadeOnStart = false;
    bool m_hideBtns = false;
    bool m_ignoreDisabled = false;
    bool m_guidedMode = false;
    bool m_showStartposSwitcher = true;
    bool m_showGuidedPercent = true;
    bool m_smartStartpos = false;
    int m_guidedLateThreshold = 50;
    int m_guidedAttemptLimit = 20;
    double m_opacity = 0;
    std::string m_activeLevelSettingsKey = "";
    std::unordered_map<std::string, GuidedProgress> m_sessionGuidedProgress = {};
};
