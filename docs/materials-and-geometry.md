---
layout: default
title: Materials & Geometry — Elements
---

# Materials & Geometry

The sound of Elements emerges from the interaction of three ingredients: a **geometry**, a **material**, and a **light source**. The specific combination of all three determines what you hear — change any one of them and the result changes entirely. This page explains what each ingredient brings to that interaction.

---

## How materials work

Each material has a transmission curve — a description of which wavelengths of light pass through it and which are absorbed. In Elements, wavelengths map directly to harmonics: red light (650–780nm) produces low harmonics, blue and violet light (380–450nm) produces high harmonics.

The final spectrum you hear is the result of three factors multiplied together:

```
sound = light emission × material transmission × fresnel response
```

This means the choice of light source matters as much as the choice of material. A material that transmits only red light will produce almost no sound under a cool blue LED — because there is no spectral overlap between what the light emits and what the material allows through.

---

## Materials

### Transparent gems

These materials have high transmission and produce rich, harmonically complex sounds.

**Diamond** · IOR 2.42 · Avg. transmission ~96%

The most versatile material in Elements. Its transmission curve is uniformly high across the entire visible spectrum, meaning all harmonics are present with similar strength. The highest IOR of any material produces the most pronounced Fresnel response — rotation has a strong effect on timbre. Works well with all three light sources.

> Crystalline, bright. Full harmonic spectrum. Maximum Fresnel sensitivity.

---

**Sapphire** · IOR 1.77 · Avg. transmission ~40%

Transmits strongly in blue, with a sharp cutoff above 550nm. Pairs almost exclusively with LED Cool and Daylight — with Sunset there is virtually no spectral overlap and the material falls nearly silent. The narrow transmission window produces a focused, airy sound.

> Clear, bright. High harmonics only. Almost silent under Sunset.

---

**Emerald** · IOR 1.57 · Avg. transmission ~37%

Has a narrow transmission window centered on green (500–550nm), absorbing both blue and red. This spectral focus produces a characteristic midrange-only timbre — nasal, focused, unlike most other materials. Daylight is the natural pairing, with its emission peak centered exactly on Emerald's transmission window.

> Balanced, focused. Midrange harmonics only. Nasal character.

---

**Amethyst** · IOR 1.54 · Avg. transmission ~44%

The most complex transmission curve in Elements: bimodal, transmitting both violet and some red while absorbing green and yellow. This produces a hollow character — high and low harmonics present, midrange absent. Responds differently to each light source: LED Cool emphasizes the highs, Sunset brings out the lows, Daylight activates both simultaneously.

> Complex, hollow. High and low harmonics, midrange absent. Changes character significantly with each light source.

---

### Warm gems and minerals

Materials with transmission weighted toward the red end of the spectrum, producing warmer, heavier sounds.

**Ruby** · IOR 1.77 · Avg. transmission ~37%

Near-zero transmission until 600nm, then a sharp jump into high transmission in the red range. Pairs almost exclusively with Sunset. Under LED Cool it is practically silent. Produces a rich, saturated sound dominated by the fundamental and low harmonics — high harmonics are almost completely eliminated.

> Rich, saturated. Strong fundamental, minimal highs. Sunset only.

---

**Amber** · IOR 1.55 · Avg. transmission ~56%

Near-zero in blue, rising gradually toward red. A natural complement to Sunset light. Under LED Cool it becomes very faint. The gradual transmission curve produces a warm, organic character with emphasis on low and mid harmonics and an absence of highs.

> Warm, organic. Low and mid harmonics. Pairs naturally with Sunset.

---

### Metals

Metals behave differently from gems — their IOR values fall below 1.0, which means they reflect rather than refract. The Fresnel response is much softer, and the timbral character is distinctly metallic.

**Gold** · IOR 0.47 · Avg. transmission ~58%

Near-zero transmission in blue, with an abrupt jump around 550nm — the interband transition characteristic of gold. Works well with Sunset and Daylight. The low IOR produces a softer Fresnel effect than gems. Warm, mid-heavy timbre.

> Metallic, warm. Mid and low harmonics. Soft Fresnel response.

---

**Copper** · IOR 0.46 · Avg. transmission ~30%

Even more extreme than Gold — only deep red and near-infrared light passes through. Pairs exclusively with Sunset. The most bass-heavy material in Elements, producing only the fundamental and first few harmonics.

> Extremely warm, deep. Fundamental and lowest harmonics only. Sunset only.

---

### Special

**Water** · IOR 1.33 · Avg. transmission ~72%

High transmission in blue and green, dropping sharply above 600nm due to O-H absorption. Produces prominent high harmonics with attenuated lows — a warm roll-off in the opposite direction to most warm materials. Best with LED Cool and Daylight. With Sunset it loses most of its brightness.

> Soft, fluid. High harmonics prominent, lows attenuated. Warm roll-off.

---

**Obsidian** · IOR 1.50 · Avg. transmission ~16%

The darkest material in Elements. Nearly opaque — only deep red light passes through, and only in thin sections. Even under Sunset it produces a very faint signal. Minimal harmonics, a barely-there presence. Use it for textural, atmospheric sound design rather than melodic content.

> Dark, minimal. Very few harmonics. The quietest material.

---

## Light × Material compatibility

The three light sources have Gaussian emission curves centered on different parts of the visible spectrum:

| Light source | Peak | Character |
|---|---|---|
| **Sunset** | 650nm (red) | Warm, narrow |
| **Daylight** | 550nm (green/yellow) | Broad, versatile |
| **LED Cool** | 470nm (blue) | Cool, focused |

Sound is only produced where the light emission overlaps with the material's transmission. No overlap means near-silence.

| Material | Sunset | Daylight | LED Cool |
|---|---|---|---|
| Diamond | ✓ | ✓ | ✓ |
| Water | △ | ✓ | ✓ |
| Amber | ✓ | △ | ✗ |
| Ruby | ✓ | △ | ✗ |
| Gold | ✓ | ✓ | △ |
| Emerald | △ | ✓ | △ |
| Amethyst | ✓ | ✓ | ✓ |
| Sapphire | ✗ | ✓ | ✓ |
| Copper | ✓ | △ | ✗ |
| Obsidian | △ | ✗ | ✗ |

*✓ Strong · △ Partial · ✗ Near-silence*

**General rule:**
- Cold materials (Sapphire, Water, Diamond) → LED Cool / Daylight
- Warm materials (Ruby, Amber, Copper, Gold) → Sunset / Daylight
- Complex materials (Amethyst, Emerald) → any light produces a different character
- Daylight is the most versatile source due to its broad emission (it overlaps with almost everything)

---

## Geometry

The geometry determines how the material presents itself to the light — the number of faces, their angles, and the nature of the surface all shape the timbral response.

**Cube**

Six faces meeting at sharp 90° angles. With only 8 vertices, the transitions between faces are abrupt — the Fresnel response jumps rather than glides as the geometry rotates. Produces a narrow, focused timbral range with sudden changes at specific rotation angles. Good for percussive, angular sounds.

> Few harmonics, abrupt transitions. Limited timbral range. Rotation produces sudden jumps.

---

**Sphere**

The only geometry with no faces or edges — a continuous, smooth surface. Without the Deformer active, the sphere presents the same curvature to the light at every angle, meaning rotation has almost no effect on timbre. When the **Deformer** is active, the smooth surface breaks open: displaced normals create variation across the surface, timbral drift sets in, and the sphere becomes the most expressive geometry in Elements.

> Uniform, stable. Minimal timbral variation without Deformer. Maximum expressiveness with Deformer active.

---

**Torus**

A continuous surface like the sphere, but with inner and outer curvature — the inner ring faces the light differently than the outer ring. This creates more timbral variation than the sphere at rest, with a smoother transition profile than the cube. A middle ground between stability and movement.

> Similar to Sphere but with more built-in timbral variation. Smooth transitions.

---

**Dodecahedron**

Twelve pentagonal faces — far more than the cube, but still faceted. The higher face count means the transitions between angles are more gradual and the timbral range is wider than the cube. Think of it as a faceted sphere: more variation than a cube, more movement than a sphere at rest, with a character somewhere between geometric and organic.

> Wide timbral range. More gradual transitions than Cube. Between geometric and organic character.

---

[← Back to Elements](index)
