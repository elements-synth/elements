/*
  ==============================================================================
    ElementsUI.h
    Elements - Shared UI color palette and material accent system
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

// ==============================================================================
// COLOR PALETTE
// ==============================================================================

namespace ElementsColors
{
    // Backgrounds (darkest → lightest)
    const juce::Colour bg0    {0xff0d1117};   // deepest background
    const juce::Colour bg1    {0xff111820};   // panels
    const juce::Colour bg2    {0xff151e2a};   // elevated elements
    const juce::Colour bg3    {0xff1c2534};   // buttons / controls

    // Borders and text
    const juce::Colour border {0xff1f2d3d};
    const juce::Colour mid    {0xff4a6075};
    const juce::Colour dim    {0xff2a3d50};
    const juce::Colour text   {0xffd0e4f0};
}

// ==============================================================================
// MATERIAL ACCENT COLORS
// ==============================================================================

namespace MaterialAccents
{
    const juce::Colour diamond  {0xffa8d8f0};
    const juce::Colour water    {0xff7ec8e3};
    const juce::Colour amber    {0xfff5b942};
    const juce::Colour ruby     {0xffe84b6a};
    const juce::Colour gold     {0xffd4a843};
    const juce::Colour emerald  {0xff4ecb8d};
    const juce::Colour amethyst {0xffb57bee};
    const juce::Colour sapphire {0xff5b9ef5};
    const juce::Colour copper   {0xffcf7e46};
    const juce::Colour obsidian {0xff6a7a8a};

    inline juce::Colour getAccentForMaterial(int index)
    {
        static const juce::Colour accents[] = {
            diamond, water, amber, ruby, gold,
            emerald, amethyst, sapphire, copper, obsidian
        };
        if (index >= 0 && index < 10)
            return accents[index];
        return diamond;
    }
}
