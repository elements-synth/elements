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

## Current State (as of Apr 7, 2026)
- Full working prototype with **PBR shader rendering** (Cook-Torrance BRDF)
- All features functional: materials, geometries, 3-point lighting, Fresnel physics, synth, MIDI
- Hybrid rendering pipeline: GLSL shaders for geometry, fixed-function for grid/axes/gizmo/light indicators
- **Audio clicks fixed** — comprehensive anti-click system implemented
- **Rotation X/Y/Z exposed as DAW-automatable VST parameters** (0-360°)
- **Volume is now APVTS parameter** (Apr 5, 2026) — DAW-automatable
- **Soft clipper implemented** — tanh output limiting reduces clipping
- **Single-oscillator architecture** — one wavetable oscillator per voice with physics-driven spectral morphing

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

### Dual-Oscillator Material Mixing (PLANNED - NOT IMPLEMENTED)

**Status**: Designed, ready for implementation (Apr 7, 2026)

**Goal**: Add a second independent oscillator to enable richer timbres and advanced sound design through spectral interaction, not just linear mixing.

#### Why Not Simple Mixing?
Testing with two separate Elements instances in Bitwig (each with different material) showed that **mere addition of curves doesn't generate interesting timbres**. We need **interaction between spectra** to create complex, evolving sounds.

#### Architecture Design

**Two Independent Oscillators**:
- Oscillator A: Material A (existing single-osc architecture)
- Oscillator B: Material B (new, parallel architecture)
- Each oscillator has:
  - Independent material selection (10 materials available)
  - Independent wavetable generation from physics spectrum
  - Independent playback with phase interpolation
  - Shared ADSR envelope (both oscs use same envelope)
  - **Detune parameter**: Oscillator B can be detuned relative to A (cents)

**Why Two Wavetables, Not Mix-Then-Generate?**:
- Mixing spectra first, then generating one wavetable would be limiting
- Two independent wavetables allow features like:
  - **Detune** between oscillators (frequency offset)
  - **Phase offset** between oscillators
  - Independent spectral evolution when materials change
  - Per-oscillator filters (future enhancement)

#### Five Blend Modes (Spectral Interaction)

Each mode creates unique timbral characteristics by combining the two spectra in different ways:

**1. RING MODULATION (Multiplication)**
- **Formula**: `output[h] = spectrum_A[h] * spectrum_B[h]`
- **Interaction**: Multiplies harmonic amplitudes point-by-point
- **Effect**: Generates **sidebands** (sum/difference frequencies), metallic/enharmonic timbres
- **Characteristic**: If one spectrum has weak harmonic, that harmonic is suppressed in output
- **Use case**: Metallic bells, inharmonic textures, aggressive timbres

**2. SPECTRAL MAX (Peak Picking)**
- **Formula**: `output[h] = max(spectrum_A[h], spectrum_B[h])`
- **Interaction**: Takes the **stronger harmonic** at each frequency
- **Effect**: Creates composite spectral envelope that dynamically shifts based on which material dominates
- **Characteristic**: No volume accumulation (avoids saturation), hybrid spectrum
- **Use case**: Dynamic hybrid timbres, morphing materials, smooth transitions

**3. AM (Amplitude Modulation - Carrier/Modulator)**
- **Formula**: `output = wavetable_A * (1.0 + depth * wavetable_B)`
- **Interaction**: Oscillator A is carrier, Oscillator B modulates its amplitude
- **Effect**: Generates sidebands like ring mod, but **maintains fundamental** from carrier
- **Characteristic**: More musical than pure ring mod, preserves pitch
- **Additional parameter**: `depth` (0.0 - 1.0) controls modulation intensity
- **Use case**: Rich harmonic movement, tremolo-like effects at low detune, sidebands at higher detune

**4. SPECTRAL XOR (Difference)**
- **Formula**: `output[h] = |spectrum_A[h] - spectrum_B[h]|`
- **Interaction**: Takes **absolute difference** between harmonic amplitudes
- **Effect**: Similar harmonics cancel out, different ones reinforce → creates "opposite" timbres
- **Characteristic**: Can create spectral holes/notches, emphasizes differences
- **Use case**: Experimental timbres, evolving spectral gaps, subtractive-style interaction

**5. HARMONIC INTERLEAVING (Alternating)**
- **Formula**: `output[h] = (h % 2 == 0) ? spectrum_A[h] : spectrum_B[h]`
- **Interaction**: **Even harmonics** from Material A, **odd harmonics** from Material B
- **Effect**: Creates "entangled" spectra impossible with single material
- **Characteristic**: Alternating harmonic sources, can create hollow/nasal timbres
- **Use case**: Unique timbral hybrids, formant-like characteristics, unusual harmonic series

#### New Parameters (APVTS)

**Material & Mixing**:
- `materialA` (0-9, default: current material) — Material for Oscillator A
- `materialB` (0-9, default: 0) — Material for Oscillator B
- `blendMode` (0-4) — Blend mode selector:
  - 0 = Ring Modulation
  - 1 = Spectral Max
  - 2 = AM (Carrier/Modulator)
  - 3 = Spectral XOR
  - 4 = Harmonic Interleaving
- `mixAmount` (0.0 - 1.0, default: 0.0) — Dry/wet mix (0 = only Osc A, 1 = full blend)
- `amDepth` (0.0 - 1.0, default: 0.5) — AM modulation depth (only used in mode 2)

**Oscillator B Tuning**:
- `oscBDetune` (-100 to +100 cents, default: 0) — Detune Oscillator B relative to A
- `oscBPhase` (0.0 - 1.0, default: 0.0) — Phase offset for Oscillator B (future enhancement)

**Enable/Disable**:
- When `mixAmount = 0.0`, only Oscillator A plays (backward compatible, no CPU overhead)
- When `mixAmount > 0.0`, both oscillators are active and blended

#### Implementation Notes

**SynthEngine Changes Required**:
1. Add second wavetable set per voice: `wavetableB[5][WAVETABLE_SIZE]`
2. Add Oscillator B playback in `processBlock()` with detune support
3. Implement blend mode mixing functions (5 variants)
4. Crossfade support for both wavetables when spectra change
5. `mixAmount` parameter for dry/wet control

**Physics Changes Required**:
1. **Calculate two completely independent spectra**:
   - `spectrumA[50]`: Full physics calculation with Material A properties
     - Uses `IOR_A`, `transmission_A[]`, `metallic_A`, `roughness_A`
     - Fresnel calculations with `IOR_A`
   - `spectrumB[50]`: Full physics calculation with Material B properties
     - Uses `IOR_B`, `transmission_B[]`, `metallic_B`, `roughness_B`
     - Fresnel calculations with `IOR_B`
   - **Shared physical context**: Both spectra respond to same rotation/lights/thickness
   - **No mixing of physical properties**: Each material maintains its physical identity
2. Generate two wavetables via `WavetableGenerator::generateFromSpectrum()` (called twice)
3. Blend modes apply to **spectra or wavetables**, NOT to physical properties (IOR, transmission, etc.)

**UI Changes Required**:
1. Material selector becomes **dual selector**: Material A | Material B
2. Blend mode dropdown/buttons (5 modes with visual icons)
3. Mix amount slider (0-100%)
4. Detune slider for Osc B (±100 cents)
5. AM Depth slider (visible only when blend mode = AM)
6. Visual indicator in spectrum display showing both spectra + blend result

**Viewport 3D Visualization**:
- **Single object** rendered (not two separate geometries)
- **Visual material blending** (for rendering only, does NOT affect physics/audio):
  ```glsl
  // PBR shader visual properties (interpolated by mixAmount)
  vec3 color = mix(colorA, colorB, mixAmount);
  float metallic = mix(metallicA, metallicB, mixAmount);
  float roughness = mix(roughnessA, roughnessB, mixAmount);
  ```
- Visual blend represents the "aleación" (alloy) appearance
- Physics calculations still use **separate, pure material properties** for spectrumA and spectrumB
- Alternative: Color could reflect blend mode (e.g., Ring Mod → `colorA * colorB`, Max → `max(colorA, colorB)`)

**Why Separate Spectra, Not Mixed Material Properties?**
- **Physically coherent**: Each material maintains its true optical properties (IOR, transmission)
- **Musically flexible**: Blend modes create interaction at spectral/audio level, not by "averaging" physics
- **Future-proof**: Enables features like independent rotation per material, per-material lighting, etc.
- **IOR mixing is not physically valid**: Emerald IOR + Gold IOR averaged ≠ realistic alloy IOR
- **True independence**: Two oscillators are genuinely separate, enabling proper detune/phase offset

#### Benefits for Sound Design

- **Richer timbres**: Interaction modes create harmonics not present in either material alone
- **Evolving textures**: Rotation affects both materials differently → dynamic spectral morphing
- **Detuning**: Classic analog-style thickness and chorusing
- **Material exploration**: Discover combinations impossible with single material
- **Performance control**: `mixAmount` can be automated for live morphing between timbres
- **Backward compatible**: With `mixAmount = 0`, behaves exactly like current single-osc version

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
