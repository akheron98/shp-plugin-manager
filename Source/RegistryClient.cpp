#include "RegistryClient.h"

namespace
{
constexpr const char* kUserAgent = "SHPPluginManager/0.1";

juce::String stripVersionPrefix (juce::String tag)
{
    if (tag.startsWithIgnoreCase ("v"))
        tag = tag.substring (1);
    return tag.trim();
}

juce::String expandPattern (const juce::String& pattern, const juce::String& version)
{
    return pattern.replace ("{version}", version);
}
}

RegistryClient::RegistryClient() = default;
RegistryClient::~RegistryClient() = default;

void RegistryClient::fetch (FetchCallback callback)
{
    auto manifestUrlCopy = manifestUrl;
    auto tokenCopy = githubToken;

    pool.addJob ([manifestUrlCopy, tokenCopy, cb = std::move (callback)]() mutable
    {
        auto result = fetchBlocking (manifestUrlCopy, tokenCopy);

        juce::MessageManager::callAsync ([cb = std::move (cb), result = std::move (result)]() mutable
        {
            cb (std::move (result));
        });
    });
}

RegistryFetchResult RegistryClient::fetchBlocking (juce::String manifestUrl,
                                                   juce::String githubToken)
{
    RegistryFetchResult result;

    juce::String fetchError;
    const auto manifestJson = httpGet (juce::URL (manifestUrl), githubToken, fetchError);

    if (manifestJson.isEmpty())
    {
        result.manifestError = fetchError.isNotEmpty() ? fetchError
                                                       : "Empty manifest response";
        return result;
    }

    juce::String parseError;
    if (! parseManifest (manifestJson, result.plugins, parseError))
    {
        result.manifestError = parseError;
        return result;
    }

    result.manifestOk = true;

    for (auto& plugin : result.plugins)
        resolveLatestRelease (plugin, githubToken);

    return result;
}

juce::String RegistryClient::httpGet (const juce::URL& url,
                                      const juce::String& githubToken,
                                      juce::String& outError)
{
    juce::String headers;
    headers << "User-Agent: " << kUserAgent << "\r\n";
    headers << "Accept: application/vnd.github+json\r\n";
    if (githubToken.isNotEmpty())
        headers << "Authorization: Bearer " << githubToken << "\r\n";

    int statusCode = 0;
    juce::StringPairArray responseHeaders;

    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
        .withExtraHeaders (headers)
        .withConnectionTimeoutMs (15000)
        .withStatusCode (&statusCode)
        .withResponseHeaders (&responseHeaders);

    auto stream = url.createInputStream (options);

    if (stream == nullptr)
    {
        outError = "Network error: could not open " + url.toString (false);
        return {};
    }

    if (statusCode >= 400)
    {
        outError = "HTTP " + juce::String (statusCode) + " for " + url.toString (false);
        return {};
    }

    return stream->readEntireStreamAsString();
}

bool RegistryClient::parseManifest (const juce::String& json,
                                    std::vector<RegistryPlugin>& outPlugins,
                                    juce::String& outError)
{
    auto root = juce::JSON::parse (json);
    if (! root.isObject())
    {
        outError = "Manifest root is not a JSON object";
        return false;
    }

    auto* plugins = root.getProperty ("plugins", {}).getArray();
    if (plugins == nullptr)
    {
        outError = "Manifest missing 'plugins' array";
        return false;
    }

    outPlugins.clear();
    outPlugins.reserve ((size_t) plugins->size());

    for (auto& v : *plugins)
    {
        RegistryPlugin p;
        p.id             = v.getProperty ("id", {}).toString();
        p.slug           = v.getProperty ("slug", {}).toString();
        p.name           = v.getProperty ("name", {}).toString();
        p.category       = v.getProperty ("category", {}).toString();
        p.githubRepo     = v.getProperty ("github_repo", {}).toString();
        p.tagPrefix      = v.getProperty ("tag_prefix", {}).toString();
        p.assetPattern   = v.getProperty ("asset_pattern", {}).toString();
        p.vst3BundleName = v.getProperty ("vst3_bundle_name", {}).toString();
        p.description    = v.getProperty ("description", {}).toString();
        p.iconUrl        = v.getProperty ("icon_url", {}).toString();
        p.manualUrl      = v.getProperty ("manual_url", {}).toString();

        if (p.id.isNotEmpty() && p.githubRepo.isNotEmpty())
            outPlugins.push_back (std::move (p));
    }

    return true;
}

void RegistryClient::resolveLatestRelease (RegistryPlugin& plugin,
                                           const juce::String& githubToken)
{
    // Branch 1 — namespaced multi-plugin repo (e.g. shp-builds): list releases and
    // pick the first one whose tag starts with our prefix.
    if (plugin.tagPrefix.isNotEmpty())
    {
        const auto listUrl = "https://api.github.com/repos/" + plugin.githubRepo
                           + "/releases?per_page=100";

        juce::String err;
        const auto body = httpGet (juce::URL (listUrl), githubToken, err);
        if (body.isEmpty())
        {
            plugin.releaseError = err.isNotEmpty() ? err : "No release available";
            return;
        }

        auto root = juce::JSON::parse (body);
        auto* arr = root.getArray();
        if (arr == nullptr)
        {
            plugin.releaseError = "Invalid releases JSON";
            return;
        }

        // /releases is ordered by created_at desc, so the first match is the latest.
        for (auto& rel : *arr)
        {
            const auto tag = rel.getProperty ("tag_name", {}).toString();
            if (! tag.startsWith (plugin.tagPrefix))
                continue;

            plugin.latestVersion = tag.substring (plugin.tagPrefix.length()).trim();
            const auto expected = expandPattern (plugin.assetPattern, plugin.latestVersion);

            if (auto* assets = rel.getProperty ("assets", {}).getArray())
            {
                for (auto& a : *assets)
                {
                    const auto name = a.getProperty ("name", {}).toString();
                    if (name.equalsIgnoreCase (expected))
                    {
                        plugin.latestAssetUrl = a.getProperty ("browser_download_url", {}).toString();
                        break;
                    }
                }
            }

            if (plugin.latestAssetUrl.isEmpty())
                plugin.releaseError = "No asset matching " + expected;
            return;
        }

        plugin.releaseError = "No release found with prefix " + plugin.tagPrefix;
        return;
    }

    // Branch 2 — legacy single-plugin repo: use /releases/latest.
    const auto apiUrl = "https://api.github.com/repos/" + plugin.githubRepo + "/releases/latest";

    juce::String err;
    const auto body = httpGet (juce::URL (apiUrl), githubToken, err);

    if (body.isEmpty())
    {
        plugin.releaseError = err.isNotEmpty() ? err : "No release available";
        return;
    }

    auto root = juce::JSON::parse (body);
    if (! root.isObject())
    {
        plugin.releaseError = "Invalid release JSON";
        return;
    }

    const auto tag = root.getProperty ("tag_name", {}).toString();
    plugin.latestVersion = stripVersionPrefix (tag);
    if (plugin.latestVersion.isEmpty())
    {
        plugin.releaseError = "Release has no tag_name";
        return;
    }

    const auto expected = expandPattern (plugin.assetPattern, plugin.latestVersion);

    if (auto* assets = root.getProperty ("assets", {}).getArray())
    {
        for (auto& a : *assets)
        {
            const auto name = a.getProperty ("name", {}).toString();
            if (name.equalsIgnoreCase (expected))
            {
                plugin.latestAssetUrl = a.getProperty ("browser_download_url", {}).toString();
                break;
            }
        }
    }

    if (plugin.latestAssetUrl.isEmpty())
        plugin.releaseError = "No asset matching " + expected;
}
