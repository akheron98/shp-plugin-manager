#include "PluginCardComponent.h"
#include "Theme.h"
#include <BinaryData.h>

using namespace shp::theme;

PluginCardComponent::PluginCardComponent (PluginInfo i)
    : info (std::move (i))
{
    if (info.hasAction())
    {
        addAndMakeVisible (actionButton);
        actionButton.setButtonText (info.actionLabel());
        actionButton.setToggleState (info.status == PluginInfo::Status::updateAvailable, juce::dontSendNotification);
        actionButton.onClick = [this] { if (onAction) onAction (info); };
    }

    if (info.manualUrl.isNotEmpty())
    {
        addAndMakeVisible (manualButton);
        manualButton.setButtonText ("MANUEL");
        manualButton.setColour (juce::TextButton::buttonColourId, shp::theme::surface);
        manualButton.setColour (juce::TextButton::textColourOffId, shp::theme::bone.withAlpha(0.86f));
        manualButton.onClick = [this] { juce::URL (info.manualUrl).launchInDefaultBrowser(); };
    }

    logo = juce::ImageCache::getFromMemory (BinaryData::shp_logo_v3_png,
                                            BinaryData::shp_logo_v3_pngSize);
}

void PluginCardComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);

    juce::ColourGradient bg (moduleA,
                             bounds.getX(),
                             bounds.getY(),
                             moduleB,
                             bounds.getRight(),
                             bounds.getBottom(),
                             false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (railLight.withAlpha (0.18f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    g.setColour (railDark.withAlpha (0.8f));
    g.drawHorizontalLine (juce::roundToInt (bounds.getBottom()) - 1,
                          bounds.getX(),
                          bounds.getRight());

    // Logo
    auto logoArea = juce::Rectangle<int> (16, (getHeight() - 56) / 2, 56, 56);
    if (logo.isValid())
        g.drawImage (logo, logoArea.toFloat(), juce::RectanglePlacement::centred);

    // Title
    auto textArea = juce::Rectangle<int> (logoArea.getRight() + 16,
                                          12,
                                          getWidth() - logoArea.getRight() - 200,
                                          getHeight() - 24);

    g.setColour (bone);
    g.setFont (oswaldFont (22.0f, juce::Font::bold, 0.04f));
    g.drawText (info.name.toUpperCase(),
                textArea.removeFromTop (26),
                juce::Justification::centredLeft,
                true);

    g.setColour (dimBone);
    g.setFont (monoFont (10.5f));
    g.drawText (info.description,
                textArea.removeFromTop (16),
                juce::Justification::centredLeft,
                true);

    // Version pill
    auto rightArea = juce::Rectangle<int> (getWidth() - 200,
                                           0,
                                           200,
                                           getHeight());
    auto pillArea = rightArea.withSizeKeepingCentre (180, 22).translated (-10, -20);

    const auto statusColour = info.status == PluginInfo::Status::updateAvailable ? blood
                            : info.status == PluginInfo::Status::upToDate        ? juce::Colour::fromRGB (40, 100, 70)
                                                                                  : dust;

    g.setColour (statusColour.withAlpha (0.22f));
    g.fillRoundedRectangle (pillArea.toFloat(), 11.0f);
    g.setColour (statusColour.withAlpha (0.9f));
    g.drawRoundedRectangle (pillArea.toFloat(), 11.0f, 1.0f);

    g.setColour (bone);
    g.setFont (monoFont (9.5f, juce::Font::bold));
    g.drawFittedText (info.statusLabel(),
                      pillArea.reduced (8, 2),
                      juce::Justification::centred,
                      1);

    // Versions
    auto verArea = pillArea.translated (0, 28).withHeight (16);
    g.setColour (dimBone);
    g.setFont (monoFont (10.0f));
    juce::String verText;
    switch (info.status)
    {
        case PluginInfo::Status::upToDate:
            verText = "v" + info.installedVersion;
            break;
        case PluginInfo::Status::updateAvailable:
            verText = "v" + info.installedVersion + "  →  v" + info.latestVersion;
            break;
        case PluginInfo::Status::notInstalled:
            verText = "v" + info.latestVersion + " available";
            break;
        case PluginInfo::Status::noRelease:
            verText = "—";
            break;
        case PluginInfo::Status::error:
            verText = info.errorMessage.isNotEmpty() ? info.errorMessage : juce::String ("error");
            break;
    }
    g.drawFittedText (verText, verArea, juce::Justification::centred, 1);
}

void PluginCardComponent::resized()
{
    auto rightArea = juce::Rectangle<int> (getWidth() - 200, 0, 200, getHeight());
    auto buttonRect = rightArea.withSizeKeepingCentre (160, 30).translated (-10, 30);
    
    actionButton.setBounds (buttonRect);

    if (info.manualUrl.isNotEmpty())
    {
        // Place the manual button to the left of the action button
        auto manualRect = buttonRect.translated (-100, 0).withWidth (90);
        manualButton.setBounds (manualRect);
    }
}
