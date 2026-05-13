#include "InstallManager.h"
#include "Settings.h"
#include "VersionTracker.h"
#include "ElevationHelper.h"

namespace
{
constexpr const char* kUserAgent = "SHPPluginManager/0.1";

juce::File tempWorkDir()
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getChildFile ("shp-manager");
}

juce::String sanitize (juce::String s)
{
    return s.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.");
}
}

InstallManager::InstallManager (Settings& s, VersionTracker& t)
    : settings (s), tracker (t)
{
}

InstallManager::~InstallManager() = default;

void InstallManager::post (ProgressCallback& cb, InstallProgress p)
{
    if (! cb)
        return;
    juce::MessageManager::callAsync ([cb, p = std::move (p)]() mutable { cb (std::move (p)); });
}

void InstallManager::install (RegistryPlugin plugin, ProgressCallback onProgress)
{
    pool.addJob ([this, plugin = std::move (plugin), onProgress = std::move (onProgress)]() mutable
    {
        runInstallJob (std::move (plugin), std::move (onProgress));
    });
}

void InstallManager::uninstall (const juce::String& pluginId,
                                const juce::String& vst3BundleName,
                                ProgressCallback onProgress)
{
    pool.addJob ([this, pluginId, vst3BundleName, onProgress = std::move (onProgress)]() mutable
    {
        InstallProgress p; p.pluginId = pluginId; p.stage = InstallProgress::Stage::copying;
        p.message = "Removing bundle…";
        post (onProgress, p);

        bool ok = true;

        if (auto entry = tracker.get (pluginId))
        {
            juce::File bundle (entry->installPath);
            if (bundle.isDirectory())
                ok = bundle.deleteRecursively();
        }
        else
        {
            // fallback: derive path from current settings + bundle name
            auto bundle = settings.getInstallPath().getChildFile (vst3BundleName);
            if (bundle.isDirectory())
                ok = bundle.deleteRecursively();
        }

        tracker.recordUninstall (pluginId);

        InstallProgress done; done.pluginId = pluginId;
        done.stage = ok ? InstallProgress::Stage::done : InstallProgress::Stage::failed;
        done.message = ok ? "Uninstalled." : "Could not delete bundle.";
        if (! ok) done.errorDetail = done.message;
        post (onProgress, done);
    });
}

void InstallManager::runInstallJob (RegistryPlugin plugin, ProgressCallback onProgress)
{
    auto postStage = [&] (InstallProgress::Stage s, juce::String msg, double frac = 0.0)
    {
        InstallProgress p;
        p.pluginId = plugin.id;
        p.stage    = s;
        p.message  = std::move (msg);
        p.fraction = frac;
        post (onProgress, p);
    };

    auto fail = [&] (juce::String message, juce::String detail = {})
    {
        InstallProgress p;
        p.pluginId    = plugin.id;
        p.stage       = InstallProgress::Stage::failed;
        p.message     = std::move (message);
        p.errorDetail = std::move (detail);
        post (onProgress, p);
    };

    if (plugin.latestAssetUrl.isEmpty())
    {
        fail ("No downloadable asset for this release.");
        return;
    }

    const auto workDir = tempWorkDir().getChildFile (sanitize (plugin.slug + "-" + plugin.latestVersion));
    workDir.deleteRecursively();
    workDir.createDirectory();

    const auto zipFile = workDir.getChildFile (sanitize (plugin.slug + "-" + plugin.latestVersion + ".zip"));
    const auto extractDir = workDir.getChildFile ("extracted");

    // ── 1. Download ─────────────────────────────────────────────────────────
    postStage (InstallProgress::Stage::downloading, "Downloading…", 0.0);

    juce::String dlError;
    auto downloaded = downloadAsset (juce::URL (plugin.latestAssetUrl),
                                     zipFile,
                                     settings.getGithubToken(),
                                     [&] (double f)
                                     {
                                         InstallProgress p;
                                         p.pluginId = plugin.id;
                                         p.stage    = InstallProgress::Stage::downloading;
                                         p.fraction = f;
                                         p.message  = "Downloading… " + juce::String (juce::roundToInt (f * 100.0)) + "%";
                                         post (onProgress, p);
                                     },
                                     dlError);

    if (! downloaded.existsAsFile())
    {
        fail ("Download failed.", dlError);
        return;
    }

    // ── 2. Extract ──────────────────────────────────────────────────────────
    postStage (InstallProgress::Stage::extracting, "Extracting…");

    extractDir.createDirectory();
    juce::ZipFile zf (downloaded);
    auto extractResult = zf.uncompressTo (extractDir, true);
    if (extractResult.failed())
    {
        fail ("Extraction failed.", extractResult.getErrorMessage());
        return;
    }

    auto extractedBundle = findSingleVst3Bundle (extractDir);
    if (! extractedBundle.isDirectory())
    {
        fail ("Bundle not found in archive.",
              "No single .vst3 folder at the top level of "
                  + downloaded.getFileName());
        return;
    }

    if (! extractedBundle.getFileName().equalsIgnoreCase (plugin.vst3BundleName))
    {
        // not fatal — registry's vst3_bundle_name vs actual archive name mismatch.
        juce::Logger::writeToLog ("Note: archive bundle '" + extractedBundle.getFileName()
            + "' differs from manifest '" + plugin.vst3BundleName + "'. Using archive name.");
    }

    // ── 3. Copy / install ───────────────────────────────────────────────────
    postStage (InstallProgress::Stage::copying, "Installing…");

    const auto destBundle = settings.getInstallPath().getChildFile (plugin.vst3BundleName);

    bool installed = false;
    if (settings.installPathRequiresElevation())
    {
        installed = ElevationHelper::runElevatedCopy (extractedBundle, destBundle);
        if (! installed)
        {
            fail ("Elevated install failed (UAC denied or copy error).");
            return;
        }
    }
    else
    {
        if (destBundle.exists())
        {
            if (! destBundle.deleteRecursively())
            {
                fail ("Could not remove existing bundle.",
                      "DAW may still have it loaded: " + destBundle.getFullPathName());
                return;
            }
        }
        destBundle.getParentDirectory().createDirectory();
        installed = extractedBundle.copyDirectoryTo (destBundle);
        if (! installed)
        {
            fail ("Copy failed.", "Target: " + destBundle.getFullPathName());
            return;
        }
    }

    tracker.recordInstall (plugin.id, plugin.latestVersion, destBundle);

    workDir.deleteRecursively();

    InstallProgress done;
    done.pluginId = plugin.id;
    done.stage    = InstallProgress::Stage::done;
    done.message  = "Installed v" + plugin.latestVersion;
    post (onProgress, done);
}

juce::File InstallManager::downloadAsset (const juce::URL& assetUrl,
                                          const juce::File& targetFile,
                                          const juce::String& githubToken,
                                          std::function<void (double)> onFraction,
                                          juce::String& outError)
{
    juce::String headers;
    headers << "User-Agent: " << kUserAgent << "\r\n";
    headers << "Accept: application/octet-stream\r\n";
    if (githubToken.isNotEmpty())
        headers << "Authorization: Bearer " << githubToken << "\r\n";

    int statusCode = 0;
    juce::StringPairArray responseHeaders;

    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
        .withExtraHeaders (headers)
        .withConnectionTimeoutMs (30000)
        .withStatusCode (&statusCode)
        .withResponseHeaders (&responseHeaders)
        .withNumRedirectsToFollow (5);

    auto stream = assetUrl.createInputStream (options);
    if (stream == nullptr)
    {
        outError = "Could not open " + assetUrl.toString (false);
        return {};
    }

    if (statusCode >= 400)
    {
        outError = "HTTP " + juce::String (statusCode);
        return {};
    }

    const auto total = stream->getTotalLength();

    targetFile.deleteFile();
    juce::FileOutputStream out (targetFile);
    if (! out.openedOk())
    {
        outError = "Cannot write to " + targetFile.getFullPathName();
        return {};
    }

    constexpr int chunkSize = 64 * 1024;
    juce::HeapBlock<char> buf (chunkSize);
    juce::int64 totalRead = 0;

    while (! stream->isExhausted())
    {
        const auto numRead = stream->read (buf, chunkSize);
        if (numRead <= 0)
            break;
        out.write (buf, (size_t) numRead);
        totalRead += numRead;

        if (total > 0 && onFraction)
            onFraction ((double) totalRead / (double) total);
    }
    out.flush();

    if (totalRead == 0)
    {
        outError = "Empty response body";
        return {};
    }

    return targetFile;
}

juce::File InstallManager::findSingleVst3Bundle (const juce::File& dir)
{
    juce::Array<juce::File> matches;
    dir.findChildFiles (matches, juce::File::findDirectories, false, "*.vst3");
    if (matches.size() == 1)
        return matches[0];

    // GitHub Actions Compress-Archive sometimes wraps content in an extra folder layer.
    // Recurse one level deep.
    juce::Array<juce::File> subDirs;
    dir.findChildFiles (subDirs, juce::File::findDirectories, false);
    for (auto& sub : subDirs)
    {
        juce::Array<juce::File> inner;
        sub.findChildFiles (inner, juce::File::findDirectories, false, "*.vst3");
        if (inner.size() == 1)
            return inner[0];
    }

    return {};
}
