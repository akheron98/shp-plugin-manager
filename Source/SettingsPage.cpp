#include "SettingsPage.h"
#include "Theme.h"

using namespace shp::theme;

namespace
{
void styleSectionLabel (juce::Label& l)
{
    l.setFont (oswaldFont (12.0f, juce::Font::bold, 0.10f));
    l.setColour (juce::Label::textColourId, blood.brighter (0.3f));
}

void styleValueLabel (juce::Label& l)
{
    l.setFont (monoFont (11.0f));
    l.setColour (juce::Label::textColourId, bone);
}

void styleEditor (juce::TextEditor& e)
{
    e.setFont (monoFont (11.0f));
    e.setColour (juce::TextEditor::backgroundColourId, surfaceDeep);
    e.setColour (juce::TextEditor::textColourId,       bone);
    e.setColour (juce::TextEditor::outlineColourId,    railDark);
    e.setColour (juce::TextEditor::focusedOutlineColourId, blood);
    e.setColour (juce::TextEditor::highlightColourId, blood.withAlpha (0.4f));
    e.setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
    e.setColour (juce::CaretComponent::caretColourId, bone);
}
}

SettingsPage::SettingsPage (Settings& s) : settings (s)
{
    styleSectionLabel (pathLabel);
    styleValueLabel (customPathDisplay);

    for (auto* t : { &scopeUserBtn, &scopeSysBtn, &scopeCustBtn })
    {
        t->setRadioGroupId (1);
        t->setColour (juce::ToggleButton::textColourId, bone);
        t->setColour (juce::ToggleButton::tickColourId, blood.brighter (0.2f));
        t->setColour (juce::ToggleButton::tickDisabledColourId, dust);
        addAndMakeVisible (*t);
    }

    addAndMakeVisible (pathLabel);
    addAndMakeVisible (customPathDisplay);
    addAndMakeVisible (browseBtn);
    addAndMakeVisible (saveBtn);
    addAndMakeVisible (closeBtn);

    browseBtn.onClick = [this] { pickCustomDir(); };
    saveBtn.onClick   = [this]
    {
        saveFromUi();
        if (onSettingsChanged) onSettingsChanged();
        if (onClose) onClose();
    };
    closeBtn.onClick  = [this] { if (onClose) onClose(); };

    applyToUi();
}

void SettingsPage::applyToUi()
{
    switch (settings.getInstallScope())
    {
        case Settings::InstallScope::userLocal:  scopeUserBtn.setToggleState (true, juce::dontSendNotification); break;
        case Settings::InstallScope::systemWide: scopeSysBtn .setToggleState (true, juce::dontSendNotification); break;
        case Settings::InstallScope::custom:     scopeCustBtn.setToggleState (true, juce::dontSendNotification); break;
    }

    auto cp = settings.getCustomInstallPath().getFullPathName();
    if (cp.isEmpty())
        cp = Settings::getUserLocalVst3Dir().getFullPathName();
    customPathDisplay.setText (cp, juce::dontSendNotification);
}

void SettingsPage::saveFromUi()
{
    if (scopeUserBtn.getToggleState())  settings.setInstallScope (Settings::InstallScope::userLocal);
    if (scopeSysBtn .getToggleState())  settings.setInstallScope (Settings::InstallScope::systemWide);
    if (scopeCustBtn.getToggleState())  settings.setInstallScope (Settings::InstallScope::custom);

    settings.setCustomInstallPath (juce::File (customPathDisplay.getText()));

    settings.save();
}

void SettingsPage::pickCustomDir()
{
    auto initial = juce::File (customPathDisplay.getText());
    if (! initial.isDirectory())
        initial = Settings::getUserLocalVst3Dir();

    auto chooser = std::make_shared<juce::FileChooser> (
        "Pick a custom VST3 install directory", initial);

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser] (const juce::FileChooser&)
        {
            auto f = chooser->getResult();
            if (f.isDirectory())
            {
                customPathDisplay.setText (f.getFullPathName(), juce::dontSendNotification);
                scopeCustBtn.setToggleState (true, juce::dontSendNotification);
            }
        });
}

void SettingsPage::paint (juce::Graphics& g)
{
    g.fillAll (background);

    g.setColour (surfaceDeep);
    g.fillRect (getLocalBounds().removeFromTop (44));
    g.setColour (railDark);
    g.drawHorizontalLine (44, 0.0f, (float) getWidth());

    g.setColour (bone);
    g.setFont (oswaldFont (20.0f, juce::Font::bold, 0.05f));
    g.drawText ("SETTINGS",
                juce::Rectangle<int> (20, 0, getWidth() - 40, 44),
                juce::Justification::centredLeft);
}

void SettingsPage::resized()
{
    auto bounds = getLocalBounds();

    auto header = bounds.removeFromTop (44);
    auto closeArea = header.reduced (8).removeFromRight (90);
    closeBtn.setBounds (closeArea.withSizeKeepingCentre (90, 28));

    bounds.reduce (24, 18);

    auto footer = bounds.removeFromBottom (40);
    saveBtn.setBounds (footer.removeFromRight (140).withSizeKeepingCentre (130, 30));

    auto place = [&] (juce::Component& c, int h)
    {
        auto r = bounds.removeFromTop (h);
        c.setBounds (r);
        bounds.removeFromTop (4);
    };

    place (pathLabel, 18);
    place (scopeUserBtn, 26);
    place (scopeSysBtn,  26);
    place (scopeCustBtn, 26);

    {
        auto r = bounds.removeFromTop (28);
        browseBtn.setBounds (r.removeFromRight (110).withSizeKeepingCentre (100, 24));
        customPathDisplay.setBounds (r.reduced (4));
        bounds.removeFromTop (12);
    }
}
