---
layout: default
title: Quick Start · Elements
---

[← Back to Elements](index)

# Quick Start

Your first sound in five steps.

---

## 1. Load Elements in your DAW

Open Elements as a VST3 or AU instrument. If this is your first time, refer to the [Installation guide](installation) for setup instructions including the Gatekeeper bypass required on macOS.

---

## 2. Choose a material and geometry

At the top of the interface, select a **Geometry** (GEO) and a **Material** (MAT) from the dropdown menus.

If you are not sure where to start, use **Sphere + Diamond**. Diamond has the broadest transmission curve and responds well to any light source, and the Sphere produces the smoothest, most immediate sound.

---

## 3. Turn on a light

Elements produces no sound without an active light source. In the Lights Bar at the bottom of the interface:

1. Check the **Key** checkbox to enable the first light slot
2. Select **Daylight** as the light source: it has the broadest emission curve and works with almost every material
3. Set the intensity to **0.5** to start at a neutral pitch

You should now hear sound when you play a note.

> If you hear nothing, verify that the Key light checkbox is checked and that the selected material is compatible with your light source. Some combinations produce near-silence by design; see [Materials & Geometry](materials-and-geometry) for the compatibility guide.

---

## 4. Play with light intensity

While holding a note, move the **Key Intensity** slider. Notice that the pitch responds directly: above 0.5 it rises, below 0.5 it falls. This bidirectional relationship between light and pitch is one of Elements' most expressive performance parameters. Try automating it in your DAW for continuous pitch movement that feels organic rather than mechanical.

---

## 5. Activate the Deformer (Sphere only)

If you have Sphere selected, slowly raise the **Deform** slider. The surface of the sphere begins to deform under an animated noise field (Simplex by default; try Alligator or Worley from the **Noise** selector for a different character). Displaced normals introduce timbral variation, a continuous harmonic drift sets in, and the sinusoidal wavefolder starts adding harmonic density. Push it to around 0.4 for a rich, living texture. Pull it back to 0 for a clean, stable tone.

This is Elements at its most expressive.

---

## Factory presets

Elements ships with 15 factory presets across five categories: fully-tuned starting points, not blank-slate examples. They're available immediately after installation and can't be deleted or overwritten from the UI, so there's always a known-good state to return to.

Open the **PRESET** dropdown at the top of the interface to browse them, grouped by category:

- **Bass**: Sub Womb, Deep Current, Resonant Fang
- **Lead**: Diamond Lead, Gold Spike, Hollow Lead
- **Pad**: Amber Pad, Amethyst Veil, Copper Bloom
- **Drone**: Obsidian Drone, Alexandrite Hum, Teapot Void
- **Choir**: Water Choir, Molten Choir, Crystal Choir

Load one, play a few notes, then open the panels and see how MAT/GEO, lights, and envelope mode combine to produce it. This is the fastest way to hear Elements at its full expressive range without dialing in a scene from scratch. Once you start tweaking, save your own variations with the SAVE button; the factory presets themselves stay untouched no matter what you change.

---

## Next steps

- Explore the [Materials & Geometry](materials-and-geometry) page to understand how each combination shapes the sound
- Read the full [Parameters](parameters) reference
- Automate **Rotation X/Y/Z** and **light intensities** from your DAW for continuous timbral movement
- Try combining all three lights with different sources and intensities. The spectral interactions between them are where Elements gets interesting

---

[← Back to Elements](index)
