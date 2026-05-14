#pragma once

#include <JuceHeader.h>
#include "Settings.h"

class SettingsPage : public juce::Component
{
public:
    explicit SettingsPage (Settings& settings);

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void()> onClose;
    std::function<void()> onSettingsChanged;

private:
    void applyToUi();
    void saveFromUi();
    void pickCustomDir();

    Settings& settings;

    juce::Label    pathLabel        { {}, "INSTALL LOCATION" };
    juce::ToggleButton scopeUserBtn { "User (no UAC)" };
    juce::ToggleButton scopeSysBtn  { "System (Program Files, needs UAC)" };
    juce::ToggleButton scopeCustBtn { "Custom path" };
    juce::Label    customPathDisplay;
    juce::TextButton browseBtn      { "Browse..." };

    juce::TextButton saveBtn  { "Save" };
    juce::TextButton closeBtn { "Back" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsPage)
};
