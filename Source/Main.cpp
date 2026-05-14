#include <JuceHeader.h>
#include "MainWindow.h"
#include "ElevationHelper.h"

namespace
{
// Returns true if argv contains "--install-helper" and runs the helper, setting outExitCode.
bool tryRunInstallHelper (const juce::String& cmdLine, int& outExitCode)
{
    if (! cmdLine.contains ("--install-helper"))
        return false;

    auto tokens = juce::StringArray::fromTokens (cmdLine, true);
    // tokens: ... --install-helper "<src>" "<dest>"
    int idx = tokens.indexOf ("--install-helper");
    if (idx < 0 || tokens.size() < idx + 3)
    {
        outExitCode = 10;
        return true;
    }

    juce::File src  (tokens[idx + 1].unquoted());
    juce::File dest (tokens[idx + 2].unquoted());

    outExitCode = ElevationHelper::runHelperCopy (src, dest);
    return true;
}
}

class SHPPluginManagerApplication final : public juce::JUCEApplication
{
public:
    SHPPluginManagerApplication() = default;

    const juce::String getApplicationName() override       { return "SHP Plugin Manager"; }
    const juce::String getApplicationVersion() override    { return "0.1.2"; }
    bool moreThanOneInstanceAllowed() override             { return true; } // helper mode

    void initialise (const juce::String& commandLine) override
    {
        int helperExit = 0;
        if (tryRunInstallHelper (commandLine, helperExit))
        {
            setApplicationReturnValue (helperExit);
            quit();
            return;
        }

        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override                    { quit(); }

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (SHPPluginManagerApplication)
