#pragma once

#include <JuceHeader.h>

struct PluginInfo
{
    enum class Status
    {
        notInstalled,
        upToDate,
        updateAvailable,
        noRelease,
        error
    };

    juce::String id;
    juce::String name;
    juce::String description;
    juce::String installedVersion;
    juce::String latestVersion;
    juce::String errorMessage;          // populated when Status::error or Status::noRelease
    Status status { Status::notInstalled };

    juce::String statusLabel() const
    {
        switch (status)
        {
            case Status::notInstalled:    return "NOT INSTALLED";
            case Status::upToDate:        return "INSTALLED";
            case Status::updateAvailable: return "UPDATE AVAILABLE";
            case Status::noRelease:       return "NO RELEASE YET";
            case Status::error:           return "ERROR";
        }
        return {};
    }

    juce::String actionLabel() const
    {
        switch (status)
        {
            case Status::notInstalled:    return "Install";
            case Status::upToDate:        return "Uninstall";
            case Status::updateAvailable: return "Update";
            case Status::noRelease:
            case Status::error:           return {};
        }
        return {};
    }

    bool hasAction() const { return actionLabel().isNotEmpty(); }
};
