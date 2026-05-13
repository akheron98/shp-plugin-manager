#pragma once

#include <JuceHeader.h>
#include "RegistryClient.h"

class Settings;
class VersionTracker;

struct InstallRequest
{
    RegistryPlugin plugin;       // contains latestVersion, latestAssetUrl, vst3BundleName, id, name
};

struct InstallProgress
{
    enum class Stage
    {
        queued,
        downloading,
        extracting,
        copying,
        done,
        failed
    };

    juce::String pluginId;
    Stage        stage      { Stage::queued };
    double       fraction   { 0.0 };          // 0..1, only meaningful in downloading
    juce::String message;                     // human-readable status
    juce::String errorDetail;                 // populated when Stage::failed
};

class InstallManager
{
public:
    using ProgressCallback = std::function<void (InstallProgress)>;

    InstallManager (Settings& settings, VersionTracker& tracker);
    ~InstallManager();

    void install   (RegistryPlugin plugin, ProgressCallback onProgress);
    void uninstall (const juce::String& pluginId,
                    const juce::String& vst3BundleName,
                    ProgressCallback onProgress);

private:
    void runInstallJob (RegistryPlugin plugin, ProgressCallback onProgress);

    static juce::File downloadAsset (const juce::URL& assetUrl,
                                     const juce::File& targetFile,
                                     const juce::String& githubToken,
                                     std::function<void (double)> onFraction,
                                     juce::String& outError);

    static juce::File findSingleVst3Bundle (const juce::File& dir);

    void post (ProgressCallback& cb, InstallProgress p);

    Settings&       settings;
    VersionTracker& tracker;
    juce::ThreadPool pool { 1 };
};
