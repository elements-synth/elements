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

> **DEFAULT**: Always build VST3. Only build Standalone when explicitly requested.

```bash
# DEFAULT — Build VST3 Debug (active arch, fast iteration)
xcodebuild -project Builds/MacOSX/Elements.xcodeproj -scheme "Elements - VST3" -configuration Debug build 2>&1 | tail -20

# Build VST3 Release — Universal Binary (for distribution)
xcodebuild -project Builds/MacOSX/Elements.xcodeproj -scheme "Elements - VST3" -configuration Release ARCHS="arm64 x86_64" ONLY_ACTIVE_ARCH=NO build 2>&1 | tail -20

# Build Standalone Debug — only when explicitly requested
xcodebuild -project Builds/MacOSX/Elements.xcodeproj -scheme "Elements - Standalone Plugin" -configuration Debug build 2>&1 | tail -20

# Build Standalone Release — only when explicitly requested
xcodebuild -project Builds/MacOSX/Elements.xcodeproj -scheme "Elements - Standalone Plugin" -configuration Release ARCHS="arm64 x86_64" ONLY_ACTIVE_ARCH=NO build 2>&1 | tail -20

# Run standalone (only when explicitly requested)
open Builds/MacOSX/build/Debug/Elements.app
```

> **IMPORTANT**: Release builds MUST always include `ARCHS="arm64 x86_64" ONLY_ACTIVE_ARCH=NO` flags to produce Universal Binaries. Projucer defaults to active arch only; these flags override that. Debug builds can use single arch for faster iteration.
> **BITWIG**: After building VST3, recreate the instrument track in Bitwig (don't just rescan) to pick up parameter layout changes.

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
- **Left**: Material buttons (10), Geometry selector (Cube/Sphere/Torus/Dodeca), Rotation fields (editable X/Y/Z), 3 light panels (Key/Fill/Rim)
- **Center**: OpenGL 3D Viewport with accordion overlay (top), Piano Roll (bottom)
- **Right**: Spectrum display, Oscilloscope, Filter (LP/HP/BP + cutoff + resonance), ADSR envelope, Volume

### 3D Viewport Overlay (accordion)
Semi-transparent collapsible header at top of viewport (both groups closed by default):
- `[▶ GEOMETRIES]` (left half): GEO combo, Thickness slider, Deform slider
- `[▶ MATERIALS]` (right half): MAT A combo, MAT B combo, BLEND combo, MIX slider, DETUNE slider, DEPTH slider, MUTE A button
- `hitTest()` override: transparent gaps pass clicks through to OpenGL gizmo/camera
- Bottom-left of viewport: X/Y/Z rotation fields + RESET (vertical stack, above lights bar)
- Bottom-right: "RMB Orbit  Scroll Zoom" hint text
- Bottom bar (full width): Key / Fill / Rim light panels

### 3D Viewport (Viewport3D class)
- Uses `juce::OpenGLRenderer` with legacy fixed-function OpenGL pipeline
- Renders: grid, axes, geometry (cube/sphere/torus), light indicators, rotation gizmo
- Mouse-drag rotation on gizmo rings (X=red, Y=green, Z=blue)
- Accumulated rotation via 4x4 matrix (column-major), not Euler angles
- Dirty-flag optimization: only repaints when state changes

### Materials (10 total)
Diamond, Water, Amber, Ruby, Gold, Emerald, Amethyst, Sapphire, Copper, Obsidian — each has:
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

### Materials
Each `Material` struct now carries:
- `wavelengths[]` / `transmission[]` — 8-point spectral curve (transmission for dielectrics, reflectance for metals)
- `refractiveIndex` (n) — real part of complex IOR
- `extinctionCoeff` (k) — imaginary part; 0 for dielectrics, ~2.83 for Gold/Copper
- `metallicFactor` — 0 = pure dielectric path, 1 = pure metallic path

### Physics — Two-Path Pipeline
`calculateSpectrum()` blends two physically distinct paths based on `metallicFactor`:

**Dielectric path** (metallicFactor = 0):
- Real-valued Fresnel using material's actual n (bug fix: was hardcoded to 1.5 for all materials)
- Strong angle-spectral shaping: blue wavelengths drop much faster with angle than red
- Beer-Lambert thickness: `T^effectiveThickness` where `effectiveThickness = 1 + (thickness-1) * (1 - metallicFactor)`

**Metallic path** (metallicFactor = 1):
- Complex Fresnel: `F0 = ((n-1)²+k²) / ((n+1)²+k²)` → Schlick angle-dependent reflectance
- transmission[] repurposed as spectral reflectance weights
- Very weak angle-spectral shaping (metals barely change colour with angle)
- Beer-Lambert does not apply (effectiveThickness → 1.0 for pure metals)

**Blended** (0 < metallicFactor < 1): lerp between both path outputs. Thickness slider effect fades as metallicFactor increases.

**Multi-face calculation** (`calculateSpectrumMultiFace`):
- Samples normals from all faces of geometry (Cube: 6, Sphere: 32, Torus: ~128, Dodecahedron: 12)
- Per-face Fresnel reflectance (angle-dependent via rotation matrix)
- Weighted contribution summation from all visible faces
- **Deformation** (Sphere only): noise bumps perturb normals, making Fresnel angle vary with rotation
- Output spectrum → `WavetableGenerator` → harmonic amplitudes

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

## Current State (as of Apr 24, 2026)
- Full working prototype with **PBR shader rendering** (Cook-Torrance BRDF)
- All features functional: materials, geometries, 3-point lighting, Fresnel physics, synth, MIDI
- Hybrid rendering pipeline: GLSL shaders for geometry, fixed-function for grid/axes/gizmo/light indicators
- **Audio clicks fixed** — comprehensive anti-click system implemented
- **Rotation X/Y/Z exposed as DAW-automatable VST parameters** (0-360°)
- **Volume is now APVTS parameter** — DAW-automatable
- **Soft clipper implemented** — tanh output limiting reduces clipping
- **Dual-oscillator feature complete** — branch `mix_materials`; true dual-osc, 6 blend modes, preset system, accordion UI
- **Accurate two-path optical physics** — dielectric vs metallic Fresnel pipeline; Beer-Lambert gated by metallicFactor
- **materialA/B removed from APVTS** — saved/loaded as manual XML attributes; processBlock no longer reads/overwrites them
- **Deform volume normalized** — wavefolding now uses `sin(drive*x)/drive`; small-signal gain stays at 1 regardless of deformAmount
- **Dielectric colors vivid at low thickness** — shader Beer-Lambert adds +0.7 visual offset so materials show rich color even at minimum thickness (audio unaffected)
- **Viewport accordion UI** — GEO and MAT controls in collapsible overlay; viewport breathes again
- **Preset combo re-selection fixed** — `setText()` called before `addItem()` so selectedId stays 0; any click on any preset always fires `onChange`

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
- `thickness` (0.1 – 2.0, Beer-Lambert absorption)
- `rotationX`, `rotationY`, `rotationZ` (0-360°)

**Lighting**:
- `lightIntensityKey`, `lightIntensityFill`, `lightIntensityRim` (0.0 – 1.0)

**Deformation** (Sphere only):
- `deformAmount` (0.0 – 1.0)
- `deformFrequency` (0.5 – 10.0)

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

**Status**: Points 1–5 complete (Apr 17, 2026). Point 6 pending.

**Goal**: Two fully independent oscillators, each with its own material/spectrum, interacting through sample-level blend modes.

#### Implementation Plan — 6 Points

**[DONE] Point 1 — APVTS Parameters**
- `materialA` (0-9), `materialB` (0-9), `blendMode` (0-5), `mixAmount` (0.0-1.0)
- `amDepth` (0.0-1.0) — modulation depth for AM and FM modes
- `oscBDetune` (±100 cents) — detune Oscillator B relative to A

**[DONE] Point 2 — SynthEngine Dual-Osc Infrastructure**
- `currentWavetablesA`, `currentWavetablesB` (independent `WavetableSet`s)
- `crossfadeA`, `crossfadeB` (`CrossfadeState`)
- Per-voice `phaseB` for independent Oscillator B phase
- `oscAMuted` flag for monitoring B in isolation

**[DONE] Point 3 — Physics: Independent Spectrum Calculation**
- `calculateSpectrumForMaterial(int matIdx, spectrum[])` — full physics pipeline per material
- `updateSpectrum()`: calculates `spectrumA` always, `spectrumB` only when `mixAmount > 0`
- `regenerateWavetables()`: generates `currentWavetablesA` always, `currentWavetablesB` when active
- Both spectra share same rotation/lights/thickness

**[DONE] Point 4 — True Dual-Oscillator processBlock + Sample-Level Blend Modes**
- Both oscillators always read their own pure wavetable (`currentWavetablesA` / `currentWavetablesB`)
- All blend modes operate at sample level: `lerp(sampleA, blended, mixT)`
- `mixAmountSmooth` smooths the mix parameter per block to avoid zipper noise
- `setBlendMode()` calls `updateSpectrum()` to keep wavetables current
- `setMixAmount()` triggers `updateSpectrum()` when first activating dual-osc (crossing 0.001)
- Oscilloscope B always captures from `currentWavetablesB` (pure B), independent of blend mode

**[DONE] Point 5 — Viewport3D: Visual Material Blending in PBR Shader**
- All PBR properties (metallic, roughness, IOR, transparency, SSS) lerped CPU-side before shader
- Albedo uses alloy model: `alloy = sqrt(colorA * colorB)`, `result = lerp(colorA, alloy, mix)`
  - mix=0 → pure A, mix=1 → alloy colour (neither A nor B)
  - Physically motivated for dielectrics (optical filter product); consistent approximation for metals

**[DONE] Point 6 — UI: Dual Material Controls in Accordion Overlay**
- MATERIALS accordion panel: MAT A combo, MAT B combo, BLEND combo, MIX slider, DETUNE slider, DEPTH slider, MUTE A button
- GEOMETRIES accordion panel: GEO combo, Thickness slider, Deform slider
- Both panels collapsed by default; viewport breathes

#### Current UI (Accordion Overlay — DONE Apr 24, 2026)
Top accordion header bar (always visible):
- `[▶ GEOMETRIES]` left half: GEO combo | Thickness slider | Deform slider
- `[▶ MATERIALS]` right half: MAT A | MAT B | BLEND | MIX | DETUNE | DEPTH | MUTE A
Bottom-left: X/Y/Z + RESET (vertical stack above lights bar)
Bottom: 3 light panels (Key / Fill / Rim) spanning full width

#### Six Blend Modes (all sample-level, true dual-oscillator)

| # | Name | Formula | Character |
|---|------|---------|-----------|
| 0 | Ring Mod | `A * B` | Sidebands, metallic, inharmonic |
| 1 | Max | `abs(A)>abs(B) ? A : B` | Waveshaping, picks dominant signal |
| 2 | AM | `A * (1 + depth * B)` | Classic AM, depth controls modulation |
| 3 | Difference | `\|A-B\|` with sign | Spectral subtraction, hollow timbres |
| 4 | Crossfade | `lerp(A, B, 0.5)` | Simple timbre blend, mix controls ratio |
| 5 | FM | phase of A modulated by B | Rich inharmonics, depth = FM index |

#### Architecture Notes

**True dual-oscillator**: both oscillators always run independently. `currentWavetablesBlended` was removed — there is no pre-baked blend. All interaction happens in the per-sample inner loop.

**Backward Compatibility**: `mixAmount = 0.0` → `dualOscActive = false`, zero CPU overhead, identical to single-osc.

**State save/load pattern**: materialB and blendMode combos call synth setters directly (bypassing APVTS). They are therefore saved/loaded as manual XML attributes in `getStateInformation`/`setStateInformation`, same as materialA and geometry. Do NOT rely on APVTS for these values.

**blendMode APVTS**: Fixed — `AudioParameterChoice` now has all 6 items (0=Ring Mod, 1=Max, 2=AM, 3=Difference, 4=Crossfade, 5=FM). FM is fully automatable.

### Preset Combo — Key Pattern
`setText(name)` in JUCE ComboBox matches items by name and calls `setSelectedId(itemId)`, leaving the combo unable to re-fire for that item. Fix: call `setText(name)` BEFORE `addItem()` in `refreshPresetList()` so no match exists → `selectedId` stays 0 → any subsequent click fires `onChange`.

### State Save/Load — materialA/B/blendMode
These are NOT APVTS parameters. They are saved as manual XML attributes and read back in `setStateInformation`. Do NOT try to expose them via APVTS — it causes stale-value overwrites in `processBlock`.

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
