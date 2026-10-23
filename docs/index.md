---
layout: default
title: Elements
description: A synthesizer where light meets sound.
---

**[Download v1.0.0 for macOS & Windows](https://github.com/elements-synth/elements/releases/latest)**
*VST3 · macOS Universal Binary (Apple Silicon + Intel) · Windows x64 · Free*

[Quick Start](quick-start) · [Installation](installation) · [Materials & Geometry](materials-and-geometry) · [Parameters](parameters) · [Known Issues](known-issues) · [The Science](science)

---

## Video demos

<div class="video-grid">
  <a class="video-card" href="https://www.youtube.com/watch?v=lJhoasqMsv8">
    <img src="{{ '/assets/images/videos/deformer.jpg' | relative_url }}" alt="Video: the Elements Deformer reshaping an amethyst sphere" loading="lazy">
    <span class="video-title">The Deformer</span>
    <span class="video-desc">Simplex Noise + Wavefolding in action</span>
  </a>
  <a class="video-card" href="https://www.youtube.com/watch?v=kIsSFacwmfY">
    <img src="{{ '/assets/images/videos/light-material.jpg' | relative_url }}" alt="Video: light and material in Elements" loading="lazy">
    <span class="video-title">Light × Material</span>
    <span class="video-desc">Light intensity controls pitch, material controls timbre</span>
  </a>
  <a class="video-card" href="https://www.youtube.com/watch?v=xloh2gQo6ps">
    <img src="{{ '/assets/images/videos/geometry-rotation.jpg' | relative_url }}" alt="Video: geometry and rotation in Elements" loading="lazy">
    <span class="video-title">Geometry × Rotation</span>
    <span class="video-desc">Same material, same light, four different sounds</span>
  </a>
  <a class="video-card" href="https://www.youtube.com/watch?v=WOz9LchJtqY">
    <img src="{{ '/assets/images/videos/viewport.jpg' | relative_url }}" alt="Video: the Elements 3D viewport" loading="lazy">
    <span class="video-title">3D Viewport</span>
    <span class="video-desc">The instrument in three dimensions</span>
  </a>
</div>

---

## What is Elements?

Elements is a spectral wavetable synthesizer built around a simple but unusual idea: what if a synthesizer was a physical scene, and sound was what happened when light hit matter?

In traditional synthesis, you start with a waveform and shape it: filter it, envelope it, modulate it. Elements takes a different approach. You begin by choosing a **material** and a **geometry**. A cube of diamond. A sphere of water. A torus of amber. A dodecahedron of obsidian. Then you shine a **light** on it. You can also introduce a second material entirely, blending two independent optical scenes into one voice.

The sound you hear is the result of that interaction: the specific combination of light intensity, geometry, and material properties. Change any one of them and the sound changes, the way a render changes when you move a light or swap a shader. The synthesizer is not a signal chain. It's a physical scene.

This makes Elements feel different from anything else you've used. Sound design stops being a matter of turning the right knobs in the right order, and starts feeling more like setting up a shot: placing objects, adjusting light, watching (and hearing) how everything responds to each other.

---

## Thirteen materials

Every material is a real optical curve: gems, minerals, two metals and water. Click any of them to see how it sounds and why.

<div class="gallery">
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/diamond-sphere.png' | relative_url }}" alt="Diamond sphere" loading="lazy">
    <span>Diamond</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/water-sphere.png' | relative_url }}" alt="Water sphere" loading="lazy">
    <span>Water</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/amber-sphere.png' | relative_url }}" alt="Amber sphere" loading="lazy">
    <span>Amber</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/ruby-sphere.png' | relative_url }}" alt="Ruby sphere" loading="lazy">
    <span>Ruby</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/gold-sphere.png' | relative_url }}" alt="Gold sphere" loading="lazy">
    <span>Gold</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/emerald-sphere.png' | relative_url }}" alt="Emerald sphere" loading="lazy">
    <span>Emerald</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/amethyst-sphere.png' | relative_url }}" alt="Amethyst sphere" loading="lazy">
    <span>Amethyst</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/sapphire-sphere.png' | relative_url }}" alt="Sapphire sphere" loading="lazy">
    <span>Sapphire</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/copper-sphere.png' | relative_url }}" alt="Copper sphere" loading="lazy">
    <span>Copper</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/obsidian-sphere.png' | relative_url }}" alt="Obsidian sphere" loading="lazy">
    <span>Obsidian</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/alexandrite-sphere.png' | relative_url }}" alt="Alexandrite sphere" loading="lazy">
    <span>Alexandrite</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/malachite-sphere.png' | relative_url }}" alt="Malachite sphere" loading="lazy">
    <span>Malachite</span>
  </a>
  <a class="gallery-item" href="materials-and-geometry">
    <img src="{{ '/assets/images/materials/neodymium-sphere.png' | relative_url }}" alt="Neodymium sphere" loading="lazy">
    <span>Neodymium</span>
  </a>
</div>

---

## Two materials, one voice

Elements runs two independent oscillators, A and B, each shaped by its own material (they share the same geometry, rotation and lights). A **BLEND** mode decides how their spectra combine at the sample level:

- **Ring Mod**: multiplies A and B sample-by-sample, producing sum/difference sidebands that exist in neither source spectrum. Inharmonic and metallic, strongest with contrasting materials.
- **AM**: B modulates the amplitude of A. At low depth you hear A enriched with new harmonics; the original timbre stays recognizable.
- **XOR**: subtracts B from A, so where the two oscillators agree the output cancels, and where their spectra differ it is loudest. Subtractive and hollow.
- **FM**: B modulates the phase of A before wavetable readout, from subtle pitch drift at low depth to dense, DX-style inharmonic spectra at high depth.

**MIX** crossfades between pure A and the blended result, **DETUNE** offsets oscillator B's pitch (±100 cents), and **DEPTH** controls modulation intensity for AM and FM. Mute A to audition B in isolation. The result is a genuinely two-voice instrument: two separate optical scenes, combined in real time.

<figure class="clip">
  <img src="{{ '/assets/images/science/blend-modes.png' | relative_url }}" alt="Waveforms of oscillators A and B and the Ring Mod, AM, XOR and FM results" loading="lazy">
  <figcaption>Ruby (A) and Emerald (B) through each blend mode: same two materials, four very different waves.</figcaption>
</figure>

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
