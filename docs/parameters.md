---
layout: default
title: Parameters · Elements
---

[← Back to Elements](index)

# Parameters

Elements exposes **35 parameters** to your DAW for automation, and keeps a further group of selectors (materials, geometry, light sources and light on/off) that are controlled from the interface only. This page documents all of them, in the order of the signal path: scene, lights, oscillator blend, envelopes, filter, deformer, chorus, and output.

Every parameter below marked *Automatable* appears in your host's automation list under the name shown in bold. Everything marked *Saved with project* is restored when you reopen the session, and all of them are saved in presets.

---

## Scene

> **In short:** Pick the materials and the shape; Thickness and Rotation decide how the light meets them.
{: .tldr}

These parameters define the physical scene: the core of how Elements generates sound.

**Material A / Material B** (MAT A / MAT B)
The optical material of each oscillator. A material determines the spectral transmission curve and index of refraction (IOR), which together define which harmonics are produced and how the Fresnel response behaves. Oscillator A always plays; oscillator B is blended in with the controls in the next section. See [Materials & Geometry](materials-and-geometry) for a breakdown of each material's character.

- Options: Diamond, Water, Amber, Ruby, Gold, Emerald, Amethyst, Sapphire, Copper, Obsidian, Alexandrite, Malachite, Neodymium
- Default: Diamond (both)
- *Not automatable · Saved with project*

The swap button between the two selectors exchanges A and B.

---

**Geometry** (GEO)
The 3D geometry whose surface normals interact with the light. It determines how many Fresnel sampling points there are, how they are weighted, and how strongly rotation changes the timbre. Both oscillators share the same geometry, rotation and lights. See [Materials & Geometry](materials-and-geometry#geometry) for the behaviour of each one.

- Options: Cube, Sphere, Torus, Dodecahedron, Teapot
- Default: Sphere
- *Not automatable · Saved with project*

---

**Thickness** · `0.1 – 2.0` · Default: `0.5`
The thickness of the material along the light path, modelled with Beer-Lambert attenuation. Lower values give a brighter, thinner sound; higher values absorb more and darken the timbre. In Physical envelope mode it also lengthens Decay and Release. Gold and Copper are metals and do not transmit light, so Thickness has no effect on their spectrum.

- *Automatable*

---

**Rotation X / Y / Z** · `0° – 360°` · Default: `0°`
Rotates the geometry on each axis. Rotation changes the angle at which the light meets each surface normal, which changes the Fresnel response and so the harmonic content. How much it matters depends on the geometry: the Cube changes in sharp steps, the Torus strongly but smoothly, the Dodecahedron and Teapot more gently. The undeformed Sphere does not respond to rotation at all (see Deformer).

- *Automatable · Automate for continuous timbral movement*

---

## Lights

> **In short:** Three lights, each with a source, a switch and an intensity. Intensity is also a pitch control.
{: .tldr}

Elements has three light slots: **Key**, **Fill** and **Rim**, each with its own source, on/off switch and intensity.

**Light Enable (Key / Fill / Rim)**
Switches each slot on or off. With all three lights off there is no light to shape, and the sound is silent.

- Default: Key on, Fill and Rim off
- *Not automatable · Saved with project*

---

**Light Source (Key / Fill / Rim)**
The spectral emission of each slot. The sound is the product of the light, the material and the Fresnel response, so a light can only bring out what the material transmits.

- Options: Sunset (650 nm peak), Daylight (550 nm peak), LED Cool (470 nm peak)
- Default: Key = Sunset, Fill = Daylight, Rim = LED Cool
- *Not automatable · Saved with project*

---

**Key Intensity / Fill Intensity / Rim Intensity** · `0.0 – 1.0` · Default: `0.5`
How bright each light is. Intensity also moves the pitch: with the lights that are on, 0.5 is neutral, raising the intensities above 0.5 raises the pitch and lowering them below 0.5 lowers it, by up to **±2 semitones** when every active light is at an extreme. This makes light intensity one of the most expressive performance parameters in Elements.

In Physical envelope mode, the average intensity of the active lights also sets the Attack: brighter light gives a faster attack.

<figure class="clip">
  <img src="{{ '/assets/images/science/pitch-vs-intensity.png' | relative_url }}" alt="Graph of pitch offset against light intensity for one, two and three lights" loading="lazy">
  <figcaption>Pitch follows intensity: neutral at 0.5, up to ±2 semitones. With more lights on, a single slider moves the total less.</figcaption>
</figure>

- *Automatable*

---

## Oscillator blend

> **In short:** Oscillator B joins A through one of four blend modes; Mix says how much.
{: .tldr}

Each voice runs two oscillators. The blend controls decide how oscillator B is combined with oscillator A. See [The Science](science#9-dual-oscillators-and-blend-modes) for the formulas.

**Blend Mode** (BLEND)
How A and B are combined.

- **Ring Mod**: multiplies A by B. Metallic, inharmonic sum and difference frequencies.
- **AM**: B modulates the amplitude of A. Higher depth gives choppier, more rhythmic results.
- **XOR**: subtracts B from A. Hollow and subtractive; the parts where the two spectra differ come through.
- **FM**: B modulates the phase of A.

- Default: Ring Mod
- *Automatable*

<figure class="clip">
  <img src="{{ '/assets/images/science/blend-modes.png' | relative_url }}" alt="Waveforms of oscillators A and B and the Ring Mod, AM, XOR and FM results" loading="lazy">
  <figcaption>The four blend modes on two real material waveforms (Ruby and Emerald).</figcaption>
</figure>

---

**Mix Amount** (MIX) · `0% – 100%` · Default: `0%`
Crossfades from oscillator A alone (0%) to the full blend result (100%). At 0% oscillator B is not computed at all.

- *Automatable*

---

**AM Depth** · `0.0 – 1.0` · Default: `0.5`
The modulation depth for the AM and FM modes (it acts as the FM index in FM mode, from 0 to 4 cycles). It has no effect in Ring Mod or XOR.

- *Automatable*

---

**Osc B Detune** (DETUNE) · `−100 – +100 cents` · Default: `0`
Detunes oscillator B relative to A.

- *Automatable*

---

**MUTE A**
Removes oscillator A from the blend so you can hear oscillator B on its own. A monitoring aid: it is not saved with the project or in presets.

- *Not automatable · Not saved*

---

## Amplitude Envelope

> **In short:** Classic is a plain ADSR you set yourself. Physical reads the shape from the materials and the lights.
{: .tldr}

**Envelope Mode**
Switches between two envelope behaviours.

- **Classic**: a standard ADSR with manual control over all four stages.
- **Physical**: Attack, Decay, Sustain and Release are derived from the optical properties of the active materials, the Thickness and the light intensities, and the manual knobs are ignored. With a blend active, the material-dependent values are interpolated between A and B by Mix.
  - Attack: from 0.5 s (dim lights) down to 5 ms (bright lights).
  - Decay: longer for thick, absorptive materials (`Thickness × (1 − average transmission) × 1.5`).
  - Sustain: higher for high-IOR materials (`IOR / 2.42`, between 0.1 and 0.95).
  - Release: longer for high-IOR, thick materials (`IOR × 0.2 × Thickness`).

- Default: Classic
- *Automatable*

<figure class="clip">
  <img src="{{ '/assets/images/parameters/amp-envelope.png' | relative_url }}" alt="Classic default envelope and two Physical envelopes for Diamond and Obsidian" loading="lazy">
  <figcaption>Classic uses your four knobs. Physical derives the shape: Diamond is short and sustained, Obsidian takes longer to settle.</figcaption>
</figure>

---

**Attack** · `0.001 – 2.0 s` · Default: `0.01 s`
Rise time from silence to full amplitude. Ignored in Physical mode.

- *Automatable*

---

**Decay** · `0.001 – 2.0 s` · Default: `0.1 s`
Time to fall from the peak to the sustain level. Ignored in Physical mode.

- *Automatable*

---

**Sustain** · `0.0 – 1.0` · Default: `0.7`
The level held while a note is held. Ignored in Physical mode.

- *Automatable*

---

**Release** · `0.001 – 2.0 s` · Default: `0.3 s`
Time to fall from the sustain level to silence after a note is released. Ignored in Physical mode.

- *Automatable*

---

## Filter

> **In short:** A 12 dB/octave filter on the final mix, with its own envelope. The materials do not touch it.
{: .tldr}

A global filter applied to the mixed output of both oscillators, with its own independent ADSR envelope. The filter envelope stays manual in both Classic and Physical mode.

<details markdown="1">
<summary>How the filter works in Elements</summary>

### How the filter works in Elements

The filter is a standard 12 dB per octave state-variable filter (Lowpass, Highpass or Bandpass). It sits **after** the oscillators and knows nothing about materials, lights or geometry. It behaves like the filter on any other synthesizer, over the full 20 Hz to 20 kHz range.

What differs from a conventional synth is the signal it filters. The oscillator always contains harmonics all the way up toward the top of the spectrum, so a lowpass has something to cut at every cutoff setting, whichever material you choose. The material decides how much energy each harmonic carries, so the same cutoff sounds different on Diamond than on Ruby, just as it would on different waveforms of a conventional synth.

A few things worth knowing when you sweep the cutoff:

- **The effect depends on the note.** A lowpass is most audible between the note's fundamental and roughly its 20th harmonic. For a 110 Hz note that is about 110 Hz to 2 kHz. Below the fundamental the sound disappears entirely. Far above the 20th harmonic the filter only trims the quiet upper partials.
- **The Cutoff knob is not linear.** Its midpoint is about 2 kHz, so the lower half of the knob covers 20 Hz to 2 kHz, where most of the action is. Moving the knob a fixed distance changes the cutoff by roughly the same number of octaves, not the same number of hertz.
- **Materials with weak low harmonics need a higher cutoff to be heard working.** Sapphire, for example, has very little energy near the fundamental, so a low cutoff on it sounds nearly silent.
- **The envelope moves the cutoff in octaves.** At full Filter Env Amt the envelope can open the cutoff up to 7 octaves above the Cutoff setting.

</details>

**Filter Type**
- Options: Lowpass, Highpass, Bandpass
- Default: Lowpass
- *Automatable*

<figure class="clip">
  <img src="{{ '/assets/images/parameters/filter-response.png' | relative_url }}" alt="Frequency response of the lowpass, highpass and bandpass filters at five resonance values" loading="lazy">
  <figcaption>The real filter, measured from its own code at a 2 kHz cutoff. Resonance adds a peak at the cutoff without changing the slopes.</figcaption>
</figure>

---

**Cutoff** · `20 Hz – 20,000 Hz` · Default: `2000 Hz`
Cutoff frequency of the filter. The knob is non-linear, with its midpoint at about 2 kHz.

- *Automatable*

---

**Resonance** · `0.5 – 10.0` · Default: `1.0`
Emphasis at the cutoff frequency. Higher values produce a more pronounced peak.

- *Automatable*

---

**Filter Env Amt** · `0.0 – 1.0` · Default: `0.0`
Depth of the filter envelope over the cutoff. At 0.0 the envelope has no effect; at 1.0 it can open the cutoff up to 7 octaves.

<figure class="clip">
  <img src="{{ '/assets/images/parameters/filter-envelope.png' | relative_url }}" alt="The filter envelope and the cutoff it produces at three amounts" loading="lazy">
  <figcaption>With the Cutoff knob at 300 Hz, a full-amount envelope throws the cutoff all the way up to the top of the range and sweeps it back. Lower amounts open it less.</figcaption>
</figure>

- *Automatable*

---

**Filter Attack** · `0.001 – 2.0 s` · Default: `0.01 s`
Attack time of the filter envelope.

- *Automatable*

---

**Filter Decay** · `0.001 – 2.0 s` · Default: `0.3 s`
Decay time of the filter envelope.

- *Automatable*

---

**Filter Sustain** · `0.0 – 1.0` · Default: `0.0`
Sustain level of the filter envelope.

- *Automatable*

---

**Filter Release** · `0.001 – 2.0 s` · Default: `0.3 s`
Release time of the filter envelope.

- *Automatable*

---

## Deformer

> **In short:** Roughens the sphere so the sound moves. Sphere only.
{: .tldr}

The Deformer turns the smooth sphere into a bumpy, evolving surface, and is available for the **Sphere geometry only** (its controls are greyed out for the other geometries). It acts on the sound in three layers, all driven by one animated noise field: tilted surface normals with scattering, a per-harmonic shimmer, and a wavefolder. See [The Science](science#8-the-deformer) for the full explanation.

**Deform Amount** · `0.0 – 1.0` · Default: `0.0`
How bumpy the surface is. At 0.0 the sphere is perfect and rotation has no timbral effect. As the value rises, displaced normals add Fresnel variation across the surface, rough-surface scattering darkens the spectrum, the harmonics begin to drift in level and phase, and the wavefolder adds harmonic density. The change is smoothly faded in over the first part of the range, so switching Deform on does not jump.

- *Automatable*

---

**Deform Frequency** (Freq) · `0.5 – 10.0` · Default: `2.0`
The spatial detail of the noise field. Low values give broad, smooth deformations and a coherent wobble; high values give finer surface variation and a more independent, twinkling movement between harmonics.

- *Automatable*

---

**Deform Rate** (Rate) · `0.0 – 3.0` · Default: `1.0`
How fast the noise field evolves over time. At 0 the surface is frozen and the sound is static; higher values make the texture move faster.

- *Automatable*

---

**Noise Type** (Noise)
The character of the noise field.

- **Simplex**: smooth and organic.
- **Alligator**: rounded, blob-like cells with no sharp edges.
- **Worley**: cellular, with sharp Voronoi-like ridges.

- Default: Simplex
- *Automatable*

<figure class="clip">
  <img src="{{ '/assets/images/science/deformer.png' | relative_url }}" alt="A diamond spectrum darkening as Deform rises, and the waveform changing from front to back" loading="lazy">
  <figcaption>Real engine output for Diamond on a Sphere. Raising Deform lowers and tilts the spectrum and reshapes the waveform.</figcaption>
</figure>

---

## Chorus

> **In short:** A trail of delayed, quieter copies of the sound.
{: .tldr}

Chorus duplicates the object into a trail of copies receding behind it; each copy is a delayed, quieter echo of the sound. See [The Science](science#10-chorus).

**Chorus** (on/off) · Default: Off
- *Automatable*

---

**Chorus Voices** · `2 – 5` · Default: `2`
The total number of copies, including the original.

- *Automatable*

---

**Chorus Spread** · `5 – 40 ms` · Default: `15 ms`
The delay between neighbouring copies.

- *Automatable*

---

**Chorus Decay** · `0.0 – 0.9` · Default: `0.4`
The fraction of energy lost with each copy, so later copies are quieter. The copies in the viewport fade by the same amount.

- *Automatable*

---

**Chorus Wobble** · `0.0 – 1.0` · Default: `0.4`
How much each copy's delay drifts over time. This movement is what makes it sound like a chorus rather than a fixed comb filter.

- *Automatable*

<figure class="clip">
  <img src="{{ '/assets/images/science/chorus-taps.png' | relative_url }}" alt="Bar charts of chorus copy gains at three decay settings" loading="lazy">
  <figcaption>Each copy is a delayed echo, quieter by the Decay amount.</figcaption>
</figure>

---

## Output

> **In short:** Transpose shifts the register; Volume sets the master level.
{: .tldr}

**Transpose** · `−24 – +24 semitones` · Default: `0`
Shifts the register of new notes. In the interface it is a five-step octave selector (−2 OCT to +2 OCT) next to the keyboard, and the keyboard's octave labels follow it. It is applied when a note starts, so changing it never bends a note that is already sounding. The parameter itself moves in semitones if you automate it.

- *Automatable*

---

**Volume** · `0.0 – 1.0` · Default: `0.95`
Master output volume. A soft clipper (tanh) is applied to the output. If you hear saturation at high polyphony, lower this or the light intensities.

- *Automatable*

---

## Automation summary

> **In short:** all 35 parameters can be automated and are saved with the project. Only the material, geometry and light selectors are interface-only, and MUTE A is not saved.
{: .tldr}

<details markdown="1">
<summary>Full automation and saving tables</summary>

**Automatable and saved with the project (all 35):**

| Group | Parameters |
|---|---|
| Scene | Thickness, Rotation X / Y / Z |
| Lights | Key / Fill / Rim Intensity |
| Blend | Blend Mode, Mix Amount, AM Depth, Osc B Detune |
| Amp envelope | Envelope Mode, Attack, Decay, Sustain, Release |
| Filter | Filter Type, Cutoff, Resonance, Filter Env Amt, Filter Attack / Decay / Sustain / Release |
| Deformer | Deform Amount, Deform Frequency, Deform Rate, Noise Type |
| Chorus | Chorus on/off, Voices, Spread, Decay, Wobble |
| Output | Transpose, Volume |

**Interface-only, saved with the project:**

| Parameter | Notes |
|---|---|
| Material A / Material B | Not automatable |
| Geometry | Not automatable |
| Light Enable (×3) | Not automatable |
| Light Source (×3) | Not automatable |

**Not saved:** MUTE A.

All of the above, except MUTE A, are stored in presets.

</details>

---

[← Back to Elements](index)
