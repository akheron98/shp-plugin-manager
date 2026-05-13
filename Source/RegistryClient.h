#pragma once

#include <JuceHeader.h>

struct RegistryPlugin
{
    juce::String id;
    juce::String slug;
    juce::String name;
    juce::String category;
    juce::String githubRepo;        // "owner/repo" — may be shared across plugins (e.g. shp-builds)
    juce::String tagPrefix;         // e.g. "vocalstrip-v"; empty = use /releases/latest
    juce::String assetPattern;      // e.g. "shp-vocalstrip-{version}-win64.zip"
    juce::String vst3BundleName;    // e.g. "SHP Vocal Strip.vst3"
    juce::String description;
    juce::String iconUrl;

    // Resolved from GitHub Releases API
    juce::String latestVersion;     // tag without prefix; empty if no release
    juce::String latestAssetUrl;    // direct download URL of the matched .zip; empty if none
    juce::String releaseError;      // human-readable error message (e.g. rate limit, no release)
};

struct RegistryFetchResult
{
    bool manifestOk { false };
    juce::String manifestError;     // if manifestOk is false
    std::vector<RegistryPlugin> plugins;
};

class RegistryClient
{
public:
    using FetchCallback = std::function<void (RegistryFetchResult)>;

    RegistryClient();
    ~RegistryClient();

    // Starts an async fetch. Callback runs on the JUCE message thread.
    void fetch (FetchCallback callback);

    void setManifestUrl (juce::String url) { manifestUrl = std::move (url); }
    void setGithubToken (juce::String token) { githubToken = std::move (token); }

private:
    static RegistryFetchResult fetchBlocking (juce::String manifestUrl,
                                              juce::String githubToken);

    static juce::String httpGet (const juce::URL& url,
                                 const juce::String& githubToken,
                                 juce::String& outError);

    static bool parseManifest (const juce::String& json,
                               std::vector<RegistryPlugin>& outPlugins,
                               juce::String& outError);

    static void resolveLatestRelease (RegistryPlugin& plugin,
                                      const juce::String& githubToken);

    juce::String manifestUrl
        { "https://raw.githubusercontent.com/akheron98/shp-plugin-registry/main/manifest.json" };
    juce::String githubToken;

    juce::ThreadPool pool { 1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RegistryClient)
};
