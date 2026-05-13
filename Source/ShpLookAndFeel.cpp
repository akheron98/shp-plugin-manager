#include "ShpLookAndFeel.h"
#include "Theme.h"

using namespace shp::theme;

ShpLookAndFeel::ShpLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::Label::textColourId,                 bone);
    setColour (juce::TextButton::buttonColourId,          surface);
    setColour (juce::TextButton::buttonOnColourId,        bloodDeep);
    setColour (juce::TextButton::textColourOffId,         dimBone);
    setColour (juce::TextButton::textColourOnId,          juce::Colours::white);
    setColour (juce::ScrollBar::thumbColourId,            dust.withAlpha (0.55f));
    setColour (juce::ScrollBar::trackColourId,            surfaceDeep);
}

void ShpLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                           juce::Button& button,
                                           const juce::Colour&,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto active = button.getToggleState();

    juce::ColourGradient fill (active ? blood : juce::Colour::fromRGB (18, 18, 22),
                               bounds.getX(),
                               bounds.getY(),
                               active ? bloodDeep : juce::Colour::fromRGB (5, 5, 8),
                               bounds.getRight(),
                               bounds.getBottom(),
                               false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds, 2.0f);

    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
    {
        g.setColour (bloodBright.withAlpha (0.16f));
        g.fillRoundedRectangle (bounds.reduced (3.0f, 2.0f), 1.5f);
    }

    g.setColour ((active ? bloodBright : dust).withAlpha (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown ? 0.95f : 0.68f));
    g.drawRoundedRectangle (bounds, 2.0f, 1.0f);
}

void ShpLookAndFeel::drawButtonText (juce::Graphics& g,
                                     juce::TextButton& button,
                                     bool,
                                     bool)
{
    g.setFont (monoFont (10.5f, juce::Font::bold));
    g.setColour (button.getToggleState() ? juce::Colours::white : bone.withAlpha (0.86f));
    g.drawFittedText (button.getButtonText().toUpperCase(),
                      button.getLocalBounds().reduced (8, 2),
                      juce::Justification::centred,
                      1);
}
