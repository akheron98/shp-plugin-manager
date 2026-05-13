#include "ElevationHelper.h"

#if JUCE_WINDOWS
 #include <windows.h>
 #include <shellapi.h>
#endif

namespace ElevationHelper
{

int runHelperCopy (const juce::File& sourceBundle, const juce::File& destinationBundle)
{
    if (! sourceBundle.isDirectory())
    {
        juce::Logger::writeToLog ("Helper: source missing: " + sourceBundle.getFullPathName());
        return 2;
    }

    if (destinationBundle.exists())
    {
        if (! destinationBundle.deleteRecursively())
        {
            juce::Logger::writeToLog ("Helper: failed to delete existing destination: "
                                      + destinationBundle.getFullPathName());
            return 3;
        }
    }

    destinationBundle.getParentDirectory().createDirectory();

    auto result = sourceBundle.copyDirectoryTo (destinationBundle);
    return result ? 0 : 4;
}

bool runElevatedCopy (const juce::File& sourceBundle, const juce::File& destinationBundle)
{
   #if JUCE_WINDOWS
    auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);

    auto quote = [] (const juce::String& s) -> juce::String
    {
        return "\"" + s + "\"";
    };

    juce::String params;
    params << "--install-helper "
           << quote (sourceBundle.getFullPathName()) << " "
           << quote (destinationBundle.getFullPathName());

    SHELLEXECUTEINFOW sei {};
    sei.cbSize = sizeof (sei);
    sei.fMask  = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    sei.lpVerb = L"runas";

    auto exeStr    = exe.getFullPathName().toWideCharPointer();
    auto paramsW   = params.toWideCharPointer();

    sei.lpFile       = exeStr;
    sei.lpParameters = paramsW;
    sei.nShow        = SW_HIDE;

    if (! ShellExecuteExW (&sei))
        return false;

    if (sei.hProcess == nullptr)
        return false;

    WaitForSingleObject (sei.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess (sei.hProcess, &exitCode);
    CloseHandle (sei.hProcess);

    return exitCode == 0;
   #else
    juce::ignoreUnused (sourceBundle, destinationBundle);
    return false;
   #endif
}

} // namespace ElevationHelper
