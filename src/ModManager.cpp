#include "ModManager.hpp"
#include <Geode/Geode.hpp>

#include <algorithm>

using namespace geode::prelude;

ModManager* ModManager::sharedState() {
    static ModManager instance;
    return &instance;
}

ModManager::ModManager() {
    m_dontFadeOnStart = Mod::get()->getSettingValue<bool>("hide");
    m_hideBtns = Mod::get()->getSettingValue<bool>("hideBtns");
    m_ignoreDisabled = Mod::get()->getSettingValue<bool>("ignoreDisabled");
    resetLevelSettingsToDefaults();
    m_opacity = Mod::get()->getSettingValue<double>("opacity") / 100 * 255;
}

void ModManager::resetLevelSettingsToDefaults() {
    auto defaults = LevelSettings();
    m_guidedMode = defaults.m_guidedMode;
    m_showStartposSwitcher = defaults.m_showStartposSwitcher;
    m_showGuidedPercent = defaults.m_showGuidedPercent;
    m_smartStartpos = defaults.m_smartStartpos;
    m_guidedLateThreshold = defaults.m_guidedLateThreshold;
    m_guidedAttemptLimit = defaults.m_guidedAttemptLimit;
}

void ModManager::loadLevelSettings(std::string const& key) {
    m_activeLevelSettingsKey = key;
    auto defaults = LevelSettings();
    auto mod = Mod::get();

    m_guidedMode = mod->getSavedValue<bool>(key + ":settings:guided-mode", defaults.m_guidedMode);
    m_showStartposSwitcher = mod->getSavedValue<bool>(key + ":settings:show-startpos-switcher", defaults.m_showStartposSwitcher);
    m_showGuidedPercent = mod->getSavedValue<bool>(key + ":settings:show-guided-percent", defaults.m_showGuidedPercent);
    m_smartStartpos = mod->getSavedValue<bool>(key + ":settings:smart-startpos", defaults.m_smartStartpos);
    m_guidedLateThreshold = std::clamp(
        mod->getSavedValue<int>(key + ":settings:guided-late-threshold", defaults.m_guidedLateThreshold),
        0,
        100
    );
    m_guidedAttemptLimit = std::clamp(
        mod->getSavedValue<int>(key + ":settings:guided-attempt-limit", defaults.m_guidedAttemptLimit),
        1,
        999
    );
}

void ModManager::saveLevelSettings() {
    if (m_activeLevelSettingsKey.empty()) {
        return;
    }
    saveLevelSettings(m_activeLevelSettingsKey);
}

void ModManager::saveLevelSettings(std::string const& key) {
    if (key.empty()) {
        return;
    }

    m_activeLevelSettingsKey = key;
    auto mod = Mod::get();
    mod->setSavedValue(key + ":settings:guided-mode", m_guidedMode);
    mod->setSavedValue(key + ":settings:show-startpos-switcher", m_showStartposSwitcher);
    mod->setSavedValue(key + ":settings:show-guided-percent", m_showGuidedPercent);
    mod->setSavedValue(key + ":settings:smart-startpos", m_smartStartpos);
    mod->setSavedValue(key + ":settings:guided-late-threshold", m_guidedLateThreshold);
    mod->setSavedValue(key + ":settings:guided-attempt-limit", m_guidedAttemptLimit);
}

$on_mod(Loaded) {
    auto mm = ModManager::sharedState();
    
    listenForSettingChanges<bool>("hide", [mm](bool val) {
        mm->m_dontFadeOnStart = val;
    });
    
    listenForSettingChanges<bool>("hideBtns", [mm](bool val) {
        mm->m_hideBtns = val;
    });
    
    listenForSettingChanges<bool>("ignoreDisabled", [mm](bool val) {
        mm->m_ignoreDisabled = val;
    });
    
    listenForSettingChanges<double>("opacity", [mm](double val) {
        mm->m_opacity = val / 100 * 255;
    });
}
