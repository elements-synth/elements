---
layout: default
title: Materials & Geometry · Elements
---

[← Back to Elements](index)

# Materials & Geometry

The sound of Elements emerges from the interaction of three ingredients: a **geometry**, a **material**, and a **light source**. The specific combination of all three determines what you hear: change any one of them and the result changes entirely. This page explains what each ingredient brings to that interaction.

---

## How materials work

Each material has a spectral curve: how much of each wavelength gets through (for the gems, minerals and water) or is reflected back (for the two metals). Elements stores it as 32 points from 380 to 780nm. Wavelengths map directly to harmonics: red light (780nm) is harmonic 1, violet (380nm) is harmonic 20, and there is roughly one harmonic for every 21nm in between. Red light produces low harmonics, blue and violet light produces high ones.

The final spectrum you hear is the result of three factors multiplied together:

```
sound = light emission × material curve × fresnel response
```

This means the choice of light source matters as much as the choice of material. A material that transmits only red light will sound noticeably duller under a cool blue LED, because there is little spectral overlap between what the light emits and what the material allows through.

**Thickness** raises each material's curve to a power: a thicker piece absorbs more of the wavelengths it already blocks, so its character gets stronger and more focused, and a thinner one gets closer to a flat spectrum. It has no effect on Gold and Copper, which are opaque. The maths is on [The Science](science).

---

## Reading the cards

Each material below comes with two pictures.

- **The sphere** is the real Elements viewport render of that material (sphere geometry, Daylight on the Key and Fill lights).
- **The wavetable** is the set of ten band-limited wavetables Elements builds for that material, drawn Bitwig-style as a stack of single-cycle waveforms. The front line is the table used for the lowest notes, which holds the most harmonics; each line behind it is the next octave up, with fewer harmonics, until the last lines are nearly pure sine waves. The differences between materials are easiest to see in the front lines.

Both are drawn in the same reference scene: undeformed Sphere, Daylight, Thickness 0.5. Where a description mentions **the strongest harmonics**, it means the harmonics within half of the loudest one under Daylight, out of the 20 that carry the material's spectrum. Light-source behaviour is described by where the energy moves, which is what you actually hear.

---

## Materials

All materials are gain-compensated so they sit at a comparable loudness, but they do not end up identical. In the reference scene, Copper is clearly the quietest material, and Gold and Copper both lose more than half their level under LED Cool. The other eleven sit between about 1× and 2× Diamond's level. If a material feels underpowered, Volume is the most direct fix; switching to Sphere geometry also helps, since it is the loudest of the five geometries (see [Geometry](#geometry) below).

The groups below match the material dropdown in the plugin.

### Gems
**<span class="mat-diamond">Diamond</span>** · IOR 2.42 · Avg. transmission ~95%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/diamond-sphere.png' | relative_url }}" alt="Diamond sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/diamond-wavetable.png' | relative_url }}" alt="Diamond wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

The most even material in Elements. Its curve is flat at about 95% across the whole visible spectrum, so all 20 harmonics are present at similar strength and the sound is shaped almost entirely by the light and the Fresnel response. Under Sunset it sits warm (energy weighted toward harmonics 3 to 11), under Daylight it is balanced, and under LED Cool it turns bright (harmonics 11 to 20 dominate). The highest IOR of any material gives the strongest Fresnel response, so rotation has a pronounced effect on timbre.

> Crystalline, bright. Flat spectrum that follows the light.

---
**<span class="mat-ruby">Ruby</span>** · IOR 1.77 · Avg. transmission ~44%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/ruby-sphere.png' | relative_url }}" alt="Ruby sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/ruby-wavetable.png' | relative_url }}" alt="Ruby wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

Near-zero transmission from violet to yellow (apart from a small bump of about 13% in the violet-blue), then a sharp rise starting around 590nm to over 90% in the deep red. All of the strong harmonics are low, roughly 1 to 10 under Daylight. Changing the light barely moves the character, because a light cannot add what the material blocks: it stays low-harmonic under all three sources. The result is a rich, saturated tone built on the fundamental.

> Rich, saturated. Strong fundamental and low harmonics, weak highs under every light.

---
**<span class="mat-emerald">Emerald</span>** · IOR 1.57 · Avg. transmission ~29%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/emerald-sphere.png' | relative_url }}" alt="Emerald sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/emerald-wavetable.png' | relative_url }}" alt="Emerald wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

A narrow green window: transmission peaks at about 89% near 520 to 535nm, is above half of that only between roughly 470 and 590nm, and falls to a few percent in the red. Blue and red are both absorbed, so the strong harmonics form a band in the middle-to-upper range (about 10 to 16 under Daylight). This focus gives a nasal, midrange-only character unlike most other materials. Daylight, with its emission centred on 550nm, sits right on its window, but Emerald changes less with the light than the complex materials do.

> Balanced, focused. Band of mid-to-upper harmonics. Nasal character.

---
**<span class="mat-amethyst">Amethyst</span>** · IOR 1.54 · Avg. transmission ~47%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/amethyst-sphere.png' | relative_url }}" alt="Amethyst sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/amethyst-wavetable.png' | relative_url }}" alt="Amethyst wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

Transmits strongly in the violet and blue (88% at 380nm), absorbs hardest at 535nm in the green-yellow (11%), and recovers to about half in the red. That dip carves out harmonics roughly 11 to 14, and how much of it you hear depends on the light. Under Sunset the energy sits in harmonics 2 to 9 with the hole clearly present; under LED Cool the energy moves to the top harmonics (15 to 20); under Daylight, whose emission peaks inside the dip, the spectrum fills in and sounds broad. It is the most light-dependent material in Elements, from warm to bright.

> Complex, hollow. A dip in the upper-middle harmonics. Changes character more than any other material with the light.

---
**<span class="mat-sapphire">Sapphire</span>** · IOR 1.77 · Avg. transmission ~28%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/sapphire-sphere.png' | relative_url }}" alt="Sapphire sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/sapphire-wavetable.png' | relative_url }}" alt="Sapphire wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

Transmits the blue end (over 75% up to about 460nm), falls through 50% near 500nm and is almost zero above 560nm. Everything strong is in the upper harmonics (about 12 to 20 under Daylight) with very little low end, and that stays true under every light: slightly less bright under Sunset, which has little blue to give, and brightest under LED Cool. The narrow window produces a focused, airy sound.

> Clear, bright. High harmonics only, almost no low end.

---
**<span class="mat-alexandrite">Alexandrite</span>** · IOR 1.745 · Avg. transmission ~53%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/alexandrite-sphere.png' | relative_url }}" alt="Alexandrite sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/alexandrite-wavetable.png' | relative_url }}" alt="Alexandrite wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

Two transmission windows: green (peaking near 510nm at 74%) and a broad red (about 88% above 690nm, already over 80% by 660nm), with a dip near 590nm (20%) between them and little transmission in the violet. This is the "alexandrite effect" applied to sound. Under Sunset the red window dominates (harmonics 2 to 9), under LED Cool the energy moves to the green cluster (harmonics 12 to 16), and under Daylight both contribute and the spectrum spreads across harmonics 1 to 15. In the viewport the sphere also shifts colour with the light.

> Complex, colour-shifting. Two harmonic clusters. Character depends heavily on the light source.

---

### Minerals

**<span class="mat-amber">Amber</span>** · IOR 1.55 · Avg. transmission ~68%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/amber-sphere.png' | relative_url }}" alt="Amber sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/amber-wavetable.png' | relative_url }}" alt="Amber wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

Near-zero in the violet, crossing 50% around 490nm and above 90% from about 590nm to the red: a smooth, gradual curve. The strongest harmonics are 1 to 15 under Daylight. It is warm under Sunset and takes on a brighter, thinner edge as the light cools, with more upper harmonics coming through under LED Cool, but it never loses its low end. The gradual curve gives a warm, organic character overall.

> Warm, organic. Fuller under Sunset, brighter and thinner under LED Cool.

---
**<span class="mat-malachite">Malachite</span>** · IOR 1.85 · Avg. transmission ~43%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/malachite-sphere.png' | relative_url }}" alt="Malachite sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/malachite-wavetable.png' | relative_url }}" alt="Malachite wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

A broad green window: transmission peaks at about 90% near 535nm, with Cu²⁺ absorption cutting the blue and a d-d band cutting the red. Compared with <span class="mat-emerald">Emerald</span> the window is wider and softer-edged: it takes over 100nm to fall from the peak to about 40% on the red side, where Emerald does it in roughly 60nm. The strong harmonics are a mid-to-upper band (about 7 to 17), centred slightly toward the red of Emerald's.

> Focused, mineral. Wide green window. <span class="mat-emerald">Emerald</span>'s softer-edged cousin.

---
**<span class="mat-obsidian">Obsidian</span>** · IOR 1.50 · Avg. transmission ~20%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/obsidian-sphere.png' | relative_url }}" alt="Obsidian sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/obsidian-wavetable.png' | relative_url }}" alt="Obsidian wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

The darkest material in Elements: near-zero transmission (1 to 7%) until about 590nm, rising slowly to 70% only at the far red. Its spectrum is concentrated in the lowest harmonics (about 1 to 10 under Daylight), the narrowest body together with <span class="mat-copper">Copper</span>. Its raw transmission is the lowest of any material, but Elements compensates with the largest gain in the table, so its level stays comparable to the others: the sound is muted and dark rather than quiet. Use it for textural, atmospheric sound design rather than melodic content.

> Dark, minimal. Energy only in the lowest harmonics, muted presence.

---

### Metals

**<span class="mat-gold">Gold</span>** · IOR 0.47 (k 2.83) · Avg. reflectance ~59%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/gold-sphere.png' | relative_url }}" alt="Gold sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/gold-wavetable.png' | relative_url }}" alt="Gold wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

For metals the curve is a reflectance, not a transmission, and it is the imaginary part of the complex index (n + ik) that makes them reflect rather than transmit; see [The Science](science). Gold reflects almost nothing below 480nm, rises steeply between 510 and 590nm (half-way at about 535nm, the interband transition), and reflects over 95% from 640nm up. Its strong harmonics sit in the low-to-middle range (about 4 to 13 under Daylight). Fresnel shaping is much softer than for gems, so rotation changes it less, and Thickness has no effect on it at all. It sounds warm and mid-heavy under Sunset and Daylight, but loses more than half its level under LED Cool, which has little emission where Gold reflects.

> Metallic, warm. Low-mid harmonics. Soft Fresnel response.

---
**<span class="mat-copper">Copper</span>** · IOR 0.46 (k 2.83) · Avg. reflectance ~32%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/copper-sphere.png' | relative_url }}" alt="Copper sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/copper-wavetable.png' | relative_url }}" alt="Copper wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

Even more extreme than <span class="mat-gold">Gold</span>: it reflects only the red end. The curve stays under 10% until about 590nm and climbs gradually to about 94% at 780nm, crossing 50% near 660nm. Only the fundamental and the first few harmonics carry real weight (about 1 to 8 under Daylight), which makes it the most bass-heavy material, and even after gain compensation it is still the quietest in Elements. It loses roughly two thirds of its level under LED Cool. As a metal, Thickness has no effect and rotation changes it only subtly.

> Extremely warm, deep. Lowest harmonics only. The quietest material.

---

### Special

**<span class="mat-water">Water</span>** · IOR 1.33 · Avg. transmission ~73%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/water-sphere.png' | relative_url }}" alt="Water sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/water-wavetable.png' | relative_url }}" alt="Water wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

Flat and clear: over 94% transmission from violet to about 560nm, declining gently through the orange and red, then dropping steeply above 700nm (33% at 716nm, 4% at 767nm) because of O-H absorption. In harmonic terms, everything above about the third harmonic is present and strong (roughly 4 to 20 under Daylight), while the very bottom is nearly empty. The result is a bright, airy sound that is thin at the bottom. It follows the light: warmer under Sunset, brightest under LED Cool.

> Soft, fluid. Bright and airy, lowest harmonics thinned.

---
**<span class="mat-neodymium">Neodymium</span>** · IOR 1.636 · Avg. transmission ~68%

<div class="mat-figs">
  <img class="mat-sphere" src="{{ '/assets/images/materials/neodymium-sphere.png' | relative_url }}" alt="Neodymium sphere as rendered in the Elements viewport" loading="lazy">
  <img class="mat-wt" src="{{ '/assets/images/materials/neodymium-wavetable.png' | relative_url }}" alt="Neodymium wavetable: stack of its ten octave-band tables" loading="lazy">
</div>

Rare-earth glass whose six narrow f-f absorption bands (about 432, 522, 583, 625, 677 and 741nm) cut notches into an otherwise broad curve, rather than giving one broad cutoff. The deepest notch is at about 585nm (6% transmission), which opens a gap near harmonic 10. The broad body makes it harmonically dense, and it responds strongly to the light: under Sunset the weight sits in harmonics 3 to 9, under Daylight it spreads across 4 to 17, and under LED Cool it moves to 11 to 20.

> Complex, harmonically dense. Comb-style notches. Unlike any other material.

---

## Light sources

The three light sources have Gaussian emission curves centered on different parts of the visible spectrum:

| Light source | Peak | Width (σ) | Character |
|---|---|---|---|
| **Sunset** | 650nm (red) | 70nm | Warm, narrow |
| **Daylight** | 550nm (green/yellow) | 100nm | Broad, versatile |
| **LED Cool** | 470nm (blue) | 75nm | Cool, focused |

How much a light's emission overlaps with a material's curve shapes both timbre and level: strong overlap gives a fuller, brighter sound, while a mismatched pairing sounds duller and thinner. Elements' lights always keep some output across the full spectrum (a floor of 15 to 20% of their peak), so no combination of material and light goes silent as long as at least one light is on; they just sound different. See each material's description above for how it responds to the three light sources.

**General rules:**
- Materials with a narrow window (<span class="mat-sapphire">Sapphire</span>, <span class="mat-emerald">Emerald</span>, <span class="mat-ruby">Ruby</span>, <span class="mat-copper">Copper</span>, <span class="mat-obsidian">Obsidian</span>) keep their character under every light, because a light cannot add what the material blocks.
- Broad materials (<span class="mat-diamond">Diamond</span>, <span class="mat-water">Water</span>, <span class="mat-amber">Amber</span>) follow the light: warmer under Sunset, brighter under LED Cool.
- Complex materials (<span class="mat-amethyst">Amethyst</span>, <span class="mat-alexandrite">Alexandrite</span>, <span class="mat-neodymium">Neodymium</span>) change character the most from one light to the next.
- <span class="mat-gold">Gold</span> and <span class="mat-copper">Copper</span> reflect mostly red, so they lose a lot of level under LED Cool.
- Daylight is the most versatile source due to its broad emission.

---

## Geometry

Geometry determines how many surface normals interact with the light and how their Fresnel contributions are weighted, which directly defines how much timbral variation rotation produces. For the full maths, see [The Science](science).

---

**Cube**: 6 faces

<div class="mat-figs mat-figs-geo">
  <img class="mat-sphere" src="{{ '/assets/images/materials/geometry-cube.png' | relative_url }}" alt="Cube geometry rendered in Diamond, as in the Elements viewport" loading="lazy">
</div>

The most aggressive geometry. Six axis-aligned normals with a steep weighting (cos⁴): the face pointing most directly at the light dominates almost completely. Rotating the cube changes the timbre in sharp steps as one face takes over from another, and it has both the strongest timbral response to rotation and the widest swings in level of any geometry. Good for rhythmic or sequenced sounds where automation produces clear, differentiated changes.

> Angular, punchy. Dramatic timbral steps on rotation.

---

**Sphere**: continuous surface

<div class="mat-figs mat-figs-geo">
  <img class="mat-sphere" src="{{ '/assets/images/materials/geometry-sphere.png' | relative_url }}" alt="Sphere geometry rendered in Diamond, as in the Elements viewport" loading="lazy">
</div>

Without the Deformer, the sphere does not respond to rotation at all: a perfect sphere always presents the same curved surface to the light from any angle. Stable, smooth, predictable, and the loudest of the five geometries. When the **Deformer** is active, the symmetry breaks: 32 displaced normals introduce rotation sensitivity, continuous timbral drift sets in, a rough-surface scattering darkens the spectrum, and the wavefolding adds harmonic density. How much it responds to rotation grows with the Deformer's Amount; the deformed sphere has the highest expressive range of any geometry once Rate is above zero and the surface starts to move.

> Stable and uniform without Deformer. Maximum movement with Deformer active.

---

**Torus**: 12 normals

<div class="mat-figs mat-figs-geo">
  <img class="mat-sphere" src="{{ '/assets/images/materials/geometry-torus.png' | relative_url }}" alt="Torus geometry rendered in Diamond, as in the Elements viewport" loading="lazy">
</div>

Twelve normals: the six axis directions plus six diagonals, with quadratic weighting (cos²). Rotating the torus gives a pronounced but smoother response than the Cube: in testing it had the second-strongest timbral response to rotation of the five geometries, ahead of the Teapot and Dodecahedron.

> Pronounced but smooth. Strong response to rotation without the Cube's hard steps.

---

**Dodecahedron**: 12 pentagonal faces

<div class="mat-figs mat-figs-geo">
  <img class="mat-sphere" src="{{ '/assets/images/materials/geometry-dodecahedron.png' | relative_url }}" alt="Dodecahedron geometry rendered in Diamond, as in the Elements viewport" loading="lazy">
</div>

Twelve face normals spread evenly across the sphere (the directions of an icosahedron's vertices), with quadratic weighting (cos²). Unlike the cube, there is no axis alignment, and several faces always contribute to the spectrum at once, so the dominant face shifts gradually rather than jumping. It is the steadiest of the solid shapes: the gentlest response to rotation and the most consistent texture.

> Dense, even. Steady texture with the gentlest rotation response of the solids.

---

**Teapot**: 28 Bezier-patch normals

<div class="mat-figs mat-figs-geo">
  <img class="mat-sphere" src="{{ '/assets/images/materials/geometry-teapot.png' | relative_url }}" alt="Teapot geometry rendered in Diamond, as in the Elements viewport" loading="lazy">
</div>

The only asymmetric geometry in Elements. Twenty-eight Bezier patches contribute one normal each, sampled from the rim, body, handle, spout and lid, with quadratic weighting (cos²). Unlike the uniform solids above, rotation produces an irregular, non-repeating sequence of timbral changes rather than a predictable cycle. The size of the changes is moderate, between the Torus and the Dodecahedron.

> Asymmetric, irregular. Unpredictable timbral variation of moderate size.

---

| | Normals | Weighting | Rotation response | Relative level | Character |
|---|---|---|---|---|---|
| **Cube** | 6 (axis-aligned) | cos⁴ | Strongest, in steps | ~0.4× | Angular, punchy |
| **Sphere** | 32 with Deformer, one fixed angle without | cos | None without Deformer | 1× (loudest) | Smooth, stable |
| **Torus** | 12 (6 axes + 6 diagonals) | cos² | Strong, smooth | ~0.6× | Pronounced |
| **Dodecahedron** | 12 (uniform) | cos² | Gentlest of the solids | ~0.7× | Dense, even |
| **Teapot** | 28 (Bezier patches) | cos² | Moderate, irregular | ~0.7× | Asymmetric, detailed |

<span class="mat-note">Relative level is the average over all 13 materials at the default rotation under Daylight, compared with the Sphere, measured with the plugin's own engine. It varies by material and by rotation.</span>

**Sound design rule:** Cube for stepped timbral changes, Torus for strong but smooth ones, Teapot for irregular asymmetric detail, Dodecahedron for steady texture, Sphere for stability and level (or maximum movement with the Deformer).

---

[← Back to Elements](index)
