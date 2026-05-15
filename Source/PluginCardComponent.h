#pragma once

#include <JuceHeader.h>
#include "PluginInfo.h"

class PluginCardComponent : public juce::Component
{
public:
    explicit PluginCardComponent (PluginInfo info);

    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void (const PluginInfo&)> onAction;

private:
    PluginInfo info;
    juce::TextButton actionButton;
    juce::TextButton manualButton;
    juce::Image logo;
};
