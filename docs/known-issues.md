---
layout: default
title: Known Issues & Limitations · Elements
---

[← Back to Elements](index)

# Known Issues & Limitations

Elements 1.0 is functional and stable, but there are known limitations to be aware of before using it in a production context.

---

## Presets

Elements has its own preset system: 15 factory presets plus SAVE / DEL for your own, all in the PRESET dropdown inside the plugin. Presets are not exposed to your DAW's own preset browser. The plugin reports a single program placeholder as required by JUCE, so the DAW's preset list will look empty. This is expected.

---

## Polyphony

Elements has a fixed limit of **8 voices**. When a ninth note arrives, the oldest held voice is taken over for the new note, with a short (about 5 to 6 ms) anti-click fade-in. In practice no note is dropped, but with long release times and fast passages you will hear the oldest notes cut short to make room.

---

## State persistence

The plugin saves and restores its full state with your project:

- All 35 automatable parameters (filter, envelopes, rotation, thickness, deform, light intensities, dual-oscillator and chorus controls, transpose, volume, etc.)
- Selected material (A and B), blend mode and geometry
- Light source (Sunset / Daylight / LED Cool) and on/off state for each of the three lights

Nothing is known to reset when you reopen a project. Your own presets are stored separately by the plugin's PRESET menu.

---

## Long-running sessions

**Sound may stop responding to changes in a session left open for a long time.** In Bitwig Studio, after returning to a project that had been open for a long time (more than a day in the reported case), changes to the material, lights, geometry or other parameters sometimes had no audible effect. We have not found the cause or a way to reproduce it, so it is listed here as an open issue rather than a fixed one.

What helps: recreate the Elements instrument track (a plain plugin rescan is not enough). This is the same step that fixed it when it was first seen. Saving your work and reopening the project is a reasonable first thing to try, but we have not confirmed that it clears the problem.

If you run into it, please [open an issue](https://github.com/elements-synth/elements/issues) with your host, how long the session had been open, and which controls stopped responding.

---

## Host compatibility

Elements uses JUCE's generic VST3 implementation with no host-specific workarounds. Elements is released as VST3 and as a Standalone app. There is no AU build for now.

**Bitwig Studio**: Bitwig aggressively caches VST3 parameter layouts. If you update Elements to a version that adds new parameters, you will need to recreate the instrument track (rescanning plugins is not enough) for Bitwig to recognize the new parameters.

**Logic Pro**: Logic loads Audio Units only, so Elements does not currently work there (no AU build is released).

**Other DAWs**: Not tested. Elements declares mono and stereo support, accepts standard MIDI, and reports a 5-second tail length for release.

---

## Other limitations

**Occasional saturation**: With spectrally dense materials and high polyphony, the output signal can clip. A soft clipper (tanh) is in place but there is no full limiter. If you experience saturation, reduce the light intensities or the number of simultaneous voices.

**Apple Silicon + Intel**: Elements is a Universal Binary, running natively on both Apple Silicon and Intel Macs. A Windows x64 build (VST3 and Standalone) is also provided, but it is built automatically and is less tested than the macOS version.

**Deformer is Sphere-only**: The Geometric Deformer works on the Sphere geometry only. With the other geometries its controls are greyed out and the sound is not deformed.

---

*Found something not listed here? Please [open an issue](https://github.com/elements-synth/elements/issues) on GitHub.*

*Elements is free. If you find it useful, [buy me a coffee](https://ko-fi.com/matiasderose).*

[← Back to Elements](index)
