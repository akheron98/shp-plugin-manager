#pragma once

#include <JuceHeader.h>

namespace shp::theme
{
inline const auto background  = juce::Colour::fromRGB (4,   4,   10);
inline const auto surface     = juce::Colour::fromRGB (12,  12,  16);
inline const auto surfaceDeep = juce::Colour::fromRGB (8,   8,   11);
inline const auto moduleA     = juce::Colour::fromRGB (26,  26,  32);
inline const auto moduleB     = juce::Colour::fromRGB (21,  21,  26);
inline const auto railDark    = juce::Colour::fromRGB (32,  34,  38);
inline const auto railLight   = juce::Colour::fromRGB (78,  80,  78);
inline const auto bone        = juce::Colour::fromRGB (205, 198, 180);
inline const auto dimBone     = juce::Colour::fromRGB (118, 116, 108);
inline const auto dust        = juce::Colour::fromRGB (83,  84,  84);
inline const auto blood       = juce::Colour::fromRGB (190, 18,  32);
inline const auto bloodBright = juce::Colour::fromRGB (245, 34,  48);
inline const auto bloodDeep   = juce::Colour::fromRGB (82,  5,   12);

inline juce::Font monoFont (float height, int flags = 0)
{
    return juce::Font (juce::FontOptions ("JetBrains Mono", height, flags)
        .withFallbacks ({ "Consolas", "Cascadia Mono", juce::Font::getDefaultMonospacedFontName() }));
}

inline juce::Font oswaldFont (float height, int flags = juce::Font::bold, float tracking = 0.0f)
{
    auto font = juce::Font (juce::FontOptions ("Oswald", height, flags)
        .withFallbacks ({ "Arial Narrow", "Segoe UI", juce::Font::getDefaultSansSerifFontName() }));
    font.setExtraKerningFactor (tracking);
    return font;
}
}
