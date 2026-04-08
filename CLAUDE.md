# Elements - Spectral Synthesizer

## Project Location
`/Users/matiasderose/Documents/JUCE_Projects/Elements`

> **NOTE**: There is also an older Python prototype at `/Users/matiasderose/Documents/Elements/` — that is NOT this project. This is the C++/JUCE port.

## Overview
Audio plugin (VST3/AU/Standalone) — a spectral synthesizer where materials (diamond, ruby, gold, etc.) shape the harmonic spectrum of sound through optical physics simulation. Each material has wavelength-dependent transmission properties that filter harmonics. 3 point lights with spectral power distributions interact with the material and geometry via Fresnel angle calculations.

## Tech Stack
- **Framework**: JUCE 7+ (C++17)
- **Build**: Projucer → Xcode (macOS, Universal Binary)
- **OpenGL**: juce_opengl module for 3D viewport
- **Target**: macOS 11.0+, Universal Binary (arm64 + x86_64), formats: Standalone, AU, VST3

## Build Commands
```bash
# Build VST3 Release — Universal Binary (ALWAYS use this for distribution)
xcodebuild -project Builds/MacOSX/Elements.xcodeproj -scheme "Elements - VST3" -configuration Release ARCHS="arm64 x86_64" ONLY_ACTIVE_ARCH=NO build 2>&1 | tail -20

# Build Standalone Release — Universal Binary
xcodebuild -project Builds/MacOSX/Elements.xcodeproj -scheme "Elements - Standalone Plugin" -configuration Release ARCHS="arm64 x86_64" ONLY_ACTIVE_ARCH=NO build 2>&1 | tail -20

# Build Debug (active arch only, faster iteration)
xcodebuild -project Builds/MacOSX/Elements.xcodeproj -scheme "Elements - Standalone Plugin" -configuration Debug build 2>&1 | tail -20

# Run standalone
open Builds/MacOSX/build/Debug/Elements.app
```

> **IMPORTANT**: Release builds MUST always include `ARCHS="arm64 x86_64" ONLY_ACTIVE_ARCH=NO` flags to produce Universal Binaries. Projucer defaults to active arch only; these flags override that. Debug builds can use single arch for faster iteration.

## Source Files

| File | Purpose |
|------|---------|
| `Source/PluginProcessor.h/.cpp` | Audio processor, parameters, MIDI handling |
| `Source/PluginEditor.h/.cpp` | Full GUI: 3D viewport, spectrum, oscilloscope, piano, controls |
| `Source/SynthEngine.h/.cpp` | Polyphonic synth engine, additive synthesis, ADSR, filters |
| `Source/Shaders.h` | PBR GLSL shaders (Cook-Torrance vertex + fragment) as constexpr strings |
| `Source/Physics.h/.cpp` | Optical physics: Fresnel equations, spectral calculations, materials data |

## Architecture

### GUI Layout (PluginEditor)
Three-column layout:
- **Left**: Material buttons (8), Geometry selector (Cube/Sphere/Torus), Rotation fields (editable X/Y/Z), 3 light panels (Key/Fill/Rim)
- **Center**: OpenGL 3D Viewport (top), Piano Roll (bottom)
- **Right**: Spectrum display, Oscilloscope, Filter (LP/HP/BP + cutoff + resonance), ADSR envelope, Volume

### 3D Viewport (Viewport3D class)
- Uses `juce::OpenGLRenderer` with legacy fixed-function OpenGL pipeline
- Renders: grid, axes, geometry (cube/sphere/torus), light indicators, rotation gizmo
- Mouse-drag rotation on gizmo rings (X=red, Y=green, Z=blue)
- Accumulated rotation via 4x4 matrix (column-major), not Euler angles
- Dirty-flag optimization: only repaints when state changes

### Materials (10 total)
Diamond, Ruby, Gold, Emerald, Sapphire, Amber, Amethyst, Topaz, Opal, Quartz — each has:
- `wavelengths[]` — nm values (50 samples from ~380-780nm)
- `transmission[]` — 0.0 to 1.0 per wavelength
- `refractiveIndex` — for Fresnel calculations
- PBR properties (metallic, roughness)
- UI hex color
- Gain compensation factor (for volume consistency across materials)

### Lighting (3-point)
- Key, Fill, Rim lights — each toggleable
- Light sources: Sunset, Daylight, LED Cool, Candle, UV, Sodium
- Each has spectral power distribution (50 wavelength samples)
- Light positions defined in world space

### Physics
- **Multi-face spectral calculation** (`calculateSpectrumMultiFace`)
  - Samples normals from all faces of geometry (Cube: 6 faces, Sphere: 32 samples, etc.)
  - Per-face Fresnel reflectance (angle-dependent via rotation)
  - Weighted contribution summation from all visible faces
- **Spectrum calculation pipeline**:
  1. Material transmission interpolation (50 wavelength samples)
  2. Light spectral power distribution (SPD) multiplication
  3. Fresnel factor per face
  4. Beer-Lambert thickness absorption
  5. Normalization by total light intensity
  6. Per-material gain compensation
- **Deformation** (Sphere only): Sinusoidal wavefolding adds harmonic complexity
- Output spectrum drives harmonic amplitudes in the synth via `WavetableGenerator`

### Synth Engine
- **Single-oscillator architecture**: one wavetable oscillator per voice
- Polyphonic (8 voices, `MAX_POLYPHONY`)
- Band-limited wavetables (5 frequency bands to avoid aliasing)
- Harmonic amplitudes from physics spectrum (50 wavelength samples → harmonic mapping)
  - High-resolution mapping with emphasis curve (`amplitude^3.0`)
  - Natural harmonic rolloff formula
  - Soft saturation (tanh) per harmonic
- Biquad filter (LP/HP/BP) with parameter smoothing and filter envelope
  - Filter envelope modulates cutoff (±7 semitones)
- **Dual ADSR modes**:
  - **Classic**: Traditional ADSR amplitude envelope
  - **Physical**: Envelope affects spectral amplitude calculation (rotation-dependent)
- Anti-click system: fade-in, fade-out, retrigger crossfade, voice stealing
- Wavetable crossfade on spectrum changes (~200ms)
- Soft clipper (tanh) on master output
- Audio buffer exposed for oscilloscope display

## Current State (as of Apr 8, 2026)
- Full working prototype with **PBR shader rendering** (Cook-Torrance BRDF)
- All features functional: materials, geometries, 3-point lighting, Fresnel physics, synth, MIDI
- Hybrid rendering pipeline: GLSL shaders for geometry, fixed-function for grid/axes/gizmo/light indicators
- **Audio clicks fixed** — comprehensive anti-click system implemented
- **Rotation X/Y/Z exposed as DAW-automatable VST parameters** (0-360°)
- **Volume is now APVTS parameter** (Apr 5, 2026) — DAW-automatable
- **Soft clipper implemented** — tanh output limiting reduces clipping
- **Single-oscillator architecture** — one wavetable oscillator per voice with physics-driven spectral morphing
- **Dual-oscillator infrastructure in progress** — branch `mix_materials`, points 1-3/6 done (APVTS params, dual WavetableSets, dual spectrum generation)

### PBR Shader Pipeline (implemented)
- `Source/Shaders.h` — vertex + fragment GLSL shaders as `constexpr const char*`
- VBOs for cube (36 verts), sphere (32x32), torus (32x32) created in `newOpenGLContextCreated()`
- Cook-Torrance BRDF: GGX distribution, Schlick-GGX geometry, Schlick Fresnel
- 3 point lights as uniforms with enable/disable
- Per-material PBR properties (metallic + roughness):
  - Diamond(0.0, 0.05), Water(0.0, 0.1), Amber(0.0, 0.3), Ruby(0.05, 0.15)
  - Gold(1.0, 0.2), Emerald(0.05, 0.2), Amethyst(0.05, 0.25), Sapphire(0.05, 0.1)
- Fallback to fixed-function if shader compilation fails
- HDR tonemapping (Reinhard) + gamma correction in fragment shader

### Click Fix Implementation (Feb 2026)
Comprehensive anti-click system in `SynthEngine.cpp`:

1. **Same-note retrigger crossfade** (`noteOn()` lines 576-629)
   - When pressing same key while note is releasing, captures current envelope level
   - Crossfades from old level to new attack over 256 samples (~5.8ms)
   - Voice fields: `retriggering`, `retriggerFadeRemaining`, `retriggerStartLevel`

2. **Graceful voice stealing** (`stealOldestVoice()` lines 1021-1056)
   - Marks oldest voice for fade-out instead of hard kill
   - Voice fields: `stealing`, `stealFadeRemaining`
   - New notes find a different free voice; if none available, note is dropped

3. **Fade-in for new voices** (`processBlock()` lines 492-497)
   - 256-sample fade-in for brand new voices
   - Voice field: `fadeInRemaining`

4. **Filter enable/disable crossfade** (`processBlock()` lines 521-544)
   - Smooth transition when toggling filter ON/OFF
   - Fields: `filterEnabledMix`, `filterEnabledTarget`

5. **Filter reset on silence→sound** (`processBlock()` lines 385-410)
   - Resets biquad filter state when first voice starts after silence

## Known Issues

### Occasional Saturation/Clipping (LOW PRIORITY)
Soft clipper (tanh) implemented, but with very strong spectra and multiple voices, some distortion can still occur. Hard clamp at ±1.0 is final safety.

### Timbre Movement Too Subtle (ACTIVE)
Rotation affects timbre via multi-face Fresnel calculations, but spectral changes lack dramatic movement. Current mitigation: emphasis curve (`^3.0`) exaggerates differences, but more exploration needed.

### Pending Spectrum Race Condition (KNOWN, NO AUDIBLE ISSUE)
`pendingSpectrum[]` array can race between GUI thread (Physics update) and audio thread (wavetable generation). Not causing clicks or artifacts currently, but theoretically unsafe.

## Current APVTS Parameters (DAW-Automatable)

All parameters exposed to DAW automation:

**Filter**:
- `filterCutoff` (20Hz - 20kHz)
- `filterResonance` (0.1 - 10.0)
- `filterType` (0=Off, 1=LowPass, 2=HighPass, 3=BandPass)

**Filter Envelope**:
- `filterAttack`, `filterDecay`, `filterSustain`, `filterRelease`
- `filterEnvAmount` (±7 semitones modulation)

**Amplitude Envelope**:
- `ampAttack`, `ampDecay`, `ampSustain`, `ampRelease`
- `envMode` (0=Classic, 1=Physical)

**Physics**:
- `thickness` (0.1mm - 50mm, Beer-Lambert absorption)
- `rotationX`, `rotationY`, `rotationZ` (0-360°)

**Lighting**:
- `lightIntensityKey`, `lightIntensityFill`, `lightIntensityRim` (0.0 - 2.0)

**Deformation** (Sphere only):
- `deformAmount` (0.0 - 1.0)
- `deformFrequency` (0.1 - 10.0)

**Master**:
- `volume` (0.0 - 1.0) — **Added Apr 5, 2026**

## Stashed Work

**Branch/Stash**: `stash@{0}` contains **Fog/Environment Feature**
- Volumetric fog with ray-marched simplex noise
- Parameters: `envType`, `fogDensity`
- Fullscreen fog shader + on-geometry fog blending
- Time-based noise animation for dynamic fog movement

## Pending Work / Future Features

### Dual-Oscillator Material Mixing (IN PROGRESS — branch `mix_materials`)

**Status**: Points 1-3 of 6 complete (commit `becb016`, Apr 8, 2026)

**Goal**: Add a second independent oscillator to enable richer timbres and advanced sound design through spectral interaction, not just linear mixing.

#### Why Not Simple Mixing?
Testing with two separate Elements instances in Bitwig (each with different material) showed that **mere addition of curves doesn't generate interesting timbres**. We need **interaction between spectra** to create complex, evolving sounds.

#### Implementation Plan — 6 Points

**[DONE] Point 1 — APVTS Parameters**
- `materialA` (0-9) — Material for Oscillator A (maps to legacy `material`)
- `materialB` (0-9) — Material for Oscillator B
- `blendMode` (0-4) — 0=Ring Mod, 1=Spectral Max, 2=AM, 3=XOR, 4=Interleave
- `mixAmount` (0.0-1.0) — Dry/wet (0=only Osc A, 1=full blend)
- `amDepth` (0.0-1.0) — AM modulation depth (mode 2 only)
- `oscBDetune` (±100 cents) — Detune Oscillator B relative to A
- Migration: legacy `material` state → `materialA`, `materialB=0`, `mixAmount=0`

**[DONE] Point 2 — SynthEngine Dual-Osc Infrastructure**
- `spectrumA[50]`, `spectrumB[50]`, `pendingSpectrumA/B` fields
- `currentWavetablesA`, `currentWavetablesB` (`WavetableSet`)
- `crossfadeA`, `crossfadeB` (`CrossfadeState`)
- Fields: `blendMode`, `mixAmount`, `mixAmountSmooth`, `amDepth`, `oscBDetune`
- Per-voice `phaseB` for independent Oscillator B phase
- Methods: `setMaterialA/B()`, `setBlendMode()`, `setMixAmount()`, `setAMDepth()`, `setOscBDetune()`
- Legacy: `setMaterial()` maps to `setMaterialA()`

**[DONE] Point 3 — Physics: Independent Spectrum Calculation + Dual Wavetable Generation**
- `calculateSpectrumForMaterial(int matIdx, spectrum[])` — full physics pipeline per material
- `updateSpectrum()`: calculates `spectrumA` always, `spectrumB` only when `mixAmount > 0`
- `regenerateWavetables()`: crossfadeA setup + generate `currentWavetablesA`; crossfadeB + `currentWavetablesB` when active
- Both spectra share same rotation/lights/thickness (same physical context, independent optical properties)

**[TODO] Point 4 — SynthEngine processBlock: Oscillator B Playback + Blend Modes**
- Replace stale refs: `currentWavetables` → `currentWavetablesA`, `crossfade` → `crossfadeA`
- In `regenerateWavetables()`: compute blended spectrum for spectral modes (0, 1, 3, 4) → `currentWavetablesBlended`
- Per-voice inner loop additions:
  - Oscillator B reads `currentWavetablesBlended` (spectral modes) or `currentWavetablesB` (AM) at detuned frequency (`voice.phaseB`)
  - Advance `phaseB` with `phaseIncrement * detuneRatio`
  - Blend modes at sample level:
    - Modes 0/1/3/4 (spectral): `output = lerp(sampleA, sampleBlended, mixAmountSmooth)`
    - Mode 2 (AM): `output = sampleA * (1.0 + amDepth * sampleB)`
- `mixAmountSmooth`: per-block smoothing to avoid zipper noise on mix changes
- Guard: when `mixAmount < 0.001`, skip all Osc B work (backward compatible, zero CPU overhead)
- Update `crossfadeA/B` progress at end of block (replace old `crossfade` block)

**[TODO] Point 5 — Viewport3D: Visual Material Blending in PBR Shader**
- Pass `materialA`, `materialB`, `mixAmount` as uniforms to fragment shader
- Interpolate PBR visual properties: `color = mix(colorA, colorB, mixAmount)`; same for `metallic`, `roughness`
- Blend mode visual variants (optional): Ring Mod → `colorA * colorB`; Max → `max(colorA, colorB)`
- Physics/audio still uses **separate, pure material properties** — visual blend is cosmetic only

**[TODO] Point 6 — UI: Dual Material Controls**
- Material selector becomes **dual row**: Material A (top) | Material B (bottom), 10 buttons each
- Blend mode selector: 5 buttons (RING / MAX / AM / XOR / INT) with icons
- Mix Amount slider (0-100%) — always visible
- Detune slider ±100 cents — always visible
- AM Depth slider — visible only when blend mode = AM
- Spectrum display: show spectrumA (dim) + spectrumB (dim) + blended result (bright) as overlaid curves

#### Architecture Notes

**Spectral vs Sample-Level Blend Modes**:
- Modes 0 (Ring Mod), 1 (Spectral Max), 3 (XOR), 4 (Interleave): blend happens at **spectrum level** in `regenerateWavetables` → generates `currentWavetablesBlended`; processBlock plays one oscillator per voice from blended wavetable plus optionally a detuned second oscillator for thickness
- Mode 2 (AM): blend happens at **sample level** in processBlock; true two-oscillator playback (A as carrier, B as modulator)

**Why Separate Spectra, Not Mixed Material Properties?**
- IOR mixing is not physically valid (Emerald IOR + Gold IOR averaged ≠ realistic alloy)
- Each material maintains true optical properties through full physics pipeline
- Blend modes create interaction at spectral/audio level — musically flexible
- Enables future: independent rotation per material, per-material lighting

**Backward Compatibility**: `mixAmount = 0.0` → behaves exactly as single-osc version, no CPU overhead

#### Five Blend Modes Reference

| # | Name | Formula | Effect |
|---|------|---------|--------|
| 0 | Ring Mod | `C[h] = A[h] * B[h]` | Metallic, suppresses weak harmonics |
| 1 | Spectral Max | `C[h] = max(A[h], B[h])` | Hybrid envelope, no saturation |
| 2 | AM | `out = A * (1 + depth * B)` | Sidebands, preserves fundamental |
| 3 | XOR | `C[h] = |A[h] - B[h]|` | Spectral holes, difference timbres |
| 4 | Interleave | even→A, odd→B | Hollow/nasal entangled spectrum |

### Task: ADSR Envelope Graph
- [ ] ADSRDisplay component (visual curve of current ADSR)
- [ ] Integrate in right column layout
- [ ] Real-time update from synth envelope parameters

### Task: Enhance Timbre Movement from Rotation
- [ ] Wider spectral variation per rotation degree
- [ ] Make different geometries produce more distinct timbral signatures
- [ ] Non-linear harmonic mapping for more dramatic timbre shifts

### Task: UI Feedback
- [ ] Filter value labels (show "2.5kHz", "Q:1.5" under knobs)

### Completed Tasks

**Task 3: Rotation as VST Parameters** — DONE (Feb 2026)
- [x] Add rotationX/Y/Z to APVTS (`createParameterLayout()`) — 0-360°, step 0.1
- [x] Sync viewport 3D from APVTS in timerCallback (when not dragging)
- [x] Gizmo drag writes to APVTS with beginChangeGesture/endChangeGesture
- [x] processBlock reads APVTS rotation with change detection → synth.setObjectRotation()
- [x] Text fields and resetRotation write to APVTS
- [x] State save/load: rotation now via APVTS, migration for old projects
- [x] Removed manual rotationX/Y/Z fields, setRotationMatrix, setDisplayRotation from Processor

## Key Implementation Details

### Spectrum → Harmonics Pipeline

**Physics Calculation** (`Physics::calculateSpectrumMultiFace`):
1. For each active light source:
   - Sample geometry normals (6-64 points depending on shape)
   - Calculate view-dependent Fresnel reflectance per face
   - Interpolate material transmission at 50 wavelengths
   - Apply Beer-Lambert thickness absorption: `exp(-k * thickness)`
   - Weight by face visibility and sum contributions
2. Normalize by total light intensity (volume consistency)
3. Apply per-material gain compensation
4. Store in `pendingSpectrum[50]` array

**Wavetable Generation** (`WavetableGenerator::generateFromSpectrum`):
1. Map 50 wavelength samples → harmonic amplitudes
   - High-resolution interpolation (blue wavelengths → high harmonics)
2. Apply emphasis curve: `amplitude = spectrum^3.0` (exaggerate differences)
3. Natural harmonic rolloff: `1.0 / (1.0 + (h - 1) * 0.05)`
4. Soft saturation per harmonic: `tanh(amplitude)`
5. Generate band-limited wavetables (5 frequency bands)

**Synthesis** (`SynthEngine::processBlock`):
1. Per-voice wavetable playback with phase interpolation
2. Crossfade old/new wavetables when spectrum changes (~200ms)
3. Apply ADSR envelope (Classic or Physical mode)
4. Global biquad filter with envelope modulation
5. Sum all voices → soft clip (tanh) → hard clamp → output

### Rotation System
**APVTS is the source of truth** for rotation values (rotationX/Y/Z, 0-360°).
- **Gizmo drag**: Uses accumulated matrix multiplication internally for smooth rendering, then writes wrapped Euler angles to APVTS via `setValueNotifyingHost()`
- **DAW automation**: processBlock reads APVTS → `synth.setObjectRotation()`. Viewport syncs from APVTS in `timerCallback()` when not dragging
- **Gesture tracking**: mouseDown/mouseUp call beginChangeGesture/endChangeGesture for Bitwig automation recording
- Gimbal lock is accepted for DAW parameters (standard for 3D VSTs)

### Dirty Flag System
`Viewport3D` only repaints when `needsRepaint = true`. Checked in `timerCallback()` by comparing current vs last: material, geometry, rotation version, light enabled/source states.

### Material Selection Flow
1. User clicks material button → `PluginProcessor::setMaterial(index)`
2. Processor updates physics spectrum → `Physics::calculateSpectrum()`
3. Editor reads material colour → `viewport3D.setMaterialColour()`
4. Synth picks up new harmonic amplitudes on next audio block

### JUCE Module Dependencies
`juce_opengl` is required for the 3D viewport. Module path: `~/JUCE/modules`
