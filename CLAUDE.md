# Elements - Spectral Synthesizer

## Project Location
`/Users/matiasderose/Documents/JUCE_Projects/Elements`

> **NOTE**: There is also an older Python prototype at `/Users/matiasderose/Documents/Elements/` — that is NOT this project. This is the C++/JUCE port.

## Overview
Audio plugin (VST3/AU/Standalone) — a spectral synthesizer where materials (diamond, ruby, gold, etc.) shape the harmonic spectrum of sound through optical physics simulation. Each material has wavelength-dependent transmission properties that filter harmonics. 3 point lights with spectral power distributions interact with the material and geometry via Fresnel angle calculations.

## Tech Stack
- **Framework**: JUCE 7+ (C++17)
- **Build**: Projucer → Xcode (macOS, arm64+x86_64)
- **OpenGL**: juce_opengl module for 3D viewport
- **Target**: macOS 11.0+, formats: Standalone, AU, VST3

## Build Commands
```bash
# Build from Xcode project (Debug)
xcodebuild -project Builds/MacOSX/Elements.xcodeproj -scheme "Elements - Standalone Plugin" -configuration Debug build 2>&1 | tail -20

# Build Release
xcodebuild -project Builds/MacOSX/Elements.xcodeproj -scheme "Elements - Standalone Plugin" -configuration Release build 2>&1 | tail -20

# Run standalone
open Builds/MacOSX/build/Debug/Elements.app
```

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

### Materials (8 total)
Diamond, Water, Amber, Ruby, Gold, Emerald, Amethyst, Sapphire — each has:
- `wavelengths[8]` — nm values from 380-780
- `transmission[8]` — 0.0 to 1.0 per wavelength
- `refractiveIndex` — for Fresnel calculations
- UI hex color

### Lighting (3-point)
- Key, Fill, Rim lights — each toggleable
- Light sources: Sunset, Daylight, LED Cool, Candle, UV, Sodium
- Each has spectral power distribution (50 wavelength samples)
- Light positions defined in world space

### Physics
- Fresnel reflectance calculations (angle-dependent)
- Geometry affects surface normals → changes Fresnel angle per face
- Spectrum = material transmission × light SPD × Fresnel factor
- Output spectrum drives harmonic amplitudes in the synth

### Synth Engine
- Polyphonic (8 voices, `MAX_POLYPHONY`)
- Band-limited wavetables (5 frequency bands to avoid aliasing)
- Harmonic amplitudes from physics spectrum (blue→high harmonics, red→low)
- Biquad filter (LP/HP/BP) with parameter smoothing
- ADSR envelope with proper release-level capture
- Anti-click system: fade-in, fade-out, retrigger crossfade
- Wavetable crossfade on spectrum changes (200ms)
- Audio buffer exposed for oscilloscope display

## Current State (as of Feb 8, 2026)
- Full working prototype with **PBR shader rendering** (Cook-Torrance BRDF)
- All features functional: materials, geometries, 3-point lighting, Fresnel physics, synth, MIDI
- Hybrid rendering pipeline: GLSL shaders for geometry, fixed-function for grid/axes/gizmo/light indicators
- **Audio clicks fixed** — see "Click Fix Implementation" below
- **Rotation X/Y/Z exposed as DAW-automatable VST parameters** (0-360°) — see below

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

### Rotation Artifacts — MOSTLY RESOLVED
Clicks are gone. Remaining issue: occasional **saturation/clipping** when multiple voices align with a strong spectrum. The hard `clamp(-1, 1)` in master output clips instead of soft-limiting.

**Still identified but lower priority (no audible clicks now)**:
1. Mid-crossfade wavetable replacement — `SynthEngine.cpp:979-1011`
2. Per-block spectralAmplitude smoothing — `SynthEngine.cpp:367`
3. GUI/Audio thread race on pendingSpectrum — `SynthEngine.h:414`

**To fix saturation**: Add soft clipper (tanh) or basic limiter before the hard clamp in processBlock master output section.

### Timbre Movement Too Subtle (ACTIVE)
Rotation affects timbre via Fresnel angle calculations, but the spectral changes are not dramatic enough. The sound lacks noticeable "movement" when modulating rotation. Need to explore:
- Wider Fresnel angle mapping → more spectral variation per degree of rotation
- Different harmonic mapping curves (spectrum→harmonics)
- Per-face contribution weighting
- Geometry-specific spectral signatures

## Pending Work

### Task 4: ADSR Envelope Graph (next)
- [ ] ADSRDisplay component (visual curve of current ADSR)
- [ ] Integrate in right column layout
- [ ] Real-time update from synth envelope parameters

### Task 5: Enhance Timbre Movement from Rotation (next)
- [ ] Analyze current Fresnel→spectrum→harmonics pipeline for dynamic range
- [ ] Explore wider spectral variation per rotation degree
- [ ] Make different geometries produce more distinct timbral signatures
- [ ] Consider non-linear harmonic mapping for more dramatic timbre shifts

### Task 2: UI Feedback
- [ ] 2.1: Filter value labels (show "2.5kHz", "Q:1.5" under knobs)

### Task 3: Rotation as VST Parameters — DONE
- [x] Add rotationX/Y/Z to APVTS (`createParameterLayout()`) — 0-360°, step 0.1
- [x] Sync viewport 3D from APVTS in timerCallback (when not dragging)
- [x] Gizmo drag writes to APVTS with beginChangeGesture/endChangeGesture
- [x] processBlock reads APVTS rotation with change detection → synth.setObjectRotation()
- [x] Text fields and resetRotation write to APVTS
- [x] State save/load: rotation now via APVTS, migration for old projects
- [x] Removed manual rotationX/Y/Z fields, setRotationMatrix, setDisplayRotation from Processor

## Key Implementation Details

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
