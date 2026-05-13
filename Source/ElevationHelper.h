#pragma once

#include <JuceHeader.h>

namespace ElevationHelper
{
    // Relaunches this same executable with `--install-helper <src> <dest>` under UAC elevation
    // and waits for it to exit. Returns true on success (exit code 0).
    bool runElevatedCopy (const juce::File& sourceBundle, const juce::File& destinationBundle);

    // Performs the actual copy (used by the helper process). Returns 0 on success.
    int runHelperCopy (const juce::File& sourceBundle, const juce::File& destinationBundle);
}
