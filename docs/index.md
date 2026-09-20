---
layout: default
title: Elements
description: A synthesizer where light meets sound.
---

**[Download Beta v0.9.4 for macOS & Windows](https://github.com/elements-synth/elements/releases)**
*VST3 · macOS Universal Binary (Apple Silicon + Intel) · Windows x64 · Free*

[Quick Start](quick-start) · [Installation](installation) · [Materials & Geometry](materials-and-geometry) · [Parameters](parameters) · [Known Issues](known-issues) · [The Science](science)

---

## Video demos

- [The Deformer](https://www.youtube.com/watch?v=lJhoasqMsv8): Simplex Noise + Wavefolding in action
- [Light × Material](https://www.youtube.com/watch?v=kIsSFacwmfY): light intensity controls pitch, material controls timbre
- [Geometry × Rotation](https://www.youtube.com/watch?v=xloh2gQo6ps): same material, same light, four different sounds
- [3D Viewport](https://www.youtube.com/watch?v=WOz9LchJtqY): the instrument in three dimensions

---

## What is Elements?

Elements is a spectral wavetable synthesizer built around a simple but unusual idea: what if a synthesizer was a physical scene, and sound was what happened when light hit matter?

In traditional synthesis, you start with a waveform and shape it: filter it, envelope it, modulate it. Elements takes a different approach. You begin by choosing a **material** and a **geometry**. A cube of diamond. A sphere of water. A torus of amber. A dodecahedron of obsidian. Then you shine a **light** on it. You can also introduce a second material entirely, blending two independent optical scenes into one voice.

The sound you hear is the result of that interaction: the specific combination of light intensity, geometry, and material properties. Change any one of them and the sound changes, the way a render changes when you move a light or swap a shader. The synthesizer is not a signal chain. It's a physical scene.

This makes Elements feel different from anything else you've used. Sound design stops being a matter of turning the right knobs in the right order, and starts feeling more like setting up a shot: placing objects, adjusting light, watching (and hearing) how everything responds to each other.

---

## Two materials, one voice

Elements runs two independent oscillators, A and B, each shaped by its own material and geometry. A **BLEND** mode decides how their spectra combine at the sample level:

- **Ring Mod**: multiplies A and B sample-by-sample, producing sum/difference sidebands that exist in neither source spectrum. Inharmonic and metallic, strongest with contrasting materials.
- **AM**: B modulates the amplitude of A. At low depth you hear A enriched with new harmonics; the original timbre stays recognizable.
- **XOR**: takes the absolute difference between A and B, highlighting where their spectra disagree most. Subtractive and hollow.
- **FM**: B modulates the phase of A before wavetable readout, from subtle pitch drift at low depth to dense, DX-style inharmonic spectra at high depth.

**MIX** crossfades between pure A and the blended result, **DETUNE** offsets oscillator B's pitch (±100 cents), and **DEPTH** controls modulation intensity for AM and FM. Mute A to audition B in isolation. The result is a genuinely two-voice instrument: two separate optical scenes, combined in real time.

---

## The physics behind the sound

Every material in Elements corresponds to a specific set of optical properties: index of refraction, Fresnel response, spectral transmission curve. These aren't invented presets: the data is drawn from real spectroscopy, including Sellmeier dispersion equations, GIA and peer-reviewed gemological papers, the USGS spectral library, and Pope & Fry's water-absorption measurements. Every sound Elements produces is the output of one formula, evaluated per wavelength:

```
sound = light emission × material transmission × Fresnel response
```

Change the light, the material, or the angle between them, and every harmonic in the spectrum shifts accordingly. There's no separate "timbre" parameter; timbre *is* this calculation.

---

## Shaping sound with lights

The **Lights Bar** is one of Elements' most expressive features. Each light source has an intensity parameter that is directly mapped to pitch: at 50% intensity the pitch is neutral, below that it drops, above it rises. This means you can use light intensity as a performance parameter, creating pitch movement that feels organic rather than mechanical.

This bidirectional relationship between light and pitch is at the heart of what makes Elements unusual. You're not modulating pitch with an LFO. You're changing how much light hits a surface, and the physics take care of the rest.

---

*Available for macOS and Windows.*
*macOS 12 or later · Universal Binary (Apple Silicon + Intel) · Windows 10/11 x64*

[Quick Start](quick-start) · [Installation](installation) · [Materials & Geometry](materials-and-geometry) · [Parameters](parameters) · [Known Issues](known-issues) · [The Science](science)

---

*Elements is free and always will be. If it made your music better, [buy me a coffee](https://ko-fi.com/matiasderose).*
