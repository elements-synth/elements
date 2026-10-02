---
layout: default
title: Parameters · Elements
---

[← Back to Elements](index)

# Parameters

Elements has 22 automatable parameters exposed to your DAW, plus 5 manual parameters controlled directly from the UI. This page documents all of them.

---

## Scene

These parameters define the physical scene: the core of how Elements generates sound.

**Material**
The optical material applied to the geometry. Determines the spectral transmission curve and index of refraction (IOR), which together define which harmonics are produced and how the Fresnel response behaves. See [Materials & Geometry](materials-and-geometry) for a detailed breakdown of each material's character.

- Options: Diamond, Water, Amber, Ruby, Gold, Emerald, Amethyst, Sapphire, Copper, Obsidian
- Default: Diamond
- *Not automatable · Saved with project*

---

**Geometry**
The 3D geometry whose surface normals interact with the light. Determines the number of Fresnel sampling points, their distribution, and how rotation affects the timbre. See [Materials & Geometry](materials-and-geometry) for details on each geometry's behavior.

- Options: Cube, Sphere, Torus, Dodecahedron
- Default: Cube
- *Not automatable · Saved with project*

---

**Thickness** · `0.1 – 2.0` · Default: `0.5`
The thickness of the material along the light path, modeled using Beer-Lambert attenuation. Lower values produce a brighter, thinner sound; higher values increase absorption and darken the timbre. Also affects Decay and Release times in Physical Envelope mode.

- *Automatable*

---

**Rotation X / Y / Z** · `0° – 360°` · Default: `0°`
Rotates the geometry on each axis. Rotation changes the angle at which the light hits each surface normal, modifying the Fresnel response and therefore the harmonic content. The effect of rotation varies significantly between geometries: the Cube produces abrupt timbral jumps, while the Dodecahedron and Torus respond smoothly and continuously.

- *Automatable · Automate for continuous timbral movement*

---

## Lights

Elements has three light slots: **Key**, **Fill**, and **Rim**. Each slot has an independent light source selection and intensity control.

**Light Enable (Key / Fill / Rim)**
Activates or deactivates each light slot. All three lights off produces silence.

- Options: On / Off
- Default: Off
- *Not automatable · Not saved with project*

---

**Light Source (Key / Fill / Rim)**
Selects the spectral emission type for each light slot. The emission curve of the light must overlap with the transmission curve of the active material to produce sound. No overlap means near-silence.

- Options: Sunset (650nm peak), Daylight (550nm peak), LED Cool (470nm peak)
- *Not automatable · Not saved with project*

> **Note:** Light source selection and on/off state are not saved with the project. You will need to reassign them when reopening a session.

---

**Key Intensity / Fill Intensity / Rim Intensity** · `0.0 – 1.0` · Default: `0.5`
Controls the intensity of each light source. Intensity has a direct bidirectional relationship with pitch: at 0.5 the pitch is neutral, above 0.5 it rises, below 0.5 it falls. This makes light intensity one of the most expressive performance parameters in Elements.

In Physical Envelope mode, Key Intensity also drives the Attack time: higher intensity produces a faster attack.

- *Automatable*

---

## Amplitude Envelope

**Envelope Mode**
Switches between two envelope behaviors.

- **Classic**: standard ADSR with fully manual control over all four stages.
- **Physical**: Attack, Decay, Sustain, and Release are derived automatically from the optical properties of the active material, its thickness, and the light intensity. The manual ADSR knobs are ignored in this mode.

- Default: Classic
- *Automatable*

---

**Attack** · `0.001 – 2.0 s` · Default: `0.01 s`
Rise time from silence to full amplitude. Ignored in Physical mode.

- *Automatable*

---

**Decay** · `0.001 – 2.0 s` · Default: `0.1 s`
Time to fall from peak to sustain level after the attack phase. Ignored in Physical mode.

- *Automatable*

---

**Sustain** · `0.0 – 1.0` · Default: `0.7`
Amplitude level held while a note is held after the decay phase. Ignored in Physical mode.

- *Automatable*

---

**Release** · `0.001 – 2.0 s` · Default: `0.3 s`
Time to fall from sustain level to silence after a note is released. Ignored in Physical mode.

- *Automatable*

---

## Filter

A global filter applied to the output signal, with its own independent ADSR envelope.

### How the filter works in Elements

The filter is a standard 12 dB per octave state-variable filter (Lowpass, Highpass or Bandpass). It sits **after** the oscillators, on the mixed signal of both materials, and it knows nothing about materials, lights or geometry. It behaves like the filter on any other synthesizer, over the full 20 Hz to 20 kHz range.

What differs from a conventional synth is the signal it filters. The oscillator always contains harmonics all the way up toward the top of the spectrum, so a lowpass has something to cut at every cutoff setting, whichever material you choose. The material decides how much energy each harmonic carries, so the same cutoff sounds different on Diamond than on Ruby, just as it would on different waveforms of a conventional synth.

A few things worth knowing when you sweep the cutoff:

- **The effect depends on the note.** A lowpass is most audible between the note's fundamental and roughly its 20th harmonic. For a 110 Hz note that is about 110 Hz to 2 kHz. Below the fundamental the sound disappears entirely. Far above the 20th harmonic the filter only trims the quiet upper partials.
- **The Cutoff knob is not linear.** Its midpoint is about 2 kHz, so the lower half of the knob covers 20 Hz to 2 kHz, where most of the action is. Moving a knob a fixed distance changes the cutoff by roughly the same number of octaves, not the same number of hertz.
- **Materials with weak low harmonics need a higher cutoff to be heard working.** Sapphire, for example, has very little energy near the fundamental, so a low cutoff on it sounds nearly silent.
- **The envelope moves the cutoff in octaves.** At full Filter Env Amt the envelope can open the cutoff up to 7 octaves above the Cutoff setting.

**Filter Type**
- Options: Lowpass, Highpass, Bandpass
- Default: Lowpass
- *Automatable*

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
Depth of the filter envelope modulation over the cutoff frequency. At 0.0 the filter envelope has no effect. At 1.0 it can open the cutoff up to 7 octaves.

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

The Deformer is currently available for the **Sphere geometry only**. It applies a 3D Simplex Noise field to the surface, affecting both the spectral path and the audio path simultaneously. See [The Science](science#7-simplex-noise-gradient-deformer) for the full technical explanation.

**Deform Amount** · `0.0 – 1.0` · Default: `0.0`
Controls the intensity of the deformation. At 0.0 the sphere is undeformed and rotation has no timbral effect. As the value increases, the displaced normals introduce Fresnel variation across the surface, timbral drift sets in, and the sinusoidal wavefolder drive increases from 1 to 15, adding progressively denser harmonic content.

- *Automatable*

---

**Deform Frequency** · `0.5 – 10.0` · Default: `2.0`
Spatial frequency of the Simplex Noise field. Lower values produce broad, smooth deformations; higher values produce finer, more detailed surface variation.

- *Automatable*

---

## Output

**Volume** · `0.0 – 1.0` · Default: `0.95`
Master output volume. A soft clipper (tanh) is applied to the output. If you experience saturation at high polyphony, reduce this value or lower the light intensities.

- *Not automatable · Saved with project*

---

## Automation summary

| Parameter | Automatable | Saved |
|---|---|---|
| Material | No | Yes |
| Geometry | No | Yes |
| Thickness | Yes | Yes |
| Rotation X / Y / Z | Yes | Yes |
| Light Enable (×3) | No | No |
| Light Source (×3) | No | No |
| Key / Fill / Rim Intensity | Yes | Yes |
| Envelope Mode | Yes | Yes |
| Attack / Decay / Sustain / Release | Yes | Yes |
| Filter Type | Yes | Yes |
| Cutoff / Resonance | Yes | Yes |
| Filter Env Amt | Yes | Yes |
| Filter Attack / Decay / Sustain / Release | Yes | Yes |
| Deform Amount | Yes | Yes |
| Deform Frequency | Yes | Yes |
| Volume | No | Yes |

---

[← Back to Elements](index)
