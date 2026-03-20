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

Geometry determines how many surface normals interact with the light and how their Fresnel contributions are weighted — which directly defines how much timbral variation rotation produces.

---

**Cube** — 6 faces

The most aggressive geometry. Six axis-aligned normals with a steep weighting (cos⁴) — the face pointing most directly at the light dominates almost completely. Rotating the cube produces discrete jumps between six distinct spectral positions rather than smooth transitions. Good for rhythmic or sequenced sounds where automation produces clear, differentiated changes.

> Angular, punchy. Dramatic timbral jumps on rotation.

---

**Sphere** — continuous surface

Without the Deformer, the sphere does not respond to rotation at all — a perfect sphere always presents the same curved surface to the light from any angle. Stable, smooth, predictable. When the **Deformer** is active, the symmetry breaks: 12 displaced normals introduce rotation sensitivity, continuous timbral drift sets in, and the wavefolding adds harmonic density. The deformed sphere has the highest expressive range of any geometry.

> Stable and uniform without Deformer. Maximum movement with Deformer active.

---

**Torus** — 12 normals + concave/convex geometry

The torus simulates caustics on its inner concave surface — light concentrates as it reflects off the cavity, producing a gaussian boost centered on the mid harmonics. This caustic emphasis is unique to the torus and gives it a inherently rich midrange character that no other geometry produces. Rotation is smooth and continuous thanks to 12 normals with quadratic weighting.

> Rich, evolving. Caustic midrange emphasis. The most spectrally complex geometry without Deformer.

---

**Dodecahedron** — 12 pentagonal faces

Twelve faces distributed uniformly across the sphere using icosahedral symmetry. Like the cube, each face is flat and uses direct Fresnel — but with 12 faces instead of 6, and no axis alignment, the dominant face shifts gradually rather than jumping. Multiple faces always contribute to the spectrum simultaneously, producing a dense, intricate timbre.

> Dense, multifaceted. Continuous variation without the sharp jumps of the Cube.

---

| | Faces | Rotation response | Character |
|---|---|---|---|
| **Cube** | 6 (axis-aligned) | Discrete, dramatic | Angular, punchy |
| **Sphere** | Continuous | None (or full with Deformer) | Smooth, stable |
| **Torus** | 12 + caustics | Continuous, smooth | Rich, mid-heavy |
| **Dodecahedron** | 12 (uniform) | Continuous, textured | Dense, intricate |

**Sound design rule:** Cube for stepped timbral changes, Dodecahedron for constant texture, Torus for midrange richness, Sphere for stability — or maximum movement with the Deformer.

---

[← Back to Elements](index)
