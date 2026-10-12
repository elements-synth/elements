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

Elements has a fixed limit of **8 voices**. When all 8 are active, soft voice stealing kicks in: the oldest voice fades out over 256 samples (~5.8ms) and is reassigned. If all 8 voices are simultaneously fading out, new notes are discarded silently with no click or artifact. This can occur in fast passages with long sustain times.

---

## State persistence

The plugin saves and restores its state between sessions. The following is persisted:

- All 28 automatable parameters (filter, envelopes, rotation, thickness, deform, light intensities, etc.)
- Selected material and geometry
- Volume

The following resets to defaults when reopening a project:

- **Light source selection per slot** (Sunset / Daylight / LED Cool)
- **Light on/off state per slot**

This means you will need to reassign your light sources after reloading a project. This will be addressed in a future update.

---

## Host compatibility

Elements uses JUCE's generic VST3 and AU implementation with no host-specific workarounds.

**Bitwig Studio**: Bitwig aggressively caches VST3 parameter layouts. If you update Elements to a version that adds new parameters, you will need to recreate the instrument track (rescanning plugins is not enough) for Bitwig to recognize the new parameters.

**Logic Pro (AU)**: Not extensively tested. The AU wrapper is JUCE's generic implementation with no Logic-specific adaptations. No issues are known, but full compatibility cannot be guaranteed.

**Other DAWs**: Not tested. Elements declares mono and stereo support, accepts standard MIDI, and reports a 5-second tail length for release.

---

## Other limitations

**Occasional saturation**: With spectrally dense materials and high polyphony, the output signal can clip. A soft clipper (tanh) is in place but there is no full limiter. If you experience saturation, reduce the light intensities or the number of simultaneous voices.

**Apple Silicon + Intel**: Elements is a Universal Binary, running natively on both Apple Silicon and Intel Macs. A Windows x64 build is also provided, but it is built automatically and is less tested than the macOS version.

**Deformer is Sphere-only**: The Geometric Deformer is currently implemented for the Sphere geometry only. Support for other geometries is planned.

---

*Found something not listed here? Please [open an issue](https://github.com/elements-synth/elements/issues) on GitHub.*

*Elements is free. If you find it useful, [buy me a coffee](https://ko-fi.com/matiasderose).*

[← Back to Elements](index)
