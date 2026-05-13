#pragma once

#include <JuceHeader.h>
#include "MainComponent.h"
#include "ShpLookAndFeel.h"

class MainWindow final : public juce::DocumentWindow
{
public:
    explicit MainWindow (const juce::String& name);
    ~MainWindow() override;

    void closeButtonPressed() override;

private:
    ShpLookAndFeel lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};
