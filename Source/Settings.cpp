#include "Settings.h"

namespace
{
constexpr const char* kKeyScope        = "installScope";
constexpr const char* kKeyCustomPath   = "customInstallPath";
constexpr const char* kKeyGithubToken  = "githubToken";
constexpr const char* kKeyRegistryUrl  = "registryUrl";
}

Settings::Settings()
{
    load();
}

juce::PropertiesFile::Options Settings::makeOptions() const
{
    juce::PropertiesFile::Options o;
    o.applicationName     = "SHPPluginManager";
    o.folderName          = "SHP" + juce::File::getSeparatorString() + "manager";
    o.filenameSuffix      = ".settings";
    o.commonToAllUsers    = false;
    o.osxLibrarySubFolder = "Application Support";
    return o;
}

void Settings::load()
{
    juce::PropertiesFile props (makeOptions());

    const auto s = props.getIntValue (kKeyScope, (int) InstallScope::userLocal);
    installScope = (InstallScope) juce::jlimit (0, 2, s);

    const auto cp = props.getValue (kKeyCustomPath, {});
    if (cp.isNotEmpty())
        customPath = juce::File (cp);

    githubToken = props.getValue (kKeyGithubToken, {});

    const auto rurl = props.getValue (kKeyRegistryUrl, registryUrl);
    if (rurl.isNotEmpty())
        registryUrl = rurl;
}

void Settings::save()
{
    juce::PropertiesFile props (makeOptions());
    props.setValue (kKeyScope, (int) installScope);
    props.setValue (kKeyCustomPath, customPath.getFullPathName());
    props.setValue (kKeyGithubToken, githubToken);
    props.setValue (kKeyRegistryUrl, registryUrl);
    props.saveIfNeeded();
}

juce::File Settings::getUserLocalVst3Dir()
{
   #if JUCE_WINDOWS
    auto localAppData = juce::SystemStats::getEnvironmentVariable ("LOCALAPPDATA", {});
    if (localAppData.isEmpty())
        localAppData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getFullPathName();
    return juce::File (localAppData).getChildFile ("Programs").getChildFile ("Common").getChildFile ("VST3");
   #else
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
        .getChildFile (".vst3");
   #endif
}

juce::File Settings::getSystemVst3Dir()
{
   #if JUCE_WINDOWS
    auto programFiles = juce::SystemStats::getEnvironmentVariable ("ProgramFiles", "C:\\Program Files");
    return juce::File (programFiles).getChildFile ("Common Files").getChildFile ("VST3");
   #else
    return juce::File ("/Library/Audio/Plug-Ins/VST3");
   #endif
}

juce::File Settings::getStateDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("SHP")
        .getChildFile ("manager");
}

juce::File Settings::getInstallPath() const
{
    switch (installScope)
    {
        case InstallScope::userLocal:  return getUserLocalVst3Dir();
        case InstallScope::systemWide: return getSystemVst3Dir();
        case InstallScope::custom:     return customPath;
    }
    return getUserLocalVst3Dir();
}

bool Settings::installPathRequiresElevation() const
{
   #if JUCE_WINDOWS
    const auto path = getInstallPath().getFullPathName();
    const auto pf  = juce::SystemStats::getEnvironmentVariable ("ProgramFiles",     "C:\\Program Files");
    const auto pf86 = juce::SystemStats::getEnvironmentVariable ("ProgramFiles(x86)", "C:\\Program Files (x86)");
    return path.startsWithIgnoreCase (pf) || path.startsWithIgnoreCase (pf86);
   #else
    return false;
   #endif
}
