---
layout: default
title: The Science · Elements
---

[← Back to Elements](index)

# The Science

Elements is built on real physics. This page documents the mathematical and optical foundations behind the synthesis engine: from the Fresnel equations that govern light transmission to the spectral-to-harmonic mapping that turns optics into sound.

> The optical physics in Elements is exact. The sonification (mapping wavelengths to harmonics, Fresnel response to timbre, Beer-Lambert attenuation to brightness) is an artistic interpretation informed by that science, not a direct acoustic simulation.

<figure class="clip">
  <img src="{{ '/assets/images/science/pipeline.png' | relative_url }}" alt="Diagram: a light spectrum times a material curve times a Fresnel curve gives a spectrum, which becomes harmonic amplitudes and then a wavetable" loading="lazy">
  <figcaption>The whole idea on one page: three curves multiplied together give a spectrum, each colour of the spectrum becomes a harmonic, and the harmonics add up to the waveform you hear. The sections below take it one step at a time.</figcaption>
</figure>

*Each section starts with a one-line summary. The maths is there for anyone who wants it, and the longer derivations fold away until you open them.*

---

## 1. Fresnel equations

> **In one sentence:** Every surface reflects a little light and lets the rest through, and the more steeply you look at it the more it reflects. That sets how much light gets into the sound.
{: .tldr}

<figure class="clip">
  <img src="{{ '/assets/images/science/fresnel.png' | relative_url }}" alt="Graph of Fresnel reflectance against angle of incidence for water, amber, ruby, diamond and gold" loading="lazy">
  <figcaption>Reflectance stays low until about 60° and then shoots up towards 1 at grazing angles. Diamond reflects more than water at every angle, and Gold, being a metal, starts high.</figcaption>
</figure>

When light hits the surface of a material, the Fresnel equations determine how much is reflected and how much is transmitted. For dielectrics (every material except Gold and Copper), Elements uses the exact equations, not approximations.

**Snell's Law** (refraction angle):

```
sin(θt) = sin(θi) / n
```

Where `n` = index of refraction, `θi` = angle of incidence, `θt` = angle of transmission.

**Fresnel coefficients** for s and p polarization:

```
rs = (cos(θi) - n·cos(θt)) / (cos(θi) + n·cos(θt))
rp = (n·cos(θi) - cos(θt)) / (n·cos(θi) + cos(θt))
```

**Total reflectance** (unpolarized light, average of both polarizations):

```
R = ½(rs² + rp²)
T = 1 - R
```

<details markdown="1">
<summary>Safety net: total internal reflection and Schlick</summary>

**Safety net for total internal reflection:** if `sin(θt) ≥ 1`, the equations above have no real solution. This cannot happen for any dielectric in Elements (all have n > 1), but the code keeps a guard that falls back to the Schlick (1994) approximation, the standard in physically based rendering:

```
F₀ = ((1 - n) / (1 + n))²
R  = F₀ + (1 - F₀) · (1 - cos(θi))⁵
T  = 1 - R
```

</details>

Gold and Copper do not go through this path at all. They are metals and have their own pipeline (section 4).

---

## 2. Angle-dependent spectral shaping

> **In one sentence:** Tilt a surface away from the light and the blue fades first, so rotating an object makes the sound darker.
{: .tldr}

<figure class="clip">
  <img src="{{ '/assets/images/science/angle-shaping.png' | relative_url }}" alt="Graphs of the angle-dependent spectral modulation across wavelength for dielectrics and metals at six angles" loading="lazy">
  <figcaption>Left: for dielectrics the blue end collapses quickly as the angle grows while red survives. Right: metals keep almost the same colour at any angle.</figcaption>
</figure>

Beyond the base Fresnel response, Elements applies a wavelength-dependent exponential decay that simulates how real dielectric materials lose short wavelengths (blue) faster than long wavelengths (red) as the angle of incidence increases:

```
angleFactor = θ / 90°
wlPos       = (λ - 380) / (780 - 380)       // 0 = violet, 1 = red
decayRate   = 1 + 5·(1 - wlPos)             // blue = 6, red = 1
spectralMod = (1 - angleFactor)^decayRate
T_final(λ) = T_fresnel · spectralMod
```

In practice:
- At **0°** (perpendicular): full spectrum, all harmonics present
- At **45°**: blue is attenuated about 32× more than red (`0.5⁶` against `0.5¹`)
- At **70°+** (grazing): almost only low frequencies, dark timbre

This is consistent with Rayleigh scattering and the selective absorption observed in real dielectric materials. Metals use a much weaker version of this shaping (section 4).

---

## 3. Beer-Lambert law (Thickness)

> **In one sentence:** Light fades as it travels through a material, and Thickness sets how far it travels: a thicker piece keeps only the colours it transmits best.
{: .tldr}

<figure class="clip">
  <img src="{{ '/assets/images/science/beer-lambert.png' | relative_url }}" alt="Graphs of Ruby and Emerald transmission raised to different thickness values" loading="lazy">
  <figcaption>Thickness is an exponent on the curve. Thin (dotted) flattens the material towards a plain spectrum, thick (dash-dot) leaves only its strongest windows.</figcaption>
</figure>

The Beer-Lambert law describes how light attenuates exponentially as it passes through an absorbing medium. The standard form:

```
I = I₀ · e^(-α·d)
```

Elements implements this as:

```
T_effective(λ) = T_material(λ)^thickness
```

Which is mathematically equivalent, since:

```
T = e^(-α·d₀)
T^thickness = e^(-α·d₀·thickness) = e^(-α·d)
```

Thickness runs from 0.1 to 2.0 (default 0.5). The effect on sound:
- **thickness < 1.0**: thinner material, more transmission, brighter sound
- **thickness = 1.0**: nominal material transmission, no modification
- **thickness > 1.0**: thicker material, more absorption, darker and heavier sound

Materials with low transmission in certain bands become practically opaque in those bands at high thickness values. For example, Ruby's blue transmission of 0.02 at thickness 2.0: `0.02² = 0.0004`.

**Metals ignore Thickness.** Beer-Lambert describes light travelling *through* a material, and metals are opaque: they work by reflecting light. The exponent is scaled by how dielectric the material is:

```
effectiveThickness = 1 + (thickness - 1) · (1 - metallicFactor)
```

For Gold and Copper (`metallicFactor = 1`) this is exactly 1 for any Thickness setting, so the slider has no effect.

---

## 4. Metals: complex refractive index

> **In one sentence:** Gold and Copper reflect light instead of letting it through, so they keep their colour at any angle and Thickness does nothing to them.
{: .tldr}

Gold and Copper are described by a complex index of refraction, `n + ik`. The imaginary part `k` (the extinction coefficient) is what makes a metal opaque and reflective. Both use data from Palik (1985):

| Metal | n | k |
|---|---|---|
| Gold | 0.47 | 2.83 |
| Copper | 0.46 | 2.83 |

For metals, the material curve stored in Elements is a spectral **reflectance**, not a transmission. The pipeline differs from the dielectric one in three ways:

**1. Base reflectance from both n and k:**

```
F₀ = ((n - 1)² + k²) / ((n + 1)² + k²)
```

**2. Schlick angle dependence:**

```
R(θ) = F₀ + (1 - F₀) · (1 - cos θ)⁵
```

**3. Much weaker angle-dependent colour shift.** Real metals barely change colour with viewing angle, so the decay is flatter and capped:

```
decayRate   = 0.5 + 1.0·(1 - wlPos)         // 0.5 to 1.5 (dielectrics: 1 to 6)
spectralMod = (1 - 0.3·angleFactor)^decayRate
S(λ) = L(λ) · reflectance(λ) · R(θ) · spectralMod
```

Materials can sit anywhere between the two paths: the final spectrum is a linear blend of the dielectric and metallic results weighted by `metallicFactor`. In the current material set, Gold and Copper are fully metallic and the other eleven are fully dielectric.

---

## 5. Geometry and light direction

> **In one sentence:** A shape is a set of faces. Each face gets its own Fresnel response, the faces facing the light count most, and the average is the spectrum, which is why rotating an object changes the timbre.
{: .tldr}

The object's surface is represented by a set of **face normals**. For each light, every normal is rotated by the object's rotation, and the angle to the light direction sets that face's Fresnel response. A face pointing away from the light (`cos θ ≤ 0`) contributes nothing. The rest are combined as a weighted average:

```
S_light(λ) = Σ_faces w · L(λ) · M(λ) · F(λ, θ_face)  /  Σ_faces w
```

| Geometry | Normals | Weight `w` |
|---|---|---|
| Cube | 6 | `cos⁴ θ` (the best-lit face dominates sharply) |
| Torus | 12 | `cos² θ` |
| Dodecahedron | 12 | `cos² θ` |
| Teapot | 28 (one per Bézier patch: rim, body, handle, spout, lid) | `cos² θ` |
| Sphere, undeformed | 1 | a fixed 10° angle |
| Sphere, deformed | 32 | `cos θ` |

<figure class="clip">
  <img src="{{ '/assets/images/science/geometry-weights.png' | relative_url }}" alt="Graph of face weight against angle for cube, torus and deformed sphere" loading="lazy">
  <figcaption>The cube's cos⁴ weight lets only the best-lit face speak, so it changes in sharp steps as it turns. Torus, Dodecahedron and Teapot (cos²) blend their faces more smoothly, and a deformed sphere (cos) most smoothly of all.</figcaption>
</figure>

Two consequences worth knowing:
- Because the average is normalized, rotating the object changes the *shape* of the spectrum (timbre). The overall *level* is handled separately (section 6).
- A perfectly smooth sphere looks identical from every direction, so **rotation does nothing to a sphere until Deform is above zero**. Deformation tilts the normals, which restores the angle-dependence a faceted shape has naturally.

Lights are **directions**, not positions in space. The three light slots point along fixed directions:

| Light | Direction (x, y, z) |
|---|---|
| Key | (0.5, 0.7, 0.5) |
| Fill | (-0.6, 0.3, 0.4) |
| Rim | (0.0, 0.2, -0.8) |

---

## 6. Combined spectrum

> **In one sentence:** Light times material times angle gives one spectrum per oscillator. More lights make it richer, not louder, and the average of the spectrum sets the volume.
{: .tldr}

For each oscillator, the final spectrum feeding the synthesis engine is built in these steps:

```
S(λ) = L(λ) · M(λ) · F(λ, θ)
```

Where:
- `L(λ)` = spectral power distribution of the light source (Gaussian, section 12)
- `M(λ)` = material curve (32 measured or modelled points, interpolated onto the 50-point wavelength grid)
- `F(λ, θ)` = spectral Fresnel response at the incidence angle (sections 1, 2, 4 and 5)

**1. Lights.** The enabled lights are summed, weighted by their intensity sliders, and normalized so that more lights means a richer timbre, not a louder one:

```
S_combined(λ) = Σᵢ (intensityᵢ · Sᵢ(λ)) / Σᵢ intensityᵢ
```

**2. Thickness.** `S(λ) → S(λ)^effectiveThickness` (section 3).

**3. Gain compensation.** A per-material gain evens out loudness between materials with very different total transmission (from 1.0 for Diamond up to 4.5 for near-opaque Obsidian).

**4. Peak normalization.** If any value exceeds 1, the whole spectrum is scaled down so its peak is 1. The shape is preserved.

**5. Silence.** If the spectrum is essentially empty (all 50 values sum to less than 0.5, for example with every light switched off), the output is silent. That is the physically honest result: no light, no sound.

**6. Level.** The mean of the spectrum sets the overall loudness:

```
amplitude = clamp(mean(S) · 1.5, 0.1, 1.0)
```

So angles and settings that let more light through are louder, and darker configurations are quieter. When MIX is above 0 and both oscillators are active, this level follows material B's spectrum.

---

## 7. Spectrum → harmonics mapping

> **In one sentence:** Red becomes the fundamental, violet the highest harmonic, and how much of each colour gets through sets how loud its harmonic is.
{: .tldr}

<figure class="clip">
  <img src="{{ '/assets/images/science/harmonic-mapping.png' | relative_url }}" alt="Three panels: a spectrum, the values sampled at each harmonic, and the amplitudes after cubing and rolloff" loading="lazy">
  <figcaption>Ruby under Daylight at 30°. The spectrum is sampled once per harmonic, then cubed (which widens the gap between loud and quiet colours) and tilted by a gentle rolloff.</figcaption>
</figure>

Each harmonic in the wavetable "body" maps to a position in the visible spectrum via continuous linear interpolation:

```
specPos(h) = 1 - (h - 1) / (H - 1)
```

Where `h` = harmonic number (1 to H) and `H` = the size of the body (20 harmonics, fewer on the highest-note bands, see below). This produces:
- **Harmonic 1** (fundamental) → specPos 1.0 → red (780nm)
- **Harmonic H** (highest of the body) → specPos 0.0 → violet (380nm)

Amplitude is obtained by interpolating the 50-point spectrum and applying:

```
A_raw      = lerp(spectrum[i₀], spectrum[i₁], frac)
A_emphasis = A_raw³                         // spectral contrast
rolloff    = 1 / (1 + (h-1) · 0.05)        // natural per-harmonic rolloff
A_final(h) = A_emphasis · rolloff
```

The cube (`A³`) amplifies spectral differences: high-transmission values are preserved (`0.9³ = 0.73`) while low values are strongly suppressed (`0.1³ = 0.001`). This translates subtle optical differences into audible timbral contrast.

The first 20 harmonics (the "body") carry the material's spectrum this way. Above the body, a material-independent tail with a gentle `1/h` slope fills the table up to the highest harmonic that fits below Nyquist. The tail is deliberately the same for every material: the filter sits after the oscillator and works on whatever harmonics exist, so every note has content all the way up the spectrum for a lowpass to shape, whatever the material.

The wavetable is built by additive synthesis (computed with an inverse FFT), soft-saturated with `tanh`, then band-limited again so the saturation cannot create harmonics past Nyquist:

```
body[i]      = Σₕ≤20 A_final(h) · sin(2π·h·i / TABLE_SIZE)
wavetable[i] = band_limit( tanh(body[i]) ) + tail[i]
```

<details markdown="1">
<summary>Band-limited wavetables: the ten bands</summary>

Ten band-limited wavetables, one per octave, prevent aliasing. Each holds every harmonic that fits below about 0.45 of the sample rate for the top note of its band (up to 512 harmonics):

| Band | Frequency range | Harmonics (44.1 kHz) |
|---|---|---|
| 1 | < 32 Hz | 512 |
| 2 | 32–64 Hz | 310 |
| 3 | 64–128 Hz | 155 |
| 4 | 128–256 Hz | 77 |
| 5 | 256–512 Hz | 38 |
| 6 | 512–1024 Hz | 19 |
| 7 | 1–2 kHz | 9 |
| 8 | 2–4 kHz | 4 |
| 9 | 4–8 kHz | 2 |
| 10 | > 8 kHz | 1 |

Where a band holds fewer than 20 harmonics (above about 512 Hz), the material's spectrum is spread across the harmonics it has, so the blue end of the spectrum stays audible on high notes.

</details>

Ring Mod and AM multiply the two oscillators, and the product contains the sum of both oscillators' frequencies. To keep those products below Nyquist, both oscillators read the table one octave higher (half the harmonics) while either of those modes is active.

---

## 8. The Deformer

> **In one sentence:** A bumpy surface tilts the normals, scatters the blue, makes every colour shimmer and folds the wave. Three layers, one noise field.
{: .tldr}

The Deformer (Sphere only) turns the smooth sphere into a bumpy, evolving surface. It acts on the sound in three layers, all driven by the same noise field. The controls are Amount (how bumpy), Freq (how dense the bumps are), Rate (how fast they evolve, 0 = frozen) and Noise type.

<figure class="clip">
  <img src="{{ '/assets/images/science/deformer.png' | relative_url }}" alt="Left: a diamond spectrum becoming darker as Deform rises. Right: a stack of waveforms from Deform 0 at the front to Deform 1 at the back" loading="lazy">
  <figcaption>Real engine output for Diamond with Daylight on a Sphere. Left: raising Deform lowers the spectrum and tilts it towards red (roughness scatters blue first). Right: the waveform changes shape from front (Deform 0) to back (Deform 1).</figcaption>
</figure>

### Layer 1: tilted normals and surface roughness

<details markdown="1">
<summary>Layer 1 maths: the noise gradient and the tilted normals</summary>

The Deformer displaces the sphere's surface normals using the gradient of a 3D noise field, calculated by central finite differences. 32 evenly spread points on the sphere (a Fibonacci spiral) are sampled:

```
∂N/∂x = (noise(x+ε, y, z) - noise(x-ε, y, z)) / 2ε
∂N/∂y = (noise(x, y+ε, z) - noise(x, y-ε, z)) / 2ε
∂N/∂z = (noise(x, y, z+ε) - noise(x, y, z-ε)) / 2ε
```

With `ε = 0.01`. Full gradient: `∇N = (∂N/∂x, ∂N/∂y, ∂N/∂z)`. Freq scales the position in the noise field. The three axes advance through the field in time at different rates (golden-ratio multiples), so the 32 normals evolve along uncorrelated paths.

**Tangential projection**: ensures displacement only tilts the normal without changing its radial magnitude.

```
grad_radial     = ∇N · p̂
grad_tangential = ∇N - p̂ · grad_radial
n_displaced     = normalize(p̂ - grad_tangential · scale)
scale           = deformAmount · 1.5
```

</details>

Each tilted normal then gets its own Fresnel response (section 1), and the results are averaged with weight `cos θ`. The Amount smoothly crossfades from the undeformed spectrum to the deformed one over its first 15%, so there is no jump when Deform is switched on.

<details markdown="1">
<summary>Noise types and surface roughness</summary>

**Three noise types** are available. Simplex (Perlin 2001) is smooth and organic: it evaluates a scalar field on a tetrahedral lattice with skew constant `F₃ = 1/3` and unskew `G₃ = 1/6`, a 256-entry permutation table and 12 gradient vectors, and produces C¹ continuous values in [-1, 1] with no axis-alignment artifacts. Alligator gives rounded, blob-like cells with no sharp edges. Worley (cellular noise) gives sharp, Voronoi-like ridges.

**Surface roughness** uses the Bennett and Porteus (1961) result for a rough surface. The specular reflectance falls off as:

```
R_spec / R_smooth = exp( -(4π · σ · cos θ / λ)² )
σ = deformAmount · 50 nm        // RMS roughness
```

Short wavelengths scatter out of the specular path first, so roughness darkens the spectrum toward red. Deform therefore always has a consistent spectral signature, on top of the angle changes from the tilted normals.

</details>

### Layer 2: spectral shimmer and phase

Each of the 50 wavelengths gets its own slowly moving noise value (the "shimmer"), which modulates the spectrum before the wavetable is rebuilt:

```
S'(λ) = S(λ) · (1 + shimmer(λ) · deformAmount · 2 · tilt(λ))
```

- **Rate** sets how fast the noise source advances. At Rate 0 it is frozen.
- **Freq** scales the source speed (about 0.3× to 4×) *and* how much neighbouring wavelengths move together. Low Freq: neighbouring wavelengths move together, a coherent wobble. High Freq: they decorrelate and twinkle independently. `tilt(λ)` also shifts the modulation depth toward the blue (high harmonic) end as Freq rises, so shimmer reads as a treble effect and wobble as broadband.

The shimmer also moves the **phase** of each harmonic, modelling the optical path difference of a bumpy surface. A point displaced by height `h` shifts the reflected wave by:

```
Δφ(λ) = 4π · h / λ         // round trip
h     = shimmer · 3σ       // peak excursions of a Gaussian rough surface
```

Shorter wavelengths (higher harmonics) swing proportionally more, so the *waveform itself* morphs over time, not just the harmonic levels. The shimmer is applied to oscillator A's wavetable.

### Layer 3: sinusoidal wavefolder

A gentle sine fold is applied to the audio, with its drive following the average shimmer, so the folding breathes in sync with the spectral movement:

```
drive = deformAmount · mean(|shimmer|)      // roughly 0 to 1
x'    = x · (1 + 3·drive)
y     = lerp( x, sin(x' · π/2), drive )
```

Properties:
- **drive = 0**: `y = x`, fully transparent.
- **Natural bounding**: `sin()` always outputs within [-1, 1].
- As the drive rises, the signal increasingly folds back on itself, adding both even and odd harmonics.

Unlike saturation (`tanh`), which compresses dynamic range, wavefolding is periodic, producing a denser, more musically complex harmonic spectrum.

<figure class="clip">
  <img src="{{ '/assets/images/science/wavefolder.png' | relative_url }}" alt="Graph of the wavefolder transfer curve for five drive values" loading="lazy">
  <figcaption>The transfer curve bends from a straight line (drive 0) into repeated folds as the drive rises. Each fold adds harmonics.</figcaption>
</figure>

---

## 9. Dual oscillators and blend modes

> **In one sentence:** Two materials give two spectra, and the blend mode decides how their waves are combined.
{: .tldr}

Elements runs two oscillators per voice. Each has its own material, so its own spectrum and wavetable, but both share the same geometry, rotation, lights and Thickness. B is detuned by up to ±100 cents relative to A (`ratio = 2^(cents/1200)`).

The MIX control crossfades from pure A (0) to the blend result (1):

```
output = lerp( A, blended, mix )
```

`A` and `B` below are the two oscillators' samples at the same instant, each in [-1, 1]:

| Mode | Formula | Character |
|---|---|---|
| Ring Mod | `A · B` | Multiplies every harmonic of A with every harmonic of B, producing sum and difference frequencies that are in neither source. Metallic, inharmonic. |
| AM | `A · max(0, 1 + 3·depth·B)` | B modulates the amplitude of A. At depth 0 it is pure A. As depth rises, sidebands appear around A's harmonics. The `max(0, …)` floor gates A to silence where B pulls the envelope below zero, so high depth sounds rhythmic and choppy. |
| XOR | `A - B` | Subtracts one waveform from the other. Where the two oscillators agree the output cancels, and where their spectra differ it is loudest. Hollow and subtractive. |
| FM | `A( phase + 4·depth·B )` | B modulates the **phase** of A before the wavetable is read. Depth is the FM index, from 0 to 4 cycles. |

<details markdown="1">
<summary>Blend mode details (Carson's rule, band selection)</summary>

Some details:
- For FM, the oscillator reads a wavetable band chosen for the boosted bandwidth (using Carson's rule: highest sideband ≈ `f · (1 + (index+1)·detuneRatio)`) so that deep modulation does not alias.
- For Ring Mod and AM, both oscillators read one band higher to keep the multiplied products below Nyquist (section 7).
- MUTE A removes A from the blend and plays B alone, which makes it easy to hear what B contributes.
- At MIX = 0, oscillator B is not computed at all.

</details>

<figure class="clip">
  <img src="{{ '/assets/images/science/blend-modes.png' | relative_url }}" alt="Waveforms of oscillators A and B and the Ring Mod, AM, XOR and FM results" loading="lazy">
  <figcaption>Ruby (A) and Emerald (B, a fifth higher) through each blend mode. Ring Mod keeps energy only where both are active, AM carves A with B, XOR leaves what differs, and FM bends A's phase.</figcaption>
</figure>


---

## 10. Chorus

> **In one sentence:** Delayed, quieter copies of the sound, one for every copy of the object in the viewport.
{: .tldr}

Chorus duplicates the object into a trail of copies receding behind it. In the viewport you see the copies, and in the audio each copy is a delayed, quieter echo of the sound. It is a signal-processing effect after the synth, not a raytracing simulation.

- **Voices** (2 to 5) is the total number of copies, **including the original**. Taps = Voices − 1.
- **Spread** (5 to 40 ms) is the delay between neighbouring copies. Copy `v` is delayed by `v · spread`.
- **Decay** (0 to 0.9) is the fraction of energy **lost** per hop, so copy `v` has gain `(1 - decay)^v`. The viewport copies fade with the same factor, as opacity.
- **Wobble** (0 to 1) makes each copy's delay drift over time, which is what makes it sound like a chorus rather than a fixed comb filter.

```
delay_v = v · spread  +  wobble · min(6 ms, 0.9 · v·spread) · simplex(1.7·v, t)
wet     = Σ_v (1 - decay)^v · tap_v
out     = tanh( dry + wet )
```

The noise value `t` advances at 0.15 Hz, slow enough to read as a gentle "breathing" rather than a tremolo. The wobble excursion is capped at 90% of each tap's own base delay so a delay can never reach zero.

<figure class="clip">
  <img src="{{ '/assets/images/science/chorus-taps.png' | relative_url }}" alt="Bar charts of chorus tap gains at three decay settings" loading="lazy">
  <figcaption>Voices 5, Spread 20 ms. With Decay 0 every copy is as loud as the original, and raising Decay thins the trail out quickly.</figcaption>
</figure>

The left and right channels read the same delay line with independently drifting delay times, so Chorus turns Elements' mono synth core into a wide stereo signal. Voices, Spread, Decay, Wobble and the on/off switch are all smoothed, so moving them does not click.

---

## 11. Physical Envelope

> **In one sentence:** Brighter light attacks faster, opaque material decays for longer, and dense material sustains and releases more slowly.
{: .tldr}

In Physical mode, the four ADSR stages are derived directly from optical properties. When both oscillators are active, the IOR and transmission are interpolated between material A and material B by the MIX amount.

**Attack**: photonic energy (light intensity).
```
attack = lerp(0.5, 0.005, avgIntensity)
```
Higher light intensity = more energy = faster transient.

**Decay**: material absorption × thickness.
```
decay = thickness · (1 - avgTransmission) · 1.5
```
A thick, opaque material absorbs more light and produces a longer decay.

**Sustain**: internal reflections (IOR).
```
sustain = IOR / 2.42        // limited to 0.1 – 0.95
```
Normalized against Diamond (maximum IOR = 2.42). Higher IOR means a lower critical angle for total internal reflection (`θ_critical = arcsin(1/n)`), trapping more light inside the material, resulting in higher sustain. Diamond ≈ 0.95, Water ≈ 0.55.

**Release**: trapped light (IOR × thickness).
```
release = IOR · 0.2 · thickness
```
A dense, thick material releases light more slowly.

<figure class="clip">
  <img src="{{ '/assets/images/science/physical-envelope.png' | relative_url }}" alt="Four ADSR envelope shapes derived from Diamond, Water, Ruby and Obsidian" loading="lazy">
  <figcaption>Real material values, Thickness 1.0 and light intensity 0.5. Diamond is short and sustained, Obsidian absorbs light and takes the longest to settle. Attack is the same in all four because it depends on the light.</figcaption>
</figure>

Each stage is also limited to a sensible range (for example attack 1 ms to 2 s) so no combination of settings produces an unplayable envelope.

---

## 12. Light spectral distributions

> **In one sentence:** Sunset is centred on red, Daylight on green and LED Cool on blue, and that decides which colours of a material get lit.
{: .tldr}

Each light source has a spectral power distribution modeled as a Gaussian, a standard approximation of blackbody (Planck) distributions used in colorimetry and CG lighting:

```
I(λ) = base + peak · e^(-(λ - center)² / 2σ²)
```

| Light | Center (nm) | σ (nm) | Base | Peak | Approx. temperature |
|---|---|---|---|---|---|
| Sunset | 650 | 70 | 0.15 | 0.85 | ~2000K |
| Daylight | 550 | 100 | 0.20 | 0.80 | ~5500K |
| LED Cool | 470 | 75 | 0.15 | 0.85 | ~6500K |

The non-zero base values ensure no light has zero energy in any band, simulating the low-energy tail of a real Planck distribution. They are deliberately low: the final spectrum is a product of light, material and Fresnel curves, so a light with a high floor would be drowned out by the much sharper material curves. With these values, changing the light source changes the timbre audibly. It matters most for broad-spectrum materials such as Diamond and Water. A narrow-band material such as Ruby or Sapphire can only pass the wavelengths it transmits, so light changes affect it less.

<figure class="clip">
  <img src="{{ '/assets/images/science/light-spectra.png' | relative_url }}" alt="Graph of the Sunset, Daylight and LED Cool spectral power distributions" loading="lazy">
  <figcaption>The three light spectra, with their gentle floor. Where a material's curve overlaps a light's peak, those harmonics sing.</figcaption>
</figure>

---

## 13. Pitch modulation by light intensity

> **In one sentence:** More light than neutral raises the pitch and less lowers it, by at most two semitones.
{: .tldr}

Light intensity modulates pitch as a deviation from equilibrium:

```
equilibrium  = activeCount · 0.5
deviation    = totalIntensity - equilibrium
maxDeviation = activeCount · 0.5
pitchOffset  = (deviation / maxDeviation) · 2.0 semitones     // limited to ±2
frequency    = f₀ · 2^(pitchOffset / 12)
```

At the default intensity of 0.5 per light there is no pitch change. This is consistent with the photon energy–frequency relationship (E = hν): more light energy → higher frequency.

<figure class="clip">
  <img src="{{ '/assets/images/science/pitch-vs-intensity.png' | relative_url }}" alt="Graph of pitch offset in semitones against light intensity for one, two and three lights" loading="lazy">
  <figcaption>With more lights on, one slider moves the total less, so its pitch range shrinks. Move all of them together and you get the full ±2 semitones.</figcaption>
</figure>

---

## 14. What is measured, what is calculated, and what is designed

> **In one sentence:** The shape of every timbre comes from real optics. The step from that shape to sound is where the design lives.
{: .tldr}

"Physics-based" can mean several different things, so here is exactly which parts of Elements are which.

**Measured data.** Real numbers from the literature:
- Refractive indices (Sellmeier dispersion for Diamond, GIA and mineralogy tables for the gems) and the complex index `n + ik` of Gold and Copper (Palik, 1985).
- The spectral curves of the materials, from published spectroscopy: Pope & Fry (Water), USGS spectral library (Malachite, via a Kubelka-Munk conversion), GIA and peer-reviewed gemology papers (the gems), and others listed in the Material optical data table. For several gems the *band positions* are from the literature, while the depth of the curve between bands is estimated from crystal-field theory. Alexandrite, Malachite and Neodymium are the least precisely sourced, and are marked as qualitative.

**Real formulas, evaluated live.** Computed from those constants and the current settings, with no shortcuts: the Fresnel equations (real and complex), Beer-Lambert absorption, Snell's law, Bennett and Porteus roughness scattering, and the path-difference phase.

**Designed conventions.** Choices that make the instrument musical, not laws of physics:
- **The harmonic mapping.** No natural law says a reflectance curve should become a harmonic series. Mapping red to the fundamental and violet to the highest harmonic is a design decision, as are the cube emphasis, the 1/h tail and the rolloff.
- **No spatial scale.** Lights are directions, not positions. Thickness is a dimensionless exponent, not a real slab thickness in millimetres.
- **Light sources.** The three Gaussian spectra are approximations, not measured illuminant spectra.
- **The angle-dependent colour shift** for dielectrics (section 2) is a simple model of the real effect.
- **Pitch from intensity, the Physical Envelope, per-material gain compensation and the blend modes** are artistic mappings informed by the physics, not simulations.

The optical physics is the source of the *shape* of every timbre. The mapping from that shape to sound is where the instrument design lives.

---

## Material optical data

The spectral curves of all 13 materials are based on real optical data. Each curve has 32 points covering 380–780nm.

<figure class="clip">
  <img src="{{ '/assets/images/science/material-curves.png' | relative_url }}" alt="Small graphs of the spectral curve of each of the 13 materials" loading="lazy">
  <figcaption>All thirteen curves, with the colours of the spectrum behind them. Compare these shapes with the wavetables on the Materials page.</figcaption>
</figure>

| Material | n | Optical basis |
|---|---|---|
| Diamond | 2.42 | Uniform ~95% transmission (colorless), slight UV scatter loss at 380nm |
| Ruby | 1.77 | Cr³⁺ in Al₂O₃: absorbs at 413nm and 550nm, transmits a blue-violet window and red above 620nm |
| Sapphire | 1.77 | Fe²⁺/Ti⁴⁺ charge transfer in Al₂O₃: broad absorption at 570nm |
| Emerald | 1.57 | Cr³⁺ in Be₃Al₂Si₆O₁₈: absorption at 430nm and 610nm opens a green window |
| Amethyst | 1.54 | [FeO₄]⁰ (Fe⁴⁺) centre in SiO₂: absorbs at 545nm, transmits violet and red |
| Alexandrite | 1.75 | Cr³⁺ in chrysoberyl: two transmission peaks (green and red) give the colour-change effect |
| Amber | 1.55 | Conjugated organic absorption in fossil resin: UV cutoff at ~440nm, smooth warm sigmoid |
| Malachite | 1.85 | Cu²⁺: charge transfer cuts the blue, a d-d band cuts the red, leaving a narrow green window |
| Obsidian | 1.50 | Fe²⁺ and magnetite nanoparticles in volcanic glass: featureless, rising toward the red |
| Gold | 0.47 (k 2.83) | 5d→6sp interband transition at ~500nm: absorbs blue and UV, reflects yellow and red |
| Copper | 0.46 (k 2.83) | Deeper interband transition at ~600nm: absorbs blue-green, reflects orange-red |
| Water | 1.33 | O-H vibrational overtones: flat in the visible, steep drop above 700nm (1 m path) |
| Neodymium | 1.64 | Nd³⁺ in glass: six narrow f-f absorption bands (432, 522, 583, 625, 677 and 741nm) giving a comb-like spectrum |

For Gold and Copper the curve is a reflectance (section 4). For all others it is a transmission.

---

## Further reading

**Fresnel equations**
- [Fresnel equations (Wikipedia)](https://en.wikipedia.org/wiki/Fresnel_equations): comprehensive reference with derivations
- [Fresnel equations (HyperPhysics)](http://hyperphysics.phy-astr.gsu.edu/hbase/phyopt/freseq.html): concise, accessible explainer
- [Fresnel equations (RP Photonics)](https://www.rp-photonics.com/fresnel_equations.html): technical reference with FAQ
- [Reflection and Transmission (Physics LibreTexts)](https://phys.libretexts.org/Bookshelves/Optics/Physical_Optics_(Tatum)/02:_Reflection_and_Transmission_at_Boundaries_and_the_Fresnel_Equations): university-level treatment

**Beer-Lambert law**
- [Beer-Lambert law (Wikipedia)](https://en.wikipedia.org/wiki/Beer%E2%80%93Lambert_law): full reference
- [The Beer-Lambert Law (Chemistry LibreTexts)](https://chem.libretexts.org/Bookshelves/Physical_and_Theoretical_Chemistry_Textbook_Maps/Supplemental_Modules_(Physical_and_Theoretical_Chemistry)/Spectroscopy/Electronic_Spectroscopy/Electronic_Spectroscopy_Basics/The_Beer-Lambert_Law): clear, accessible explanation
- [Beer-Lambert law (RP Photonics)](https://www.rp-photonics.com/beer_lambert_law.html): technical reference

**Schlick approximation**
- Schlick, C. (1994). *An Inexpensive BDRF Model for Physically-based Rendering*. Computer Graphics Forum, 13(3), 233–246. Original paper describing the Fresnel approximation used for metals in Elements.

**Metals and complex refractive index**
- Palik, E.D. (1985). *Handbook of Optical Constants of Solids*. Source of the `n` and `k` values for Gold and Copper.

**Rough-surface scattering**
- Bennett, H.E. & Porteus, J.O. (1961). *Relation between surface roughness and specular reflectance at normal incidence*. Journal of the Optical Society of America, 51(2), 123–129.

**Simplex noise**
- Perlin, K. (2001). *Noise hardware*. Real-Time Shading SIGGRAPH Course Notes. Original description of Simplex Noise.

---

[← Back to Elements](index)
