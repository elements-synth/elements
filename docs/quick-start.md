---
layout: default
title: Quick Start · Elements
---

[← Back to Elements](index)

# Quick Start

Your first sounds, step by step.

---

## 1. Load Elements in your DAW

Open Elements as a VST3 or AU instrument (or run the Standalone app). If this is your first time, refer to the [Installation guide](installation) for setup instructions including the Gatekeeper bypass required on macOS.

A new instance starts with a ready-made scene: **Diamond** on a **Sphere**, lit by the **Key** light (Sunset, intensity 0.5). Play a note and you will already hear it.

---

## 2. Choose a material and geometry

The 3D viewport has two collapsible headers along its top edge, **GEOMETRIES** and **MATERIALS**. Click one to open its panel.

- In **MATERIALS**, pick a material from the **MAT A** dropdown.
- In **GEOMETRIES**, pick a shape from the **GEO** dropdown (Cube, Sphere, Torus, Dodeca or Teapot).

<figure class="clip">
  <img src="{{ '/assets/images/quick-start/materials.gif' | relative_url }}" alt="Opening the GEOMETRIES and MATERIALS panels in Elements and switching the material and geometry" loading="lazy">
  <figcaption>Open MATERIALS and GEOMETRIES, then pick a material from MAT A and a shape from GEO: the object in the viewport changes at once.</figcaption>
</figure>

If you are not sure where to start, use **Sphere + Diamond**. Diamond has the broadest transmission curve and responds well to any light source, and the Sphere produces the smoothest, most immediate sound.

---

## 3. Choose and balance your lights

Elements produces no sound without an active light source. The **LIGHTS** bar at the bottom of the viewport has three slots, **Key**, **Fill** and **Rim**. Each has a checkbox to switch it on, a source dropdown and an intensity slider.

1. The **Key** light is already on. To add more light, tick **Fill** and/or **Rim**
2. Pick a source for each: **Sunset** (warm), **Daylight** (broad, works with almost every material) or **LED Cool** (bright)
3. Leave the intensities at **0.5** to start at a neutral pitch

> If you hear nothing, check that at least one light is ticked and that the intensity is not at zero. Some material and light pairings sound thin by design; see [Materials & Geometry](materials-and-geometry) for how each material responds to each light.

---

## 4. Play with light intensity

While holding a note, move the **Key Intensity** slider. The pitch responds directly: above 0.5 it rises, below 0.5 it falls, by up to two semitones in either direction. This is one of Elements' most expressive performance parameters. Try automating it in your DAW for continuous pitch movement that feels organic rather than mechanical.

---

## 5. Rotate the object

Grab one of the three coloured rings around the object in the viewport and drag it, or double-click the **rotX**, **rotY** and **rotZ** fields to type an angle, or automate **Rotation X / Y / Z** from your DAW. (The right mouse button orbits the camera, which changes only what you see, not the sound.) Rotation changes the angle at which the light meets the surface, and so the timbre. The Cube changes in sharp steps and the Torus strongly but smoothly. The undeformed Sphere does not respond at all, which leads to the next step.

---

## 6. Activate the Deformer (Sphere only)

With the Sphere selected, open **GEOMETRIES** and slowly raise **Deform**. The surface begins to deform under an animated noise field (Simplex by default; try Alligator or Worley from the **Noise** selector). Displaced normals introduce timbral variation, the harmonics drift in level and phase, and a wavefolder adds harmonic density. **Freq** sets how fine the surface detail is and **Rate** how fast it moves (0 freezes it). Push Deform to around 0.4 for a rich, living texture, and pull it back to 0 for a clean, stable tone.

This is Elements at its most expressive.

---

## 7. Add a second material

Each voice has two oscillators. In **MATERIALS**, choose a different material for **MAT B**, pick a **BLEND** mode (Ring Mod, AM, XOR or FM) and raise **MIX** above 0%. The two materials now interact: try Ring Mod for metallic, inharmonic tones or AM for pulsing ones, and use **DETUNE** and **MUTE A** to hear what B adds. The arrows button between the two selectors swaps A and B.

---

## 8. Try the Chorus

Tick **Chorus** at the top left of the viewport. The object is duplicated into a trail of copies receding behind it, and each copy is a delayed, quieter echo of the sound. **Voices**, **Spread**, **Decay** and **Wobble** control how many copies there are, how far apart, how fast they fade, and how much they drift.

---

## Factory presets

Elements ships with 15 factory presets across five categories: fully-tuned starting points, not blank-slate examples. They're available immediately after installation and can't be deleted or overwritten from the UI, so there's always a known-good state to return to.

Open the **PRESET** dropdown at the top of the interface to browse them, grouped by category:

- **Bass**: Sub Womb, Deep Current, Resonant Fang
- **Lead**: Diamond Lead, Gold Spike, Hollow Lead
- **Pad**: Amber Pad, Amethyst Veil, Copper Bloom
- **Drone**: Obsidian Drone, Alexandrite Hum, Teapot Void
- **Choir**: Water Choir, Molten Choir, Crystal Choir

Load one, play a few notes, then open the panels and see how the materials, geometry, lights, and envelope mode combine to produce it. This is the fastest way to hear Elements at its full expressive range without dialing in a scene from scratch. Once you start tweaking, save your own variations with the SAVE button; the factory presets themselves stay untouched no matter what you change.

---

## Next steps

- Explore the [Materials & Geometry](materials-and-geometry) page to understand how each combination shapes the sound
- Read the full [Parameters](parameters) reference
- Automate **Rotation X/Y/Z** and **light intensities** from your DAW for continuous timbral movement
- Try combining all three lights with different sources and intensities. The spectral interactions between them are where Elements gets interesting

---

[← Back to Elements](index)
