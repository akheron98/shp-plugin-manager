#pragma once

#include <JuceHeader.h>

struct ManagerUpdateInfo
{
    bool         updateAvailable { false };
    juce::String latestVersion;            // e.g. "0.1.1"
    juce::String installerUrl;             // direct .exe download
    juce::String installerFileName;        // e.g. "SHPPluginManager-Setup-v0.1.1.exe"
    juce::String error;                    // populated on failure
};

class UpdateChecker
{
public:
    using ResultCallback = std::function<void (ManagerUpdateInfo)>;

    void check (juce::String currentVersion, ResultCallback onResult);

    // Downloads the installer into %TEMP%, spawns it, then quits the app.
    static void downloadAndRunInstaller (const ManagerUpdateInfo& info,
                                         std::function<void (juce::String)> onError);

    // Returns positive if `a` > `b`, 0 if equal, negative if `a` < `b`.
    // Pads missing parts with 0 (so "0.1" == "0.1.0").
    static int compareVersions (juce::String a, juce::String b);

private:
    juce::ThreadPool pool { 1 };
};
