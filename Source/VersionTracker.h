#pragma once

#include <JuceHeader.h>

struct InstalledPlugin
{
    juce::String id;
    juce::String version;
    juce::String installPath;       // full path to the .vst3 folder bundle
    juce::String installedAt;       // ISO 8601 timestamp
};

class VersionTracker
{
public:
    VersionTracker();

    // Reloads from disk and prunes entries whose bundle is missing.
    void refresh();

    juce::Optional<InstalledPlugin> get (const juce::String& id) const;

    void recordInstall (const juce::String& id,
                        const juce::String& version,
                        const juce::File&   bundlePath);

    void recordUninstall (const juce::String& id);

    static juce::File getStateFile();

private:
    void load();
    void save() const;

    std::map<juce::String, InstalledPlugin> installs;
};
