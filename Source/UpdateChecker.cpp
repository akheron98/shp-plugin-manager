#include "UpdateChecker.h"

namespace
{
constexpr const char* kRepo      = "akheron98/shp-plugin-manager";
constexpr const char* kUserAgent = "SHPPluginManager/0.1";

juce::String stripV (juce::String s)
{
    if (s.startsWithIgnoreCase ("v"))
        s = s.substring (1);
    return s.trim();
}

juce::String httpGet (const juce::URL& url, juce::String& outError)
{
    juce::String headers;
    headers << "User-Agent: " << kUserAgent << "\r\n";
    headers << "Accept: application/vnd.github+json\r\n";

    int statusCode = 0;
    auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
        .withExtraHeaders (headers)
        .withConnectionTimeoutMs (10000)
        .withStatusCode (&statusCode);

    auto stream = url.createInputStream (opts);
    if (stream == nullptr)
    {
        outError = "Could not open " + url.toString (false);
        return {};
    }
    if (statusCode >= 400)
    {
        outError = "HTTP " + juce::String (statusCode);
        return {};
    }
    return stream->readEntireStreamAsString();
}
}

int UpdateChecker::compareVersions (juce::String a, juce::String b)
{
    a = stripV (a);
    b = stripV (b);

    auto partsA = juce::StringArray::fromTokens (a, ".", "");
    auto partsB = juce::StringArray::fromTokens (b, ".", "");

    const int n = juce::jmax (partsA.size(), partsB.size());
    for (int i = 0; i < n; ++i)
    {
        const auto ai = i < partsA.size() ? partsA[i].getIntValue() : 0;
        const auto bi = i < partsB.size() ? partsB[i].getIntValue() : 0;
        if (ai != bi)
            return ai - bi;
    }
    return 0;
}

void UpdateChecker::check (juce::String currentVersion, ResultCallback onResult)
{
    pool.addJob ([currentVersion, cb = std::move (onResult)]() mutable
    {
        ManagerUpdateInfo info;

        const auto apiUrl = juce::String ("https://api.github.com/repos/") + kRepo + "/releases/latest";
        juce::String err;
        const auto body = httpGet (juce::URL (apiUrl), err);

        if (body.isEmpty())
        {
            info.error = err.isNotEmpty() ? err : "No release found";
        }
        else
        {
            auto root = juce::JSON::parse (body);
            const auto tag = root.getProperty ("tag_name", {}).toString();
            info.latestVersion = stripV (tag);

            if (info.latestVersion.isNotEmpty()
                && compareVersions (info.latestVersion, currentVersion) > 0)
            {
                info.updateAvailable = true;

                if (auto* assets = root.getProperty ("assets", {}).getArray())
                {
                    for (auto& a : *assets)
                    {
                        const auto name = a.getProperty ("name", {}).toString();
                        if (name.startsWithIgnoreCase ("SHPPluginManager-Setup-")
                            && name.endsWithIgnoreCase (".exe"))
                        {
                            info.installerUrl      = a.getProperty ("browser_download_url", {}).toString();
                            info.installerFileName = name;
                            break;
                        }
                    }
                }

                if (info.installerUrl.isEmpty())
                    info.error = "Latest release has no installer asset";
            }
        }

        juce::MessageManager::callAsync ([cb = std::move (cb), info = std::move (info)]() mutable
        {
            cb (std::move (info));
        });
    });
}

void UpdateChecker::downloadAndRunInstaller (const ManagerUpdateInfo& info,
                                             std::function<void (juce::String)> onError)
{
    if (info.installerUrl.isEmpty())
    {
        if (onError) onError ("No installer URL");
        return;
    }

    juce::Thread::launch ([info, onError = std::move (onError)]
    {
        const auto tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory)
            .getChildFile ("shp-manager-update");
        tempDir.createDirectory();

        const auto target = tempDir.getChildFile (info.installerFileName);
        target.deleteFile();

        juce::String headers;
        headers << "User-Agent: " << kUserAgent << "\r\n";
        headers << "Accept: application/octet-stream\r\n";

        int statusCode = 0;
        auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders (headers)
            .withConnectionTimeoutMs (30000)
            .withStatusCode (&statusCode)
            .withNumRedirectsToFollow (5);

        auto stream = juce::URL (info.installerUrl).createInputStream (opts);
        if (stream == nullptr || statusCode >= 400)
        {
            juce::MessageManager::callAsync ([onError]
            {
                if (onError) onError ("Download failed");
            });
            return;
        }

        juce::FileOutputStream out (target);
        if (! out.openedOk())
        {
            juce::MessageManager::callAsync ([onError]
            {
                if (onError) onError ("Cannot write installer to TEMP");
            });
            return;
        }

        constexpr int chunk = 64 * 1024;
        juce::HeapBlock<char> buf (chunk);
        while (! stream->isExhausted())
        {
            const auto n = stream->read (buf, chunk);
            if (n <= 0) break;
            out.write (buf, (size_t) n);
        }
        out.flush();

        juce::MessageManager::callAsync ([target]
        {
            // Launch installer (per-user, no UAC), then quit so it can replace files.
            target.startAsProcess();
            juce::JUCEApplicationBase::quit();
        });
    });
}
