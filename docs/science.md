---
layout: default
title: The Science — Elements
---

## Further reading

**Fresnel equations**
- [Fresnel equations — Wikipedia](https://en.wikipedia.org/wiki/Fresnel_equations) — comprehensive reference with derivations
- [Fresnel equations — HyperPhysics](http://hyperphysics.phy-astr.gsu.edu/hbase/phyopt/freseq.html) — concise, accessible explainer
- [Fresnel equations — RP Photonics](https://www.rp-photonics.com/fresnel_equations.html) — technical reference with FAQ
- [Reflection and Transmission — Physics LibreTexts](https://phys.libretexts.org/Bookshelves/Optics/Physical_Optics_(Tatum)/02:_Reflection_and_Transmission_at_Boundaries_and_the_Fresnel_Equations) — university-level treatment

**Beer-Lambert law**
- [Beer-Lambert law — Wikipedia](https://en.wikipedia.org/wiki/Beer%E2%80%93Lambert_law) — full reference
- [The Beer-Lambert Law — Chemistry LibreTexts](https://chem.libretexts.org/Bookshelves/Physical_and_Theoretical_Chemistry_Textbook_Maps/Supplemental_Modules_(Physical_and_Theoretical_Chemistry)/Spectroscopy/Electronic_Spectroscopy/Electronic_Spectroscopy_Basics/The_Beer-Lambert_Law) — clear, accessible explanation
- [Beer-Lambert law — RP Photonics](https://www.rp-photonics.com/beer_lambert_law.html) — technical reference

**Schlick approximation**
- Schlick, C. (1994). *An Inexpensive BDRF Model for Physically-based Rendering*. Computer Graphics Forum, 13(3), 233–246. — original paper describing the Fresnel approximation used for metals in Elements

**Simplex noise**
- Perlin, K. (2001). *Noise hardware*. Real-Time Shading SIGGRAPH Course Notes. — original description of Simplex Noise

---

[← Back to Elements](index)

# The Science

Elements is built on real physics. This page documents the mathematical and optical foundations behind the synthesis engine — from the Fresnel equations that govern light transmission to the spectral-to-harmonic mapping that turns optics into sound.

> The optical physics in Elements is exact. The sonification — mapping wavelengths to harmonics, Fresnel response to timbre, Beer-Lambert attenuation to brightness — is an artistic interpretation informed by that science, not a direct acoustic simulation.

---

## 1. Fresnel equations

When light hits the surface of a material, the Fresnel equations determine how much is reflected and how much is transmitted. Elements uses the exact equations, not approximations.

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

**Special case — Total Internal Reflection (TIR):** When `sin(θt) ≥ 1` (occurs with low-IOR metals like Gold = 0.47 and Copper = 0.46), Elements uses the Schlick (1994) approximation — the standard in physically based rendering:

```
F₀ = ((1 - n) / (1 + n))²
R  = F₀ + (1 - F₀) · (1 - cos(θi))⁵
T  = 1 - R
```

---

## 2. Angle-dependent spectral shaping

Beyond the base Fresnel response, Elements applies a wavelength-dependent exponential decay that simulates how real materials absorb short wavelengths (blue) more than long wavelengths (red) as the angle of incidence increases:

```
angleFactor = θ / 90°
wlPos       = (λ - 380) / (780 - 380)       — 0 = violet, 1 = red
decayRate   = 1 + 5·(1 - wlPos)             — blue = 6, red = 1
spectralMod = (1 - angleFactor)^decayRate
T_final(λ) = T_fresnel · spectralMod
```

In practice:
- At **0°** (perpendicular): full spectrum, all harmonics present
- At **45°**: blue decays ~64× faster than red
- At **70°+** (grazing): almost only low frequencies, dark timbre

This is consistent with Rayleigh scattering and the selective absorption observed in real dielectric materials.

---

## 3. Beer-Lambert law (Thickness)

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

The effect on sound:
- **thickness < 1.0** — thinner material, more transmission, brighter sound
- **thickness = 1.0** — nominal material transmission, no modification
- **thickness > 1.0** — thicker material, more absorption, darker and heavier sound

Materials with low transmission in certain bands become practically opaque in those bands at high thickness values. For example, Ruby's blue transmission of 0.02 at thickness 2.0: `0.02² = 0.0004`.

---

## 4. Combined spectrum

The final spectrum feeding the synthesis engine is the product of three factors per wavelength:

```
S(λ) = L(λ) · M(λ) · F(λ, θ)
```

Where:
- `L(λ)` = spectral power distribution of the light source (Gaussian)
- `M(λ)` = material transmission curve (interpolated from 8 to 50 points)
- `F(λ, θ)` = spectral Fresnel transmission at incidence angle θ

For multiple active lights, spectra are summed and normalized:

```
S_combined(λ) = Σᵢ (intensityᵢ · Sᵢ(λ)) / Σᵢ intensityᵢ
```

Beer-Lambert attenuation and per-material gain compensation are then applied.

---

## 5. Spectrum → harmonics mapping

Each harmonic in the wavetable maps to a position in the visible spectrum via continuous linear interpolation:

```
specPos(h) = 1 - (h - 1) / (H - 1)
```

Where `h` = harmonic number (1 to H), `H` = maximum harmonic. This produces:
- **Harmonic 1** (fundamental) → specPos 1.0 → red (780nm)
- **Harmonic H** (highest) → specPos 0.0 → violet (380nm)

Amplitude is obtained by interpolating the 50-point spectrum and applying:

```
A_raw      = lerp(spectrum[i₀], spectrum[i₁], frac)
A_emphasis = A_raw³                         — spectral contrast
rolloff    = 1 / (1 + (h-1) · 0.05)        — natural per-harmonic rolloff
A_final(h) = A_emphasis · rolloff
```

The cube (`A³`) amplifies spectral differences: high-transmission values are preserved (`0.9³ = 0.73`) while low values are strongly suppressed (`0.1³ = 0.001`). This translates subtle optical differences into audible timbral contrast.

The final wavetable is built by additive synthesis:

```
wavetable[i] = Σₕ A_final(h) · sin(2π·h·i / TABLE_SIZE)
```

Five band-limited wavetables are generated to prevent aliasing:

| Band | Frequency range | Max harmonics |
|---|---|---|
| Low | < 200 Hz | 20 |
| Mid-Low | 200–400 Hz | 20 |
| Mid | 400–800 Hz | 16 |
| Mid-High | 800–1600 Hz | 12 |
| High | > 1600 Hz | 8 |

---

## 6. Sinusoidal wavefolder

The wavefolder applies a periodic non-linear function directly to the audio signal:

```
y = sin(drive · x)
drive = 1 + deformAmount · 14       — range: 1 to 15
```

Properties:
- **drive = 1** — `sin(x) ≈ x` for small amplitudes (Taylor series). Transparent.
- **drive > 1** — the signal folds each time it crosses `±π/drive`. At drive 15, a full-amplitude signal folds ~4.8 times per cycle.
- **Natural bounding** — `sin()` always outputs within [-1, 1] with no clipper needed.
- **Harmonic spectrum** — each fold generates both even and odd harmonics. Density grows with drive.

Unlike saturation (`tanh`), which compresses dynamic range, wavefolding is periodic — producing a denser, more musically complex harmonic spectrum.

---

## 7. Simplex noise gradient (Deformer)

The Deformer displaces the sphere's surface normals using the gradient of a 3D Simplex Noise field, calculated by central finite differences:

```
∂N/∂x = (noise(x+ε, y, z) - noise(x-ε, y, z)) / 2ε
∂N/∂y = (noise(x, y+ε, z) - noise(x, y-ε, z)) / 2ε
∂N/∂z = (noise(x, y, z+ε) - noise(x, y, z-ε)) / 2ε
```

With `ε = 0.01`. Full gradient: `∇N = (∂N/∂x, ∂N/∂y, ∂N/∂z)`.

**Tangential projection** — to ensure displacement only tilts the normal without changing its radial magnitude:

```
grad_radial     = ∇N · p̂
grad_tangential = ∇N - p̂ · grad_radial
n_displaced     = normalize(p̂ - grad_tangential · scale)
scale           = deformAmount · 0.3
```

The 3D Simplex Noise (Perlin 2001) evaluates the scalar field on a tetrahedral lattice using skew constant `F₃ = 1/3` and unskew `G₃ = 1/6`, with a 256-entry permutation table and 12 gradient vectors. It produces C¹ continuous values in [-1, 1] with isotropic distribution — no axis-alignment artifacts.

---

## 8. Physical Envelope

In Physical mode, the four ADSR stages are derived directly from optical properties:

**Attack** — photonic energy (light intensity):
```
attack = lerp(0.5, 0.005, avgIntensity)
```
Higher light intensity = more energy = faster transient.

**Decay** — material absorption × thickness:
```
decay = thickness · (1 - avgTransmission) · 1.5
```
A thick, opaque material absorbs more light and produces a longer decay.

**Sustain** — internal reflections (IOR):
```
sustain = IOR / 2.42
```
Normalized against Diamond (maximum IOR = 2.42). Higher IOR means a lower critical angle for total internal reflection (`θ_critical = arcsin(1/n)`), trapping more light inside the material — higher sustain. Diamond ≈ 0.95, Water ≈ 0.55.

**Release** — trapped light (IOR × thickness):
```
release = IOR · 0.2 · thickness
```
A dense, thick material releases light more slowly.

---

## 9. Light spectral distributions

Each light source has a spectral power distribution modeled as a Gaussian — a standard approximation of blackbody (Planck) distributions used in colorimetry and CG lighting:

```
I(λ) = base + peak · e^(-(λ - center)² / 2σ²)
```

| Light | Center (nm) | σ (nm) | Base | Peak | Approx. temperature |
|---|---|---|---|---|---|
| Sunset | 650 | 80 | 0.3 | 0.7 | ~2000K |
| Daylight | 550 | 120 | 0.5 | 0.5 | ~5500K |
| LED Cool | 470 | 90 | 0.4 | 0.6 | ~6500K |

The non-zero base values ensure no light has zero energy in any band, simulating the low-energy tail of a real Planck distribution.

---

## 10. Pitch modulation by light intensity

Light intensity modulates pitch as a deviation from equilibrium:

```
equilibrium  = activeCount · 0.5
deviation    = totalIntensity - equilibrium
maxDeviation = activeCount · 0.5
pitchOffset  = (deviation / maxDeviation) · 2.0 semitones
frequency    = f₀ · 2^(pitchOffset / 12)
```

This is consistent with the photon energy–frequency relationship (E = hν): more light energy → higher frequency.

---

## Material optical data

The transmission curves of the 10 materials are based on real optical data:

| Material | Optical basis |
|---|---|
| Diamond | Uniform ~95% transmission (colorless, IOR 2.42) |
| Ruby | Cr³⁺ in Al₂O₃ — d-d transitions absorb below 600nm |
| Sapphire | Fe²⁺/Ti⁴⁺ charge transfer in Al₂O₃ |
| Emerald | Cr³⁺ transmission window in Be₃Al₂Si₆O₁₈ |
| Amethyst | Fe⁴⁺ charge transfer in SiO₂ (bimodal) |
| Gold | 5d→6sp interband transition at ~500nm |
| Copper | Interband transition at ~590nm |
| Amber | Conjugated organic absorption in fossil resin |
| Water | O-H vibrational overtones in infrared/red |
| Obsidian | Fe₃O₄/Fe₂O₃ inclusions in amorphous volcanic glass |

---

[← Back to Elements](index)
