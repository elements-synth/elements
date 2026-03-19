---
layout: default
title: Elements
description: A synthesizer where light meets sound.
---

# Elements

### A synthesizer where light meets sound.

**[Download Beta 1.0 — macOS](https://github.com/elements-synth/elements/releases)**  
*VST3 · AU · macOS only for now · Free*

[Installation](installation) · [Materials & Geometry](materials-and-geometry) · [Known Issues](known-issues)

---

## What is Elements?

Elements is a spectral wavetable synthesizer built around a simple but unusual idea: what if a synthesizer was a physical scene — and sound was what happened when light hit matter?

In traditional synthesis, you start with a waveform and shape it — filter it, envelope it, modulate it. Elements takes a different approach. You begin by choosing a **material** and a **geometry**. A cube of diamond. A sphere of water. A torus of amber. A dodecahedron of obsidian. Then you shine a **light** on it.

The sound you hear is the result of that interaction — the specific combination of light intensity, geometry, and material properties. Change any one of them and the sound changes, the way a render changes when you move a light or swap a shader. The synthesizer is not a signal chain. It's a physical scene.

This makes Elements feel different from anything else you've used. Sound design stops being a matter of turning the right knobs in the right order, and starts feeling more like setting up a shot — placing objects, adjusting light, watching (and hearing) how everything responds to each other.

---

## The physics behind the sound

Every material in Elements corresponds to a specific set of optical properties — index of refraction, Fresnel response, spectral behavior. These aren't just names or presets. They define how light interacts with the surface, and through that interaction, what harmonics the oscillator produces.

The **Geometric Deformer** is currently exclusive to the Sphere geometry, and it works on two levels simultaneously — both affecting how the sound is generated and how it evolves over time.

At the spectral level, it applies a 3D Simplex Noise field to the sphere's surface normals. Rather than sampling a single point, Elements samples 12 directions distributed across the sphere — the vertices of an icosahedron — and calculates the noise gradient at each one. Those displaced normals are then run through a full Fresnel calculation per wavelength, weighted by their angle to the light source. The result is that rotation now modulates the timbre in ways a perfect sphere never could, because each displaced normal presents a different Fresnel angle to the light.

What makes this genuinely unusual is the **timbral drift**: when the Deformer is active, the noise field moves continuously across the surface at a slow, organic rate. The spectrum is recalculated at roughly 8Hz, producing a subtle, living movement in the sound that no static wavetable can replicate.

At the audio level, the same Deform parameter drives a **sinusoidal wavefolder** directly on the output signal. The drive scales from 1 to 15 as you increase the parameter — at low values the signal passes nearly unchanged, at high values it folds back on itself repeatedly, generating dense harmonic content with a metallic, complex character. Unlike clipping or saturation, wavefolding is periodic and always stays within bounds, producing a more musical harmonic distribution.

Both effects — spectral and audio — are controlled by a single **Deform** slider, and both respond to it simultaneously.

The **Physical Envelope** extends this logic into the amplitude domain. In Physical mode, the four ADSR stages are no longer manual knobs — they are derived automatically from the optical properties of the active material:

- **Attack** is driven by light intensity. More light means more photonic energy, which means a faster attack — interpolating from 0.5s at minimum intensity down to 0.005s at maximum.
- **Decay** is a function of material thickness and absorption. A thick, opaque material absorbs more light and produces a longer decay.
- **Sustain** is mapped to the index of refraction (IOR), normalized against Diamond — the densest material at 2.42. Higher IOR means more internal reflections, which means a higher sustain level. Diamond sits near 0.95; Water around 0.55.
- **Release** combines IOR and thickness. A dense, thick material traps light longer, producing a slower release.

Every value recalculates automatically whenever you change the material, adjust the thickness, or move the lights. The envelope becomes a property of the scene, not a separate set of controls.

---

## Light as a musical instrument

The **Lights Bar** is one of Elements' most expressive features. Each light source has an intensity parameter that is directly mapped to pitch — at 50% intensity the pitch is neutral, below that it drops, above it rises. This means you can use light intensity as a performance parameter, creating pitch movement that feels organic rather than mechanical.

This bidirectional relationship between light and pitch is at the heart of what makes Elements unusual. You're not modulating pitch with an LFO. You're changing how much light hits a surface, and the physics take care of the rest.

---

*Elements is currently in beta. Windows support is planned for v1.1.*  
*macOS 12 or later · Apple Silicon and Intel native*
