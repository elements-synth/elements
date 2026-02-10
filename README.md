# Elements — Spectral Wavetable Synthesizer

A VST3/AU/Standalone synthesizer where **light generates sound**. Materials with real optical properties (Fresnel equations, wavelength-dependent transmission) shape harmonic spectra through physics simulation.

## How It Works

Choose a material (Diamond, Ruby, Gold, Emerald...) and a geometry (Cube, Sphere, Torus, Dodecahedron). Three colored lights illuminate the object. The physics of how light passes through the material produces a visible light spectrum that maps directly to audio harmonics:

- **Red wavelengths** (600-780nm) → low harmonics (warmth)
- **Green wavelengths** (500-600nm) → mid harmonics (body)
- **Blue wavelengths** (380-500nm) → high harmonics (brightness)

Rotating the object changes the angle between each face and the lights, producing real-time timbral movement driven by optics — not conventional oscillators.

## Current State

**v0.9 pre-beta** — Core functionality working:

- 8 materials with distinct spectral signatures
- 4 geometries (Cube, Sphere, Torus, Dodecahedron)
- 3-point lighting system (Sunset, Daylight, LED Cool)
- Rotation X/Y/Z exposed as DAW-automatable parameters
- PBR shader rendering (Cook-Torrance BRDF) with environment mapping
- Polyphonic wavetable synthesis (8 voices, band-limited)
- Biquad filter (LP/HP/BP), ADSR envelope, ADSR graph display
- Anti-click system (crossfades, voice stealing, retrigger fades)

## Requirements

- **macOS** 11.0+ (arm64 + x86_64)
- **Xcode** 15+
- **JUCE** 7+ (modules expected at `~/JUCE/modules`)
- **Projucer** (to regenerate Xcode project from `Elements.jucer` if needed)

## Build

```bash
# Build VST3 (installs to ~/Library/Audio/Plug-Ins/VST3/)
xcodebuild -project Builds/MacOSX/Elements.xcodeproj \
  -scheme "Elements - VST3" -configuration Debug build

# Build Standalone
xcodebuild -project Builds/MacOSX/Elements.xcodeproj \
  -scheme "Elements - Standalone Plugin" -configuration Debug build

# Run standalone
open Builds/MacOSX/build/Debug/Elements.app
```

## Project Structure

| File | Purpose |
|------|---------|
| `Source/PluginProcessor.h/.cpp` | Audio processor, APVTS parameters, MIDI |
| `Source/PluginEditor.h/.cpp` | GUI: 3D viewport, spectrum, oscilloscope, piano, controls |
| `Source/SynthEngine.h/.cpp` | Polyphonic synth, additive synthesis, ADSR, filters |
| `Source/Physics.h/.cpp` | Fresnel equations, spectral calculations, materials |
| `Source/Shaders.h` | PBR GLSL shaders (Cook-Torrance) |
| `Elements.jucer` | Projucer project file |

## License

All rights reserved. This is a private project in active development.
