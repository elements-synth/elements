---
layout: default
title: Materials & Geometry · Elements
---

[← Back to Elements](index)

# Materials & Geometry

The sound of Elements emerges from the interaction of three ingredients: a **geometry**, a **material**, and a **light source**. The specific combination of all three determines what you hear: change any one of them and the result changes entirely. This page explains what each ingredient brings to that interaction.

---

## How materials work

Each material has a transmission curve: a description of which wavelengths of light pass through it and which are absorbed. In Elements, wavelengths map directly to harmonics: red light (650–780nm) produces low harmonics, blue and violet light (380–450nm) produces high harmonics.

The final spectrum you hear is the result of three factors multiplied together:

```
sound = light emission × material transmission × fresnel response
```

This means the choice of light source matters as much as the choice of material. A material that transmits only red light will sound noticeably duller under a cool blue LED, because there is little spectral overlap between what the light emits and what the material allows through.

---

## Materials

### Transparent gems

These materials have high transmission and produce rich, harmonically complex sounds.

**<span class="mat-diamond">Diamond</span>** · IOR 2.42 · Avg. transmission ~96%

The most versatile material in Elements. Its transmission curve is uniformly high across the entire visible spectrum, meaning all harmonics are present with similar strength. The highest IOR of any material produces the most pronounced Fresnel response: rotation has a strong effect on timbre. Works well with all three light sources.

> Crystalline, bright. Full harmonic spectrum. Maximum Fresnel sensitivity.

---

**<span class="mat-sapphire">Sapphire</span>** · IOR 1.77 · Avg. transmission ~40%

Transmits strongly in blue, with a sharp cutoff above 550nm. Pairs best with LED Cool and Daylight. Under Sunset there is little spectral overlap and the material sounds noticeably duller. The narrow transmission window produces a focused, airy sound.

> Clear, bright. High harmonics only. Noticeably duller under Sunset.

---

**<span class="mat-emerald">Emerald</span>** · IOR 1.57 · Avg. transmission ~37%

Has a narrow transmission window centered on green (500–550nm), absorbing both blue and red. This spectral focus produces a characteristic midrange-only timbre: nasal, focused, unlike most other materials. Daylight is the natural pairing, with its emission peak centered exactly on <span class="mat-emerald">Emerald</span>'s transmission window.

> Balanced, focused. Midrange harmonics only. Nasal character.

---

**<span class="mat-malachite">Malachite</span>** · IOR 1.85 · Avg. transmission ~43%

Cu²⁺ charge-transfer and d-d absorption cut both blue and red, leaving a narrow green transmission window similar to <span class="mat-emerald">Emerald</span>'s. Produces the same nasal, midrange-focused character, though the window sits slightly lower and the falloff is steeper on the red side.

> Focused, mineral. Narrow green window. <span class="mat-emerald">Emerald</span>'s steeper-edged cousin.

---

**<span class="mat-amethyst">Amethyst</span>** · IOR 1.54 · Avg. transmission ~44%

The most complex transmission curve among the gems: bimodal, transmitting both violet and some red while absorbing green and yellow. This produces a hollow character: high and low harmonics present, midrange absent. Responds differently to each light source: LED Cool emphasizes the highs, Sunset brings out the lows, Daylight activates both simultaneously.

> Complex, hollow. High and low harmonics, midrange absent. Changes character significantly with each light source.

---

**<span class="mat-alexandrite">Alexandrite</span>** · IOR 1.745 · Avg. transmission ~53%

Dual-peak transmission: a green window (490–570nm) and a separate red window above 640nm, with a dip between them. The classic "alexandrite effect": which window dominates the combined spectrum shifts audibly with the light source, cool sources emphasizing the green, warm sources the red. Two distinct harmonic clusters rather than one continuous band.

> Complex, color-shifting. Two harmonic clusters. Character depends heavily on light source.

---

### Warm gems and minerals

Materials with transmission weighted toward the red end of the spectrum, producing warmer, heavier sounds.

**<span class="mat-ruby">Ruby</span>** · IOR 1.77 · Avg. transmission ~37%

Near-zero transmission until 600nm, then a sharp jump into high transmission in the red range. Pairs best with Sunset; under LED Cool it sounds duller and less saturated. Produces a rich, saturated sound dominated by the fundamental and low harmonics; high harmonics are almost completely eliminated.

> Rich, saturated. Strong fundamental, minimal highs. Sounds best under Sunset.

---

**<span class="mat-amber">Amber</span>** · IOR 1.55 · Avg. transmission ~56%

Near-zero in blue, rising gradually toward red. A natural complement to Sunset light. Under LED Cool it sounds noticeably duller. The gradual transmission curve produces a warm, organic character with emphasis on low and mid harmonics and an absence of highs.

> Warm, organic. Low and mid harmonics. Pairs naturally with Sunset.

---

### Metals

Metals behave differently from gems: their IOR values fall below 1.0, which means they reflect rather than refract. The Fresnel response is much softer, and the timbral character is distinctly metallic.

**<span class="mat-gold">Gold</span>** · IOR 0.47 · Avg. transmission ~58%

Near-zero transmission in blue, with an abrupt jump around 550nm: the interband transition characteristic of gold. Works well with Sunset and Daylight. The low IOR produces a softer Fresnel effect than gems. Warm, mid-heavy timbre.

> Metallic, warm. Mid and low harmonics. Soft Fresnel response.

---

**<span class="mat-copper">Copper</span>** · IOR 0.46 · Avg. transmission ~30%

Even more extreme than <span class="mat-gold">Gold</span>: only deep red and near-infrared light passes through. Pairs best with Sunset, and sounds much duller under LED Cool. The most bass-heavy material in Elements, producing only the fundamental and first few harmonics.

> Extremely warm, deep. Fundamental and lowest harmonics only. Sounds best under Sunset.

---

### Special

**<span class="mat-water">Water</span>** · IOR 1.33 · Avg. transmission ~72%

High transmission in blue and green, dropping sharply above 600nm due to O-H absorption. Produces prominent high harmonics with attenuated lows, a warm roll-off in the opposite direction to most warm materials. Best with LED Cool and Daylight. Somewhat duller under Sunset.

> Soft, fluid. High harmonics prominent, lows attenuated. Warm roll-off.

---

**<span class="mat-obsidian">Obsidian</span>** · IOR 1.50 · Avg. transmission ~16%

The darkest material in Elements. Nearly opaque: only deep red light passes through, and only in thin sections. Its raw transmission is the lowest of any material, but Elements compensates with gain correction, so its loudness stays comparable to other materials. What stays distinct is the harmonic content: minimal and muted rather than a full spectrum. Use it for textural, atmospheric sound design rather than melodic content.

> Dark, minimal. Very few harmonics, muted presence.

---

**<span class="mat-neodymium">Neodymium</span>** · IOR 1.636 · Avg. transmission ~68%

Rare-earth glass with six narrow f-f absorption bands scattered across the visible spectrum (~432/522/583/625/677/741nm), rather than one broad cutoff. Produces a comb-filter transmission curve, alternating narrow peaks and notches, giving it a harmonically dense character unlike any other material in Elements.

> Complex, harmonically dense. Comb-filter spectrum. Unlike any other material.

---

## Light sources

The three light sources have Gaussian emission curves centered on different parts of the visible spectrum:

| Light source | Peak | Character |
|---|---|---|
| **Sunset** | 650nm (red) | Warm, narrow |
| **Daylight** | 550nm (green/yellow) | Broad, versatile |
| **LED Cool** | 470nm (blue) | Cool, focused |

How much a light's emission overlaps with a material's transmission curve shapes both timbre and loudness: strong overlap gives a fuller, brighter sound, while a mismatched pairing sounds duller and more muted. Elements' lights always retain some output across the full spectrum and material loudness is gain-compensated, so no combination ever goes silent, they just sound different. See each material's description above for how it responds to the three light sources.

**General rule:**
- Cold materials (<span class="mat-sapphire">Sapphire</span>, <span class="mat-water">Water</span>, <span class="mat-diamond">Diamond</span>) sound best under LED Cool / Daylight
- Warm materials (<span class="mat-ruby">Ruby</span>, <span class="mat-amber">Amber</span>, <span class="mat-copper">Copper</span>, <span class="mat-gold">Gold</span>) sound best under Sunset / Daylight
- Complex materials (<span class="mat-amethyst">Amethyst</span>, <span class="mat-emerald">Emerald</span>) produce a distinctly different character under each light
- Daylight is the most versatile source due to its broad emission

---

## Geometry

Geometry determines how many surface normals interact with the light and how their Fresnel contributions are weighted, which directly defines how much timbral variation rotation produces.

---

**Cube**: 6 faces

The most aggressive geometry. Six axis-aligned normals with a steep weighting (cos⁴): the face pointing most directly at the light dominates almost completely. Rotating the cube produces discrete jumps between six distinct spectral positions rather than smooth transitions. Good for rhythmic or sequenced sounds where automation produces clear, differentiated changes.

> Angular, punchy. Dramatic timbral jumps on rotation.

---

**Sphere**: continuous surface

Without the Deformer, the sphere does not respond to rotation at all: a perfect sphere always presents the same curved surface to the light from any angle. Stable, smooth, predictable. When the **Deformer** is active, the symmetry breaks: 32 displaced normals introduce rotation sensitivity, continuous timbral drift sets in, and the wavefolding adds harmonic density. The deformed sphere has the highest expressive range of any geometry.

> Stable and uniform without Deformer. Maximum movement with Deformer active.

---

**Torus**: 12 normals + concave/convex geometry

The torus simulates caustics on its inner concave surface: light concentrates as it reflects off the cavity, producing a gaussian boost centered on the mid harmonics. This caustic emphasis is unique to the torus and gives it a inherently rich midrange character that no other geometry produces. Rotation is smooth and continuous thanks to 12 normals with quadratic weighting.

> Rich, evolving. Caustic midrange emphasis. The most spectrally complex geometry without Deformer.

---

**Dodecahedron**: 12 pentagonal faces

Twelve faces distributed uniformly across the sphere using icosahedral symmetry. Like the cube, each face is flat and uses direct Fresnel, but with 12 faces instead of 6, and no axis alignment, the dominant face shifts gradually rather than jumping. Multiple faces always contribute to the spectrum simultaneously, producing a dense, intricate timbre.

> Dense, multifaceted. Continuous variation without the sharp jumps of the Cube.

---

**Teapot**: 28 Bezier-patch normals

The only asymmetric geometry in Elements. Twenty-eight Bezier patches sample surface normals from the spout, handle, body, and lid, each contributing a differently-weighted Fresnel response. Unlike the uniform solids above, rotation produces an irregular, non-repeating sequence of timbral changes rather than a predictable cycle.

> Asymmetric, detailed. Irregular timbral variation. The most complex static geometry.

---

| | Faces | Rotation response | Character |
|---|---|---|---|
| **Cube** | 6 (axis-aligned) | Discrete, dramatic | Angular, punchy |
| **Sphere** | Continuous | None (or full with Deformer) | Smooth, stable |
| **Torus** | 12 + caustics | Continuous, smooth | Rich, mid-heavy |
| **Dodecahedron** | 12 (uniform) | Continuous, textured | Dense, intricate |
| **Teapot** | 28 (Bezier patches) | Continuous, irregular | Asymmetric, detailed |

**Sound design rule:** Cube for stepped timbral changes, Dodecahedron for constant texture, Torus for midrange richness, Teapot for irregular asymmetric detail, Sphere for stability (or maximum movement with the Deformer).

---

[← Back to Elements](index)
