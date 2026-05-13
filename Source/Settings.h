#pragma once

#include <JuceHeader.h>

class Settings
{
public:
    enum class InstallScope
    {
        userLocal,    // %LOCALAPPDATA%\Programs\Common\VST3
        systemWide,   // C:\Program Files\Common Files\VST3
        custom        // user-provided path
    };

    Settings();

    void save();

    InstallScope getInstallScope() const   { return installScope; }
    juce::File   getInstallPath() const;   // resolves the scope to an actual path
    juce::String getGithubToken() const    { return githubToken; }
    juce::String getRegistryUrl() const    { return registryUrl; }
    juce::File   getCustomInstallPath() const { return customPath; }

    void setInstallScope (InstallScope s)         { installScope = s; }
    void setCustomInstallPath (juce::File f)      { customPath = std::move (f); }
    void setGithubToken (juce::String t)          { githubToken = std::move (t); }
    void setRegistryUrl (juce::String u)          { registryUrl = std::move (u); }

    static juce::File getUserLocalVst3Dir();
    static juce::File getSystemVst3Dir();
    static juce::File getStateDir();   // %APPDATA%\SHP\manager\

    // True if the install path requires admin elevation to write to.
    bool installPathRequiresElevation() const;

private:
    void load();

    juce::PropertiesFile::Options makeOptions() const;

    InstallScope installScope { InstallScope::userLocal };
    juce::File   customPath;
    juce::String githubToken;
    juce::String registryUrl
        { "https://raw.githubusercontent.com/akheron98/shp-plugin-registry/main/manifest.json" };
};
