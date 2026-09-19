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

## The Deformer

The **Deformer** is currently exclusive to the Sphere geometry, and it works in three coordinated layers, spectral, timbral, and audio, all scaled by a single **Deform** slider, with **Freq**, **Rate**, and **Noise** shaping how the underlying noise field behaves.

At the spectral layer, noise displaces the sphere's surface normals. Rather than sampling a single point, Elements samples 32 uniformly-distributed directions across the sphere, placed via a golden-ratio (Fibonacci) mapping, and calculates the noise gradient at each one. Those displaced normals are run through a full Fresnel calculation per wavelength, weighted by their angle to the light source. Rotation now modulates the timbre in ways a perfect sphere never could, because each displaced normal presents a different Fresnel angle to the light. The **Noise** selector picks the character of that field: **Simplex** (smooth, organic), **Alligator** (rounded, cellular bumps), or **Worley** (sharp, Voronoi-like ridges).

The second layer is timbral drift. Each wavelength in the material's spectrum tracks the noise field independently and slightly decorrelated from its neighbors, so harmonics shimmer rather than move in lockstep. **Freq** controls how far apart neighboring harmonics sample the field: tight and coherent at low values, twinkling and independent at high values. **Rate** controls how fast the underlying noise field itself evolves over time; at Rate=0 the shimmer freezes completely.

At the audio layer, the same shimmer state that drives the timbral drift also drives a **sinusoidal wavefolder** on the output signal. The fold amount breathes in sync with the spectral movement instead of following a fixed curve, so the audio layer's harmonic complexity rises and falls with the same noise shaping the timbre. Unlike clipping or saturation, wavefolding is periodic and always stays within bounds, producing a more musical harmonic distribution.

All three layers respond to **Deform** simultaneously, colored by Freq, Rate, and Noise type.

---

## Physical Envelope

The **Physical Envelope** extends the same optical logic into the amplitude domain. In Physical mode, the four ADSR stages are no longer manual knobs: they are derived automatically from the optical properties of the active material.

- **Attack** is driven by light intensity. More light means more photonic energy, which means a faster attack: interpolating from 0.5s at minimum intensity down to 0.005s at maximum.
- **Decay** is a function of material thickness and absorption. A thick, opaque material absorbs more light and produces a longer decay.
- **Sustain** is mapped to the index of refraction (IOR), normalized against Diamond, the densest material at 2.42. Higher IOR means more internal reflections, which means a higher sustain level. Diamond sits near 0.95; Water around 0.55.
- **Release** combines IOR and thickness. A dense, thick material traps light longer, producing a slower release.

Every value recalculates automatically whenever you change the material, adjust the thickness, or move the lights. The envelope becomes a property of the scene, not a separate set of controls.

---

## Light as a musical instrument

The **Lights Bar** is one of Elements' most expressive features. Each light source has an intensity parameter that is directly mapped to pitch: at 50% intensity the pitch is neutral, below that it drops, above it rises. This means you can use light intensity as a performance parameter, creating pitch movement that feels organic rather than mechanical.

This bidirectional relationship between light and pitch is at the heart of what makes Elements unusual. You're not modulating pitch with an LFO. You're changing how much light hits a surface, and the physics take care of the rest.

---

*Elements is currently in beta. Available for macOS and Windows.*
*macOS 12 or later · Universal Binary (Apple Silicon + Intel) · Windows 10/11 x64*

[Quick Start](quick-start) · [Installation](installation) · [Materials & Geometry](materials-and-geometry) · [Parameters](parameters) · [Known Issues](known-issues) · [The Science](science)

---

*Elements is free and always will be. If it made your music better, [buy me a coffee](https://ko-fi.com/matiasderose).*
