#pragma once

#include <JuceHeader.h>

class ShpLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ShpLookAndFeel();

    void drawButtonBackground (juce::Graphics&,
                               juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&,
                         juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;
};
