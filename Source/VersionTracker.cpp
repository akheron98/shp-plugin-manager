#include "VersionTracker.h"
#include "Settings.h"

namespace
{
juce::String nowIso()
{
    return juce::Time::getCurrentTime().toISO8601 (true);
}
}

VersionTracker::VersionTracker()
{
    load();
}

juce::File VersionTracker::getStateFile()
{
    return Settings::getStateDir().getChildFile ("state.json");
}

void VersionTracker::load()
{
    installs.clear();

    auto file = getStateFile();
    if (! file.existsAsFile())
        return;

    auto root = juce::JSON::parse (file);
    if (! root.isObject())
        return;

    if (auto* arr = root.getProperty ("plugins", {}).getArray())
    {
        for (auto& v : *arr)
        {
            InstalledPlugin p;
            p.id          = v.getProperty ("id", {}).toString();
            p.version     = v.getProperty ("version", {}).toString();
            p.installPath = v.getProperty ("installPath", {}).toString();
            p.installedAt = v.getProperty ("installedAt", {}).toString();

            if (p.id.isNotEmpty())
                installs[p.id] = std::move (p);
        }
    }
}

void VersionTracker::save() const
{
    auto file = getStateFile();
    file.getParentDirectory().createDirectory();

    juce::Array<juce::var> arr;
    for (auto& [id, p] : installs)
    {
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        o->setProperty ("id",          p.id);
        o->setProperty ("version",     p.version);
        o->setProperty ("installPath", p.installPath);
        o->setProperty ("installedAt", p.installedAt);
        arr.add (juce::var (o.get()));
    }

    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ("schema_version", 1);
    root->setProperty ("plugins",        std::move (arr));

    file.replaceWithText (juce::JSON::toString (juce::var (root.get()), true));
}

void VersionTracker::refresh()
{
    load();

    // Prune entries whose bundle is missing.
    bool changed = false;
    for (auto it = installs.begin(); it != installs.end(); )
    {
        if (! juce::File (it->second.installPath).isDirectory())
        {
            it = installs.erase (it);
            changed = true;
        }
        else
        {
            ++it;
        }
    }

    if (changed)
        save();
}

juce::Optional<InstalledPlugin> VersionTracker::get (const juce::String& id) const
{
    auto it = installs.find (id);
    if (it == installs.end())
        return {};
    return it->second;
}

void VersionTracker::recordInstall (const juce::String& id,
                                    const juce::String& version,
                                    const juce::File&   bundlePath)
{
    InstalledPlugin p;
    p.id          = id;
    p.version     = version;
    p.installPath = bundlePath.getFullPathName();
    p.installedAt = nowIso();

    installs[id] = std::move (p);
    save();
}

void VersionTracker::recordUninstall (const juce::String& id)
{
    installs.erase (id);
    save();
}
