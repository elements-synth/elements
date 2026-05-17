# Elements — Spectral Science Reference

> This document records the scientific basis for every material's spectral curve:
> sources consulted, absorption band positions, conversion methodology, and
> the final sample-point values stored in `Physics.cpp`.
>
> **Policy**: Band positions must come from peer-reviewed literature or authoritative
> databases. Transmission *depths* at those positions are estimated from crystal-field
> theory and mineral optics — not directly measured. This is noted explicitly per material.
> Do not update a material's curve without updating this document.

---

## Sample-Point Architecture

All 13 materials use the **32-point grid** (380–780 nm uniform, Δλ ≈ 12.9 nm):

```
380, 393, 406, 419, 432, 445, 457, 470, 483, 496, 509, 522, 535, 548, 561, 574,
587, 600, 613, 626, 638, 651, 664, 677, 690, 703, 716, 728, 741, 754, 767, 780
```

Simple materials (Diamond, Water, Amber, Gold, Amethyst, Copper, Obsidian) were
originally designed at 16 pts and upsampled to 32 via linear interpolation from the
original 16-point source grid (380–780 nm, Δλ ≈ 26.7 nm). The upsampled values are
stored directly in `Physics.cpp` — no runtime interpolation from 16 pts.

**`MATERIAL_MAX_SAMPLES = 32`** in `Physics.h`. Each `Material` carries `numSamples`.
`interpolateMaterial()` uses `numSamples` — no hardcoded count anywhere.

---

## Measurement Conditions and Light Source Independence

A common question when reading spectral data: *under what light were these curves measured?*
The answer is that **the measurement light source is intentionally removed from the data** — all
sources used here report intrinsic material properties, not raw sensor readings tied to a
particular illuminant.

### How each measurement type normalises out the source

**Transmission spectrophotometry**
Used for: Amber, Ruby, Emerald, Sapphire, Alexandrite, Malachite (crystal reference),
Water (integrating cavity variant).

A broadband lamp (xenon arc or tungsten-halogen) passes through the sample, then a
monochromator isolates each wavelength. The instrument reports:

```
T(λ) = I_sample(λ) / I_reference(λ)
```

The source spectrum appears in both numerator and denominator and cancels exactly.
The result is a pure material property: the fraction of light transmitted at each wavelength.

**Absorption coefficient measurement — Beer-Lambert**
Used for: Water (Pope & Fry 1997).

The measured quantity is `µ_a(λ)` in units of 1/m (or 1/cm) — an intrinsic property of
the medium. We reconstruct transmission for any path length via `T = exp(−µ_a · d)`.
The original light source plays no role in the published coefficient table.

**Optical constants n, k (ellipsometry / reflectance)**
Used for: Gold, Copper (Palik 1985).

Ellipsometry measures the change in polarisation state of a reflected beam. The ratio of
the two polarisation components (p and s) is computed, eliminating the absolute intensity
of the source. The result is the complex refractive index `ñ = n + ik`, an intrinsic
electromagnetic property of the material.

**USGS diffuse reflectance spectroscopy**
Used for: Malachite (splib07, ASD FieldSpec spectrometer).

The ASD illuminates the sample with a tungsten-halogen lamp and records:

```
R(λ) = I_sample(λ) / I_Spectralon(λ)
```

Spectralon is a near-perfect Lambertian white reflector (R ≈ 0.99 across 380–2500 nm).
Dividing by it normalises out the lamp spectrum and detector response. The result is
a fractional reflectance — again a material property.

### Why this matters for Elements

Because all curves describe intrinsic material properties, they combine correctly with
any light source inside the plugin. When Elements multiplies a material's transmission
curve by a light source's spectral power distribution, it is performing exactly the
physics a spectroscopist reverses when computing a material measurement: the illuminant
spectrum and the material response are independent factors.

Illuminating Ruby with Sunset light in Elements is physically meaningful — it is equivalent
to asking "what wavelengths survive after passing through ruby-like absorption under a warm
red light?" The two spectra multiply correctly because they were measured independently.

### Caveats in our data

| Material | Caveat |
|----------|--------|
| Malachite | USGS measures powdered mineral (diffuse reflectance), not single-crystal transmission. Curve shape is correct; absolute depths estimated via Kubelka-Munk conversion. |
| Ruby, Emerald, Sapphire, Alexandrite | Birefringent crystals have slightly different spectra for ordinary vs extraordinary ray. Published data typically reports ordinary-ray values or an average. Our curves use the ordinary-ray (or averaged) form. |
| All gems | Absorption depths in our curves are estimated from crystal-field theory, not directly read from a transmission measurement. Band *positions* are verified from literature; band *depths* are approximate. |
| Neodymium | Band positions from laser spectroscopy (Nd:YAG / Nd:glass), accurate to ±5 nm. Depths approximate — actual values vary with Nd³⁺ concentration in the glass host. |

---

## Conversion Methods

### Beer-Lambert (Water)
`T(λ) = exp(−µ_a(λ) · d)`
- `µ_a` = absorption coefficient (1/cm) from Pope & Fry (1997)
- `d` = path length in cm. Used 100 cm (1 m) for chart.
- Data ends at 727.5 nm; 740–780 nm extrapolated from OH overtone slope.

### Kubelka-Munk (Malachite, inspiration)
`F(R) = (1 − R)² / (2R)`
- Applied to USGS splib07 reflectance `R` of powdered malachite (HS254.1B).
- Gives relative absorption profile K/S (absorption/scattering ratio).
- Used as shape reference only; absolute values re-estimated for crystal transmission.
- `T_normalized = 1 − F(R) / F_max` (maximum-normalised).

---

## Materials

---

### Diamond
**IOR**: n = 2.417 (@ 589 nm), dispersive 2.474 → 2.401 (380–780 nm)  
**k**: ≈ 0 across entire visible (Type IIa)  
**Chromophore**: none — widest bandgap of any natural mineral (~5.47 eV)

**Sources**:
- Sellmeier, W. (1871/1923 fit). Dispersion formula. Reproduced in Phillip & Taft (1964),
  *Phys. Rev.* 136, A1445–A1448. Also at refractiveindex.info (main/C).
- Thomas, M.E. & Tropf, W.J. (1993). "Optical Properties of Diamond."
  *APL Technical Digest* 14(1). https://www.jhuapl.edu/Content/techdigest/pdf/V14-N01/14-01-Thomas.pdf

**Band positions**: none in visible  
**Curve**: flat. Slight UV surface-scatter loss at 380 nm (Rayleigh).  
**Depth estimation**: n/a (confirmed transparent)  
**Points**: 32 (upsampled from 16-pt source via linear interp)

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.920 0.930 0.939 0.945 0.950 0.954 0.959 0.964 0.969 0.970 0.970 0.970 0.970 0.970 0.970 0.967

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.962 0.960 0.960 0.960 0.960 0.958 0.953 0.950 0.950 0.950 0.950 0.950 0.945 0.940 0.940 0.940
```

---

### Water
**IOR**: n = 1.333 (@ 589 nm)  
**Chromophore**: O–H vibrational overtones (NIR), Rayleigh scattering (UV)

**Sources**:
- Pope, R.M. & Fry, E.S. (1997). "Absorption Spectrum (380–700 nm) of Pure Water.
  II. Integrating Cavity Measurements." *Applied Optics* 36(33), 8710–8723.
  DOI: 10.1364/AO.36.008710
- Data table: https://omlc.org/spectra/water/data/pope97.txt (µ_a in 1/cm, 2.5 nm steps)

**Real data**: YES — µ_a values are directly measured.  
**Conversion**: Beer-Lambert, d = 100 cm.  
**Extrapolation**: 730–780 nm extrapolated from exponential fit to last 10 points
(OH overtone at ~740 nm causes steep rise; values are physically consistent).

**Points**: 32 (upsampled from 16-pt source via linear interp)

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.989 0.992 0.995 0.995 0.995 0.993 0.991 0.989 0.987 0.978 0.967 0.960 0.956 0.948 0.938 0.917

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.888 0.844 0.789 0.749 0.730 0.705 0.670 0.629 0.576 0.496 0.328 0.182 0.126 0.072 0.045 0.018
```

*Note: the steep drop at 727 nm is real — water absorbs strongly at ~740 nm (first
OH overtone). This makes Water sound like a steep low-pass filter in the red harmonics.*

---

### Amber
**IOR**: n ≈ 1.54  
**Chromophore**: Polycyclic aromatic hydrocarbons (perylene derivatives, succinic acid
polymers). UV cutoff from π→π* transitions. No sharp visible-range bands.

**Sources**:
- Wolfe, A.P. et al. (2025). "Spectroscopic Studies of Baltic Amber — Critical Analysis."
  *Molecules* 30(12), 2617. PMC12196071. https://pmc.ncbi.nlm.nih.gov/articles/PMC12196071/
- Reproduced in: GIA (Winter 2020). "Fluorescence Characteristics of Blue Amber."
  *Gems & Gemology* 56(4). https://www.gia.edu/gems-gemology/winter-2020-fluorescence-characteristics-of-blue-amber

**Band positions**: UV cutoff begins ~420–445 nm (Baltic); no sharp visible bands.  
**Curve**: smooth sigmoid UV absorption, broad warm transmission above 460 nm.  
**Depth estimation**: approximated from figure in PMC12196071 (qualitative match).  
**Points**: 32 (upsampled from 16-pt source via linear interp)

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.010 0.020 0.029 0.053 0.078 0.142 0.204 0.316 0.441 0.549 0.649 0.713 0.761 0.804 0.842 0.871

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.891 0.908 0.922 0.934 0.943 0.952 0.957 0.960 0.960 0.961 0.966 0.970 0.970 0.970 0.970 0.970
```

---

### Ruby
**IOR**: n = 1.762 (ordinary ray, @ 589 nm)  
**Chromophore**: Cr³⁺ substituting Al³⁺ in corundum (α-Al₂O₃)

**Sources**:
- Rossman, G.R. et al. (2022). "The Use of UV-Visible Diffuse Reflectance Spectrophotometry
  for a Fast, Preliminary Authentication of Gemstones." *Molecules* 27(15), 4716.
  PMC9330567. https://pmc.ncbi.nlm.nih.gov/articles/PMC9330567/
- Gaft, M. et al. (2022). "Influence of light path length on synthetic ruby color."
  *Sci. Reports* 12, 5889. PMC8993818. https://pmc.ncbi.nlm.nih.gov/articles/PMC8993818/

**Band positions** (verified from both sources):
| Band | λ (nm) | Transition | Confidence |
|------|--------|-----------|------------|
| 1 | 413 | ⁴A₂ → ⁴T₁ (Cr³⁺) | high |
| 2 | 550 | ⁴A₂ → ⁴T₂ (Cr³⁺) | high |
| R-line | 693–694 | ²E → ⁴A₂ (spin-forbidden) | high |
| Blue-violet window | 455–480 | between bands 1 and 2 | high |

**Depth estimation**: absorption minima from crystal-field splitting in corundum
(10Dq ≈ 18000 cm⁻¹ for Cr³⁺:Al₂O₃). Transmission depths estimated, not measured.  
**Points**: 32

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.02 0.02 0.03 0.06 0.10 0.12 0.13 0.10 0.08 0.05 0.03 0.02 0.02 0.02 0.04 0.08

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.15 0.30 0.55 0.75 0.85 0.90 0.93 0.95 0.97 0.96 0.97 0.97 0.97 0.97 0.98 0.98
```

---

### Gold
**IOR**: n = 0.47, k = 2.83 (@ 550 nm, complex)  
**Chromophore**: interband transition at ~500 nm (d-band → Fermi level). Metallic path.

**Sources**:
- Palik, E.D. (1985). *Handbook of Optical Constants of Solids*. Academic Press.
  (Johnson & Christy 1972 data incorporated.)
- refractiveindex.info/main/Au/Johnson

**Band positions**: interband edge at ~500 nm (sp→d absorption cuts off below 510 nm).  
**Depth estimation**: n/a — reflectance weights derived from published n,k.  
**Points**: 32 (upsampled from 16-pt source via linear interp; metallic — transmission[] used as spectral reflectance weights)

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.010 0.010 0.010 0.015 0.020 0.024 0.029 0.034 0.039 0.068 0.108 0.273 0.495 0.645 0.751 0.830

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.885 0.920 0.940 0.954 0.963 0.970 0.970 0.971 0.976 0.980 0.980 0.980 0.980 0.980 0.980 0.980
```

---

### Emerald
**IOR**: n = 1.565 (ordinary ray, @ 589 nm)  
**Chromophore**: Cr³⁺ substituting Al³⁺ in beryl (Be₃Al₂Si₆O₁₈)

**Sources**:
- Wood, D.L. & Nassau, K. (1968). "The characterization of beryl and emerald by visible
  and infrared absorption spectroscopy." *American Mineralogist* 53, 777–800.
  http://www.minsocam.org/ammin/AM53/AM53_777.pdf
- PMC9330567 (FORS authentication — same paper as ruby).

**Band positions** (verified):
| Band | λ (nm) | Transition | Confidence |
|------|--------|-----------|------------|
| 1 | 430 | ⁴A₂ → ⁴T₁ (Cr³⁺) | high |
| 2 | 610 | ⁴A₂ → ⁴T₂ (Cr³⁺) | high |
| R-line doublet | 680–683 | ²E → ⁴A₂ | high |
| Green window | 480–560 | between bands | high |

*Note: crystal field is weaker in beryl than corundum (10Dq ≈ 16000 cm⁻¹),
shifting ⁴T₂ from 550 nm (ruby) to 610 nm — this moves the red-end absorption
and opens the green window.*

**Depth estimation**: approximate from Wood & Nassau (1968) figures.  
**Points**: 32

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.04 0.07 0.09 0.07 0.04 0.12 0.28 0.48 0.65 0.78 0.87 0.90 0.89 0.88 0.82 0.70

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.50 0.28 0.14 0.08 0.06 0.06 0.06 0.05 0.04 0.04 0.03 0.03 0.03 0.02 0.02 0.02
```

---

### Amethyst
**IOR**: n = 1.544 (ordinary ray, @ 589 nm)  
**Chromophore**: [FeO₄/Li]⁰ hole centre (effective Fe⁴⁺) substituting Si⁴⁺ in quartz

**Sources**:
- Hatipoğlu, M. & Türk, N. (2020). "Study on the effect of heat treatment on amethyst
  color and the cause of coloration." *Sci. Reports* 10, 14927.
  PMC7483767. https://pmc.ncbi.nlm.nih.gov/articles/PMC7483767/
- Lehmann, G. & Moore, R.K. (1966). "Optical absorption of the d⁴ ion Fe⁴⁺ in
  pleochroic amethyst quartz." *Physics and Chemistry of Minerals*.

**Band positions** (verified):
| Band | λ (nm) | Assignment | Confidence |
|------|--------|-----------|------------|
| 1 | 345–360 | O²⁻ → Fe³⁺ charge transfer (UV edge) | high |
| 2 | 545 | [FeO₄]⁰ centre (main visible band) | high |

*The 545 nm band absorbs green/yellow, transmitting violet + red → purple.*

**Depth estimation**: V-shape absorption centred at 545 nm with violet and red wings.  
**Points**: 32 (upsampled from 16-pt source via linear interp)

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.880 0.861 0.841 0.812 0.782 0.736 0.691 0.595 0.484 0.363 0.238 0.163 0.110 0.105 0.129 0.162

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.202 0.246 0.294 0.342 0.387 0.432 0.472 0.507 0.531 0.547 0.532 0.519 0.509 0.499 0.490 0.480
```

---

### Sapphire
**IOR**: n = 1.762 (ordinary ray, @ 589 nm) — same host mineral as ruby  
**Chromophore**: Fe²⁺–Ti⁴⁺ intervalence charge transfer (IVCT) at ~570 nm (primary);
Fe³⁺ spin-forbidden bands at ~390 nm and ~450 nm (secondary)

**Sources**:
- Dubinsky, E.V. et al. (2020). "A Quantitative Description of the Causes of Color in
  Corundum." *Gems & Gemology* 56(1) (Spring 2020).
  https://www.gia.edu/gems-gemology/spring-2020-corundum-chromophores
  Supplementary data: https://www.gia.edu/doc/sp20-corundum-chromophores-appendix1.xlsx
- PMC9330567 (FORS — as above).

**Band positions** (verified):
| Band | λ (nm) | Assignment | Confidence |
|------|--------|-----------|------------|
| Fe³⁺ | 378–390 | ⁶A₁→⁴A₁,⁴E (spin-forbidden) | high |
| Fe³⁺ | ~450 | ⁶A₁→⁴T₂ | high |
| Fe²⁺–Ti⁴⁺ IVCT | 570 (broad, 415–620 nm) | intervalence charge transfer | high |
| Fe³⁺ pair | ~706 | paired-ion transition | moderate |

*IVCT is the dominant blue chromophore — much broader than Cr³⁺ bands.*

**Depth estimation**: broad IVCT modelled as Gaussian centred at 570 nm with σ ≈ 80 nm;
Fe³⁺ features add small dips at 393 nm and 445 nm.  
**Points**: 32

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.88 0.82 0.85 0.86 0.83 0.78 0.75 0.68 0.58 0.46 0.34 0.24 0.16 0.11 0.08 0.07

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.07 0.06 0.05 0.04 0.03 0.03 0.02 0.02 0.02 0.01 0.01 0.01 0.01 0.01 0.01 0.01
```

---

### Copper
**IOR**: n = 0.46, k = 2.83 (@ 550 nm, complex)  
**Chromophore**: interband transition at ~600 nm (deeper red character than Gold). Metallic.

**Sources**:
- Palik, E.D. (1985). *Handbook of Optical Constants of Solids*. Academic Press.
- refractiveindex.info/main/Cu/Johnson

**Points**: 32 (upsampled from 16-pt source via linear interp; metallic — transmission[] used as spectral reflectance weights)

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.010 0.010 0.010 0.010 0.010 0.010 0.010 0.014 0.019 0.020 0.020 0.023 0.028 0.036 0.046 0.055

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.065 0.111 0.189 0.274 0.363 0.461 0.561 0.663 0.769 0.853 0.868 0.882 0.902 0.921 0.936 0.950
```

---

### Obsidian
**IOR**: n ≈ 1.49–1.50  
**Chromophore**: Fe²⁺ + magnetite (Fe₃O₄) nanoparticles → broadband absorption.
Fe²⁺–Ti⁴⁺ IVCT adds weak NIR feature.

**Sources**:
- Cloutis, E.A. et al. (2021). "Visible and near-InfraRed (VNIR) reflectance of silicate
  glasses." *Icarus* 361. https://doi.org/10.1016/j.icarus.2021.114391
- Confirmed by USGS: obsidian not in splib07a (was in deprecated splib06).
  Described as featureless monotonic spectrum in Icarus 2021.

**Band positions**: none in 380–780 nm — featureless confirmed.  
**Curve**: monotonically rising UV→NIR (dark in blue-violet, less dark in red/NIR).  
**Depth estimation**: dark baseline from Fe broadband absorption; slope from published VNIR.  
**Points**: 32 (upsampled from 16-pt source via linear interp)

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.010 0.010 0.010 0.010 0.010 0.010 0.010 0.014 0.019 0.020 0.020 0.023 0.028 0.036 0.046 0.055

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.065 0.083 0.107 0.138 0.173 0.218 0.278 0.344 0.421 0.493 0.551 0.603 0.638 0.672 0.696 0.720
```

---

### Alexandrite
**IOR**: n = 1.745 (@ 589 nm)  
**Chromophore**: Cr³⁺ substituting Al³⁺ in chrysoberyl (BeAl₂O₄)

**Sources**:
- Taran, M.N. et al. (2020). "Explanation of the Colour Change in Alexandrites."
  *Sci. Reports* 10, 6645. PMC7145866.
  https://pmc.ncbi.nlm.nih.gov/articles/PMC7145866/
- Manning, P.G. (1965). "Crystal-Field Spectra of Chrysoberyl, Alexandrite, Peridot,
  and Sinhalite." *American Mineralogist* 50, 1982.
  http://www.minsocam.org/ammin/AM50/AM50_1972.pdf
- PMC9330567 (FORS — as above).

**Band positions** (verified):
| Band | λ (nm) | Transition | Confidence |
|------|--------|-----------|------------|
| 1 | 440 | ⁴A₂ → ⁴T₁ (Cr³⁺) | high |
| 2 | 572 | ⁴A₂ → ⁴T₂ (Cr³⁺) | high |
| R-line | 678–682 | ²E → ⁴A₂ | high |
| Green window | 490–520 | between bands | high |
| Red window | >620 | above band 2 | high |

*The two transmission windows cause the alexandrite effect: under daylight (blue-rich)
the green window dominates → green appearance; under incandescent (red-rich) the red
window dominates → red/purplish appearance.*

**Depth estimation**: approximate from PMC7145866 figure 3 (transmittance vs λ).  
**Points**: 32

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.07 0.09 0.10 0.08 0.05 0.08 0.22 0.45 0.62 0.72 0.74 0.70 0.60 0.42 0.25 0.16

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.20 0.32 0.46 0.58 0.68 0.76 0.82 0.86 0.88 0.88 0.88 0.87 0.88 0.88 0.89 0.90
```

---

### Malachite
**IOR**: n = 1.85 (@ 589 nm)  
**Chromophore**: Cu²⁺ in Cu₂CO₃(OH)₂ (two transitions)

**Sources**:
- USGS Spectral Library Version 7 (Kokaly et al. 2017). Sample HS254.1B (ASD spectrometer,
  diffuse reflectance, 380–2500 nm).
  DOI: https://dx.doi.org/10.5066/F7RR1WDJ
- Kubelka-Munk conversion applied to derive absorption profile.
- CHSOS Pigments Checker (art conservation database, Kremer K-10300):
  https://chsopensource.org/malachite-k-10300/

**Band positions** (verified):
| Band | λ (nm) | Assignment | Confidence |
|------|--------|-----------|------------|
| LMCT | <450 | ligand-to-metal charge transfer (Cu²⁺←O²⁻) | high |
| d–d | ~700–800 | Cu²⁺ ¹⁰Dq ligand-field band | high |
| Green window | 490–560 | between LMCT and d–d | high |

**USGS K-M data (HS254.1B reflectance → K-M):**
The USGS ASD measurement gives diffuse reflectance R in 10 nm steps (380–780 nm).
K-M: F(R) = (1−R)²/(2R), then T_normalized = 1 − F(R)/F_max.
Key derived values:
- Peak green reflectance at 530–540 nm (R ≈ 0.349)
- LMCT rises steeply below 450 nm (R drops from 0.265 at 430 to 0.092 at 380)
- d–d absorption: R declines from 0.348 at 540 to 0.169 at 780 nm

*Final curve uses USGS K-M shape for the green window and red decline;
absolute transmission depths estimated for crystal (not powder) transmission.*  
**Points**: 32

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.08 0.12 0.18 0.25 0.34 0.45 0.56 0.66 0.74 0.81 0.86 0.89 0.90 0.88 0.84 0.78

wl:  587  600  613  626  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.70 0.62 0.53 0.45 0.38 0.32 0.27 0.22 0.19 0.16 0.14 0.13 0.12 0.12 0.11 0.11
```

---

### Neodymium
**IOR**: n = 1.636 (Nd-doped borosilicate glass)  
**Chromophore**: Nd³⁺ f–f transitions from ⁴I₉/₂ ground state

**Sources**:
- Kaminskii, A.A. (1981). *Laser Crystals: Their Physics and Properties*.
  Springer-Verlag. (Nd³⁺ absorption spectrum in glass, Chapter 3.)
- Weber, M.J. (1990). "Optical Properties of Nd in Crystals and Glasses."
  in *Optical Properties of Excited States in Solids*, Plenum Press.
- Nd:YAG absorption cross-section data widely reproduced in laser physics literature;
  band positions accurate to ±5 nm.

**Band positions** (verified from laser spectroscopy):
| Band | λ (nm) | Transition (from ⁴I₉/₂) | Confidence |
|------|--------|------------------------|------------|
| 1 | 432 | → ⁴G₁₁/₂ | high |
| 2 | 521 | → ⁴G₉/₂ + ²K₁₅/₂ | high |
| 3 | 583 | → ⁴G₅/₂ (strongest visible) | high |
| 4 | 628 | → ²H₉/₂ + ⁴G₅/₂ | high |
| 5 | 680 | → ⁴F₇/₂ + ⁴S₃/₂ | high |
| 6 | 745 | → ⁴F₅/₂ + ²H₉/₂ | high |

*All six bands are well-established from Nd:YAG and Nd:glass laser absorption measurements.
Between bands, transmission is ~0.80–0.88 (Nd glass is mostly transparent).*

**Points**: 32

```
wl:  380  393  406  419  432  445  457  470  483  496  509  522  535  548  561  574
T:  0.72 0.78 0.82 0.85 0.55 0.80 0.86 0.82 0.75 0.85 0.78 0.18 0.72 0.86 0.88 0.30

wl:  587  599  612  625  638  651  664  677  690  703  716  728  741  754  767  780
T:  0.06 0.52 0.78 0.38 0.72 0.85 0.78 0.30 0.68 0.82 0.84 0.72 0.38 0.76 0.86 0.88
```

---

## Pending / Future Work

- [ ] Source USGS reflectance data for Malachite and apply K-M quantitatively with
      thickness calibration (currently shape-informed only)
- [ ] Obtain Caltech Mineral Spectroscopy ASCII files for Emerald + Amethyst
      (server currently offline — check periodically)
- [ ] GIA Excel appendix (sp20-corundum-chromophores-appendix1.xlsx) for
      quantitative Sapphire chromophore absorption cross-sections
- [ ] Amber: find published transmittance table (not just figures) for Baltic amber
- [ ] Consider increasing Water to 32 pts around the steep 700–780 nm OH overtone region
- [ ] Iron, Brass, Fluorite — future material candidates

## Change Log

| Date | Material | Change | Reason |
|------|----------|--------|--------|
| 2026-04-15 | Alexandrite, Malachite | Added (8 pts, placeholder) | Expand palette |
| 2026-04-15 | Neodymium | Added (8 pts) | Rare-earth comb spectrum |
| 2026-05-02 | Neodymium | Upgraded to 32 pts | Capture f–f band structure |
| 2026-05-07 | All | Architecture: MATERIAL_MAX_SAMPLES=32, numSamples field | Variable sample count |
| 2026-05-07 | All | Redesigned to 16/32 pts, physically-motivated curves | Scientific grounding |
| 2026-05-07 | All | All materials upgraded to 32 pts; simple materials upsampled from 16-pt source | Richer harmonic content, confirmed audible improvement |
