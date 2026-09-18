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

## Version

> **RULE**: Never assume or infer the plugin version. Always read it from `Elements.jucer` (the `version="..."` attribute on the `<JUCERPROJECT>` line) before referencing or modifying it anywhere in the code or documentation.
> Current version: **1.0.0** (bumped from 0.9.4 — Sep 2026, user-directed). Splash screen and Help > About both read this dynamically via `JucePlugin_VersionString`, so they update automatically on any future version change — no hardcoded literal to maintain there. **Not yet released**: this is the local/`main` version only; the last actually-published GitHub Release is still `v0.9.4-beta` (see Project Status below) — README/docs "Download" links and badges intentionally were NOT changed to say 1.0.0, since that would misrepresent what's actually downloadable right now.

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
| `Source/TeapotData.h` | Utah Teapot: 28 bicubic Bézier patch control points (Newell/Blinn dataset) |

## Architecture

### GUI Layout (PluginEditor)
Three-column layout:
- **Left**: Material buttons (10), Geometry selector (Cube/Sphere/Torus/Dodeca), Rotation fields (editable X/Y/Z), 3 light panels (Key/Fill/Rim)
- **Center**: OpenGL 3D Viewport with accordion overlay (top), Piano Roll + OCT transpose combo (bottom, see "Transpose" in Preset Bank section)
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
- Renders: grid, axes, geometry (Cube/Sphere/Torus/Dodecahedron/Teapot), light indicators, rotation gizmo
- Mouse-drag rotation on gizmo rings (X=red, Y=green, Z=blue)
- Accumulated rotation via 4x4 matrix (column-major), not Euler angles
- Dirty-flag optimization: only repaints when state changes

### Materials (13 total)
Diamond, Water, Amber, Ruby, Gold, Emerald, Amethyst, Sapphire, Copper, Obsidian, Alexandrite, Malachite, Neodymium — each has:
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
- `wavelengths[]` / `transmission[]` — 32-point spectral curve (transmission for dielectrics, reflectance for metals)
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
- Samples normals from all faces of geometry (Cube: 6, Sphere: 32, Torus: ~128, Dodecahedron: 12, Teapot: 28 patch-center normals)
- Per-face Fresnel reflectance (angle-dependent via rotation matrix)
- Weighted contribution summation from all visible faces
- **Deformation** (Sphere only): noise bumps perturb normals, making Fresnel angle vary with rotation; noise type selectable in UI (Simplex / Alligator / Worley). Rough-surface scattering (Bennett-Porteus) and interference phase offsets also apply — see "Deform / Surface Roughness Feature" section
- Output spectrum → `WavetableGenerator` → harmonic amplitudes

### Synth Engine
- **Dual-oscillator architecture**: two independent wavetable oscillators per voice (A and B), each with its own material/spectrum
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

## Project Status (as of Sep 18, 2026)

**v1.0.0 is code-complete — user-declared wrap ("this is a wrap! v1.0 is ready to be shipped!"), committed to local `main` and pushed to `origin/main`.** Still not made public (see Releases below) — a push to `main` is not a release.

### Repository
- **Local version**: **1.0.0** (bumped from 0.9.4 — Sep 2026, user-directed; `Elements.jucer` updated + `Projucer --resave`, confirmed via built VST3's `Info.plist` `CFBundleShortVersionString`).
- **Remote (`origin/main`)**: synced with local as of this commit — includes the completed 15-preset factory bank + BinaryData packaging, the `transpose` parameter, the gradient viewport background, the Node.js-deprecation CI fix, the deform Rate=0 fix, the light-panel preset-resync fix, the all-lights-off visual fix, the 1.0.0 version bump, BETA badge/label removal, the splash-screen + About feedback message, and the full Help section rewrite (Phase 1 above).
- **Not yet released**: local/remote `main` is now a full version ahead (1.0.0) of the last published release (`v0.9.4-beta`, see below) — per explicit user instruction, none of this is to be cut into a new GitHub Release or made public yet. Pushing to `main` keeps the source current; it does not publish anything downloadable.

### Releases
- **v0.9.4-beta on GitHub Releases** (last published) — macOS Universal (arm64+x86_64) + Windows x64 VST3 uploaded. Predates all of this session's work (presets, packaging, transpose, the 1.0.0 bump) — **the published beta does not have any factory presets bundled**, since packaging was only just finished. Revisit before cutting a new release.
- **Download links in README/docs intentionally NOT updated to 1.0.0** — they describe what's actually downloadable right now, which is still `v0.9.4-beta`. Changing them before an actual 1.0.0 release is cut would be inaccurate. Update them as part of the actual 1.0.0 release process, not before.

### Online Documentation (`docs/` → GitHub Pages)
- `docs/index.md` and `docs/installation.md` updated to v0.9.4
- `docs/science.md` has not been synced with the updated `science.md` in the project root
- **TODO**: sync science.md before next public release

### GitHub Actions
- **Windows VST3 build**: passing — fixed missing PNG assets (`elements_spectra_panel.png`, `swap_arrows.png`) in CMakeLists.txt
- **Node.js deprecation (RESOLVED — Sep 2026)**: `actions/checkout@v4`→`v6`, `actions/cache@v4`→`v5`, `actions/upload-artifact@v4`→`v6` in `build-windows-vst3.yml` and `update-docs.yml`. Committed and pushed to `main`; not yet verified by an actual CI run (no push/PR has triggered the workflow since this landed).

## Release & Outreach Prep (Phase 1 COMPLETE — Sep 2026; Phase 2 still pending)

Four tasks, sequenced by the user into two phases. **Phase 1 (done) — everything that needed a new plugin build**: splash screen text, Help > About text, full Help section audit/update. **Phase 2 (not started) — no plugin build needed**: documentation sweep, GitHub Pages restyle.

### Phase 1a. Splash Screen — DONE
`SplashOverlay` (`PluginEditor.h`): "BETA" badge removed, fade delay extended 2s→4s to give the feedback message room to be read, logo width increased 220→264 (+20%), vertical anchor moved from dead-center to 40% down (`anchorY`) so the block doesn't read as centered. Final feedback message (user-approved):
```
Elements is and will always be free - use it for whatever
you want, personal or commercial.

All I ask in return is some feedback, especially if you
find it useful. Critique, comments, and bug reports are
always welcome.
```
Version string reads dynamically via `JucePlugin_VersionString` (no hardcoded literal).

### Phase 1b. Help > About — DONE
Same message + contact info (`GitHub: github.com/elements-synth/elements`, `Email: elements.synth@gmail.com`) appended to `HelpContent::about()` in `PluginEditor.h`, below a separator line. "(Beta)" label removed from the version line; version is dynamic via `JucePlugin_VersionString`.

### Phase 1c. Help section audit — DONE
Full audit of `HelpContent` (`PluginEditor.h`) found and fixed:
- `geometry()`: added a TEAPOT section (was undocumented — 28 Bezier patches, asymmetric spectrum).
- `viewport()`: removed the THICKNESS entry (it's a GEOMETRIES-accordion material property, not a viewport element per user's call). Accordion UI itself deliberately left undocumented in-app per user ("pretty self explanatory") — candidate for the online docs instead (Phase 2a).
- `controls()`: fixed misleading "Low-pass filter frequency" wording for Cutoff — filter actually has 3 types (Lowpass/Highpass/Bandpass), now just says "Filter frequency". Preset Bank documentation deliberately deferred to the online docs (Phase 2a), not added in-app.
- `materials()`, `science()`, `lights()`, `about()` verified accurate against current code (4 blend modes, 13-material grouping, ±2-semitone pitch mod range, Physical envelope formulas fact-checked against `SynthEngine.cpp`) — no changes needed.

Also fixed 3 related `HelpOverlay` UI mechanics bugs the user found while reviewing:
- Close button ("X") was visually colliding with the last tab label, reading as "<Tab>X" — `tabBarBounds` used to span the full panel width (same width the close button sits within); now stops 12px short of `closeBounds` in `recalcLayout()`.
- Scrollbar was visual-only (`mouseWheelMove` was the only working input) — added `mouseDown`/`mouseDrag`/`mouseUp` handling with click-to-jump-on-track + drag-the-thumb behavior. Scrollbar geometry is now cached as member state (`scrollBarTrackBounds`, `scrollThumbH`, `currentMaxScroll`) inside `drawFormattedContent()` so the mouse handlers can hit-test against exactly what's drawn.
- Main plugin header subtitle ("Spectral Synthesizer" below the logo, `ElementsAudioProcessorEditor::paint()`) now reads "Spectral Synthesizer v1.0.0" — appended dynamically via `JucePlugin_VersionString`, box widened 200→260px to avoid clipping.

Follow-up refinements after the initial audit pass (same session, still Sep 2026):
- **Sapphire/Sunset factual error fixed**: `materials()` claimed Sapphire "produces no sound" under Sunset light. Verified against code and found false — every light source has a non-zero intensity floor at all wavelengths (`Physics.cpp`'s `gaussianIntensity`, base=0.15), and the wavetable is renormalized to a fixed peak after synthesis (`SynthEngine.cpp:212-221`), so it can't go silent — it just gets duller. Text now says "Sounds noticeably duller under Sunset light."
- **About tab**: version line moved from after the two intro paragraphs to directly under "Spectral Wavetable Synthesizer"; hyphen → comma in "Elements is and will always be free, use it for whatever..." (same wording fixed in the splash screen); added a pointer line after the physics bullets: "This Help covers the essentials. For deeper technical detail and advanced techniques, visit the online docs: elements-synth.github.io/elements".
- **Envelope tab removed entirely** (tab label + `HelpContent::controls()`, renamed then deleted) — user judged the Physical-envelope explanation too niche to earn its own tab; that content is deferred to the online docs (Phase 2a) instead, not preserved in-app anywhere.
- **Deformation content relocated twice**: first added to the `controls()`/Envelope tab, then moved into `geometry()` nested under SPHERE (it's sphere-only), then re-styled after user feedback that it read as a 6th geometry type. Added a new `{sub}TEXT` tag to `drawFormattedContent()`'s parser (alongside the existing `{#AARRGGBB}TEXT` color-tag convention) rendering at 12.5pt bold in `ElementsColors::mid` — a visual tier between the 15pt bold colored geometry titles and 13pt plain body text — so "Deformation" reads as a sub-topic of Sphere, not a peer heading.

VST3 + Standalone both rebuilt and verified (`BUILD SUCCEEDED`) after every edit in this pass. `ElementsTests` not re-run since none of this touched `Physics.cpp`'s numerical logic or `SynthEngine.cpp`.

### Phase 2a. Documentation sweep — outdated info + v1.0 physics model changes
Go through all docs and search/replace outdated info: `docs/index.md`, `docs/installation.md`, `docs/known-issues.md`, `docs/materials-and-geometry.md`, `docs/parameters.md`, `docs/science.md`, plus the root `science.md` they're supposed to mirror. `docs/science.md` being out of sync with root `science.md` is already a standing TODO (see Online Documentation above) — this task subsumes and broadens it into a full pass, not just that one file.

Also add information that changed due to the physics model changes reflected in the code as "version 1.0" (the user's words) — most concretely the two-path dielectric-vs-metallic Fresnel pipeline (see "Physics — Two-Path Pipeline" above): dielectrics use real-valued Fresnel + Beer-Lambert thickness, metals use complex Fresnel (from n+ik) with reflectance instead of transmission and no Beer-Lambert. This architecture is already documented in this file but may predate what's in the public docs.

**RESOLVED (Sep 2026)**: confirmed — an actual version bump was intended, not informal shorthand. `Elements.jucer` is now `version="1.0.0"` (was 0.9.4). This was a real bump, not just docs framing, so Phase 2a's public-facing physics-model documentation should describe the current (1.0.0) calculation model as the current one, not a change relative to some other named version.

### Phase 2b. GitHub Pages restyle — match Elements' color palette
Target: https://elements-synth.github.io/elements/index. Source lives in `docs/` (Jekyll site, `docs/_config.yml` currently sets `theme: jekyll-theme-midnight`, no custom CSS override exists yet — pages are plain Markdown relying entirely on the stock theme's styling).

Goal: reskin to match Elements' actual in-plugin color palette, not the stock theme. Concrete source of truth already exists in `Source/ElementsUI.h`:
- Backgrounds: `bg0` `#0d1117`, `bg1` `#111820`, `bg2` `#151e2a`, `bg3` `#1c2534`
- Border/text: `border` `#1f2d3d`, `mid` `#4a6075`, `dim` `#2a3d50`, `text` `#d0e4f0`
- Material accent colors also defined there (`MaterialAccents::*`) if accent/highlight colors are wanted beyond the base dark palette.

Standard Jekyll approach for overriding a stock GitHub Pages theme: add `docs/assets/css/style.scss` that `@import`s the theme's base stylesheet then overrides the relevant Sass variables/selectors — no need to fork the whole theme.

---

## Current State (as of June 11, 2026) — v0.9.4 Beta

> **This section is a historical snapshot from June 2026, not the latest state.** Everything from Aug–Sep 2026 (the complete 15-preset factory bank + BinaryData packaging, the `transpose` parameter, several audio-quality fixes) is documented in the **Preset Bank** and **Known Issues** sections instead of being folded into the bullet list below — check those first for anything preset/packaging/transpose-related.

- Full working prototype with **PBR shader rendering** (Cook-Torrance BRDF)
- All features functional: materials, geometries, 3-point lighting, Fresnel physics, synth, MIDI
- Hybrid rendering pipeline: GLSL shaders for geometry, fixed-function for grid/axes/gizmo/light indicators
- **Audio clicks fixed** — comprehensive anti-click system implemented
- **Rotation X/Y/Z exposed as DAW-automatable VST parameters** (0-360°)
- **Volume is now APVTS parameter** — DAW-automatable
- **Soft clipper implemented** — tanh output limiting reduces clipping
- **Dual-oscillator feature complete** — 4 blend modes (Ring Mod, AM, XOR, FM); Spectral Max and Crossfade removed
- **Material swap button** — swaps MAT A/B with full UI color refresh (accent, oscilloscopes, filter ADSR, piano roll, subtitle)
- **Blended spectrum display** — spectrum widget shows dim A + bright blend result when MIX > 0; blend normalized to prevent visual clipping
- **All 13 materials upgraded to 32 sample points** — simple materials upsampled from 16-pt source data; confirmed audible improvement
- **PBR materials complete** — Alexandrite (chromism), Malachite (banding), Neodymium (Nd³⁺ glass, violet↔red chromism) fully implemented
- **PBR alloy blend model** — all scalar PBR properties (roughness, metallic, IOR, transparency, SSS) use alloy model; mix=1 gives a fused hybrid, not pure B
- **Accurate two-path optical physics** — dielectric vs metallic Fresnel pipeline; Beer-Lambert gated by metallicFactor
- **materialA/B removed from APVTS** — saved/loaded as manual XML attributes; processBlock no longer reads/overwrites them
- **Deform volume normalized** — wavefolding now uses `sin(drive*x)/drive`; small-signal gain stays at 1 regardless of deformAmount
- **Dielectric colors vivid at low thickness** — shader Beer-Lambert adds +0.7 visual offset so materials show rich color even at minimum thickness (audio unaffected)
- **Viewport accordion UI** — GEO and MAT controls in collapsible overlay; viewport breathes again
- **Preset combo re-selection fixed** — `setText()` called before `addItem()` so selectedId stays 0; any click on any preset always fires `onChange`
- **Science tab** — spectral chart PNG embedded in Help overlay with per-material scientific sources; measurement conditions section
- **Materials grouped in dropdowns** — GEMS / MINERALS / METALS / SPECIAL with section headers and separators
- **Utah Teapot geometry** — 5th geometry; 28-patch Bézier tessellation (N=8, 10,752 vertices); 28 precomputed patch-center normals for Fresnel physics; renders correctly in PBR viewport
- **Crossfade regression fix** — rotation now reliably changes timbre in real time; fixed stale-table bug where mid-crossfade regeneration always faded back to the first wavetable snapshot
- **Noise type selector** — UI control to choose Simplex / Alligator / Worley for the Sphere deformer

### PBR Shader Pipeline (implemented)
- `Source/Shaders.h` — vertex + fragment GLSL shaders as `constexpr const char*`
- VBOs for Cube (36 verts), Sphere (32×32), Torus (32×32), Dodecahedron, Teapot (28 patches × 8×8 × 2 tris = 10,752 verts) created in `newOpenGLContextCreated()`
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

### Deform Wobble/Shimmer Too Subtle (RESOLVED — Aug 25, 2026)
`deformFrequency` (low=wobbly, high=shimmery) was previously almost inaudible: the shimmer noise source itself only advanced at <1Hz regardless of `deformFrequency` (it only tuned a tracking-filter cutoff, not the source's own rate), so there was nothing fast to track at high settings. Fixed in `SynthEngine.cpp` `processBlock`:
- `deformFrequency` now scales the shimmer noise's own time-advance rate (0.3x–4x), not just the tracking filter.
- Per-harmonic noise sampling step scales with `deformFrequency` — low freq keeps neighboring harmonics correlated (coherent wobble), high freq decorrelates them (twinkling shimmer).
- Modulation depth (in `regenerateWavetables`) tilts toward high harmonics as `deformFrequency` rises (shimmer reads as treble, wobble as broadband).
- Confirmed audibly improved in Bitwig by the user.

### FM Blend Mode DEPTH Too Subtle (RESOLVED — Aug 25, 2026)
`amDepth` in FM mode (blendMode=3) only reached a max phase-modulation index of 0.25 cycles — inaudible next to AM's up-to-2x amplitude swing at the same depth value. Fixed in `SynthEngine.cpp`: raised the max index to `FM_MOD_INDEX_MAX = 4.0` cycles (constant near top of file). `amDepth=0` still gives `modIndex=0` → dry (identical to `sampleA`, matching AM's depth=0 convention exactly); `amDepth=1` now gives strong DX-style inharmonic FM character. Added anti-aliasing: the FM read now selects a wavetable band pre-band-limited for the boosted effective frequency (`freq * (1 + (modIndex+1) * detuneRatio)`, Carson's-rule approximation) instead of the fundamental's band, since the widened index can otherwise read the table faster than the fundamental and alias past Nyquist.

### AM Blend Mode DEPTH Felt Weak Next to Widened FM (RESOLVED — Aug 25, 2026)
Once FM's depth range was widened (above), AM's textbook 0–100% modulation (`1 + amDepth*sampleB`, envelope range `[0,2]`) felt weak by comparison. Fixed in `SynthEngine.cpp`: `amDepth` now scales up to `AM_MOD_DEPTH_MAX = 3.0` (constant near top of file), envelope range `[-2,4]` before clamping — see below for the clamp actually shipped.

**Shipped version — gated/rectified**: envelope floor clamped at 0 (`std::max(0.0f, 1.0f + amDepth*AM_MOD_DEPTH_MAX*sampleB)`), range `[0,4]`. This rectifies the modulator, producing rhythmic silences/gating synced to B's waveform — a choppier, percussive/stuttery texture. Preferred by ear over the alternative below.

**Alternative tried, kept for reference — unclamped over-modulation**: `blended = sampleA * (1.0f + amDepth * AM_MOD_DEPTH_MAX * sampleB)`, no floor clamp, envelope allowed to swing to `[-2,4]`. Negative envelope values flip A's polarity instead of gating it to silence. Mathematically this is still linear in `amDepth` (no new frequencies vs. standard AM — same sum/difference cross-terms between every harmonic of A and B), but as depth rises past 1 those cross-term sidebands increasingly dominate over the fixed `1*sampleA` term, so the sound morphs smoothly and continuously toward the existing Ring Mod mode's character (pure `A*B`) without ever fully reaching it. No silence/gating, just a smooth AM→ring-mod-like wash. If gating ever feels too abrupt/rhythmic for a given patch, this is the fallback to revisit — swap the clamp line above for this one.

### Light Source Choice Barely Audible, Regardless of Material (RESOLVED — Aug 25, 2026)
Switching a light's SOURCE (Sunset/Daylight/LED Cool) had almost no audible effect on timbre for any material. Root cause verified with a standalone probe (`Physics.cpp` compiles standalone, no JUCE dependency — see scratch probe used during investigation): the pipeline is multiplicative, `output[w] = materialCurve[w] * fresnelCurve[w] * lightIntensity[w]`. The light Gaussians had a high floor (`base=0.3-0.5`, only ~2-3x dynamic range across 380-780nm) versus material/Fresnel curves that vary far more sharply (Ruby's output alone spans ~700x) — in a product, the factor with the largest dynamic range dominates the resulting shape, so the light's tint got almost entirely swamped. Measured shape correlation between different lights' output spectra: 0.75-0.995 (nearly identical) across Diamond/Ruby/Sapphire.

Fixed in `Physics.cpp` `getLightSources()`: lowered `base` / tightened `sigma` on all 3 Gaussians (Sunset: σ80→70, base 0.3→0.15; Daylight: σ120→100, base 0.5→0.20; LED Cool: σ90→75, base 0.4→0.15), raising raw dynamic range from ~2-3x to ~4-6.6x. Re-measured correlation after the change: 0.6-0.86 for broad-spectrum materials (Diamond, Water), some pairs anti-correlated (Sunset↔LED Cool on Diamond: -0.60) — genuinely distinct shapes now. Narrow-band gems (Ruby, Sapphire) still show high correlation (0.9+) between light choices — physically expected, since a material that only transmits in a narrow window can't be reshaped much by light variation outside that window; not fixable by light-curve tuning alone.

A more dramatic tuning (`base=0.03-0.1`, tested and available if the moderate version still feels too subtle) produces stronger, more sign-flipping differentiation but makes each light go nearly dark at wavelengths far from its peak — a bigger change to the instrument's baseline character. User chose the moderate version for 1.0.

### Deform Rate=0 Didn't Actually Freeze the Shimmer (RESOLVED — Sep 2026)
With Deform on and Rate=0, the spectrogram kept moving instead of holding static. Root cause: `SynthEngine.cpp`'s deform-advance line used `std::exp(deformRateSmooth * 1.0f)` as the speed multiplier — `exp(0) = 1`, not `0`, so Rate=0 still advanced the noise clock at a 1x baseline speed. Fixed by gating the whole advancement behind `if (deformRateSmooth > 0.001f)` rather than reshaping the exponential curve, so every already-tuned nonzero-Rate preset (Obsidian Drone at 0.6, Alexandrite Hum at 0.35, etc.) keeps its exact existing feel — only `Rate=0` changed, to true freeze. Pre-existing bug, unrelated to any recent session's changes; the downstream per-wavelength shimmer smoothing naturally settles once the clock stops, so no other code needed touching.

### Dead Code Cleanup (RESOLVED — Sep 2026)
A full audit of every file compiled into the plugin (`PluginProcessor`, `PluginEditor`, `SynthEngine`, `Physics`, `ElementsUI.h`, `Shaders.h`, `TeapotData.h`) found and removed 15 genuinely dead symbols, cross-checked against `ElementsTests` so nothing test-only got deleted: in `Physics.cpp`, the `createLightSources()`/`s_lightSources` duplicate (previously noted here, now actually removed) plus a matching `s_lightPositions` duplicate, the whole deprecated Euler-angle rotation cluster (`Rotation3D`, `applyRotation`, `calculateLightAngle`, `calculateLightAngleForGeometry` — superseded by the matrix-based versions actually in use), and `calculateGeometryFresnel`; in `SynthEngine`, a leftover `debugPhase` static, the unused `FrequencyBand` enum/`NUM_FREQUENCY_BANDS`, and three unwired getters (`getPitchOffsetSemitones`, `getCurrentSpectrum`, `getEnvelopeMode`); in `PluginProcessor.h`, `getThicknessParam()`; in `PluginEditor`, `PianoRoll::getKeyBounds`/`isBlackKey`, `Viewport3D::setGeometry`/`setMaterialColour`, and an unused `materialColours[]` array. Verified via `ElementsTests` (14/14 pass) and clean VST3/Standalone rebuilds after removal.

### Material Dropdown Sometimes Doesn't Register a Click (OPEN — Sep 2026, unconfirmed root cause)
User report: clicking a different material in the MAT A dropdown most of the time doesn't change the material — closing the dropdown and clicking again usually works. Investigated this session; ruled out two candidate causes by reading the code (not by reproducing live):
- The DSP/material-application path (`comboBoxChanged` → `audioProcessor.setMaterial()`) is intact.
- `matCombo`'s item IDs directly encode the true material index (`Diamond=1, Ruby=4, Emerald=6...`, not positional), so unlike the old `presetCombo` bug this can't desync from display order.
- The accordion's custom `hitTest()` (`matCombo` lives inside `accordion.matPanel`, which has manual click-passthrough logic for the gaps around it, to let clicks reach the OpenGL viewport/gizmo) looked correct on inspection but was **not verified live** — this is the most likely remaining suspect, since an off-by-a-few-pixels edge case wouldn't show up just from reading the bounds math.
- Asked the user whether this happens only with MAT A or also MAT B/BLEND/GEO, and whether it correlates with the panel having just been opened — **no answer received**; user moved on to other work before this was resolved. Needs a live repro (with Accessibility/Screen Recording permissions granted, since this session's shell couldn't drive/screenshot the app) to actually confirm the hit-test theory before attempting a fix.
- **Deferred by user (Sep 2026): pinned as a future dev task, likely v1.1.** Not being worked further for now.

### Session-Duration "Everything Sounds the Same" Report (DEFERRED — Sep 2026, likely stale-VST3-binary, not confirmed)
User reported that partway through a long Bitwig session, changing material/settings stopped having any audible effect at all (not just Deform — described as global), until recreating the instrument track fixed it. Investigated the regen/crossfade code (`updateSpectrum`, `regenerateWavetables`, the deform pipeline) and found nothing that looks capable of getting stuck over time — no obvious overflow or stuck-flag path. Leading theory: the VST3 was rebuilt **six times** in the session this was reported in (transpose feature, two OCT-label-size passes, LookAndFeel dot scoping, gradient background), and per the already-known Bitwig VST3 caching behavior, auditioning presets across multiple rebuilds without recreating the track each time would produce exactly this symptom from a stale/mismatched binary — not a real engine bug. **Not proven either way** — no reproduction was captured tying the symptom to a specific build. If this recurs after a single rebuild + track recreation, that would rule out the stale-binary theory and point at something real in the regen pipeline.
- **Deferred by user (Sep 2026): pinned as a future dev task, likely v1.1.** Not being worked further for now.

### Pending Spectrum Race Condition (KNOWN, NO AUDIBLE ISSUE)
`pendingSpectrum[]` array can race between GUI thread (Physics update) and audio thread (wavetable generation). Not causing clicks or artifacts currently, but theoretically unsafe.

## Preset Bank (COMPLETE & SHIPPED — 15/15, Aug–Sep 2026)

### Where presets live
Plain XML `.preset` files (root `<ElementsState>` with manual attributes `material`, `materialB`, `blendMode`, `geometry`, `lightEnabled0/1/2`, `lightSource0/1/2`, a `category` attribute, plus a child `<Parameters>` element with one `<PARAM id="..." value="..."/>` per APVTS parameter including `transpose`), split across two directories under `~/Library/Application Support/Elements/Presets/`:
- **`Presets/Factory/`** — the 15 factory presets. Rewritten from `BinaryData` **every time the editor is constructed**, always overwriting — this folder is never meant to be hand-edited, so there's no versioning concern, it just always mirrors whatever's embedded in the currently-running build.
- **`Presets/` (top level)** — user's own `SAVE`-button presets. Untouched by the factory-sync logic.

### Packaging (DONE — Sep 2026)
Presets are now bundled directly into the plugin binary via JUCE's `BinaryData` mechanism — the same pattern already used in this project for the PNG/font assets. Mechanics:
1. The 15 approved `.preset` files live in `Source/FactoryPresets/` in the repo (real project source, not a build artifact).
2. Each is registered as a `<FILE resource="1">` in `Elements.jucer` (see the `factorypresetsgroup` GROUP).
3. `Projucer --resave Elements.jucer` (headless CLI, no GUI needed — `/Applications/Projucer.app/Contents/MacOS/Projucer --resave Elements.jucer`) regenerates `JuceLibraryCode/BinaryData.h/.cpp` and the Xcode project. **Run this after adding/removing/renaming anything in `Source/FactoryPresets/`.**
4. `ElementsAudioProcessorEditor::writeFactoryPresets()` (`PluginEditor.cpp`, called from the constructor before `refreshPresetList()`) iterates `BinaryData::namedResourceList`/`originalFilenames` generically — filtering for anything ending in `.preset` — and writes each one into `getFactoryPresetsDir()`. This is fully generic: adding a 16th factory preset later needs zero C++ changes, just add the file to `Source/FactoryPresets/`, register it in the `.jucer`, and resave.
5. `refreshPresetList()` globs **both** `getFactoryPresetsDir()` and `getPresetsDir()` (both non-recursive, so no double-counting) before running the existing category-grouping logic — grouping itself is unchanged, since it already keys off the `category` XML attribute regardless of file location.
6. `deletePreset()` refuses outright (returns early) if `isFactoryPreset(currentPresetFile)` is true, and `deletePresetButton` is disabled/re-enabled to match whenever `currentPresetFile` changes (`loadPreset()`, `savePreset()`, `deletePreset()`) — clicking DEL on a factory preset would be a no-op anyway (rewritten on next launch) but silently doing nothing mid-session would be confusing, so it's blocked instead.

A fresh install now has all 15 presets available immediately with no manual setup — verified end-to-end by launching Standalone from a clean `Presets/` directory and confirming `Factory/` populated correctly, byte-identical to the `Source/FactoryPresets/` source files.

### Category grouping in the preset dropdown (DONE)
`refreshPresetList()` groups presets by a fixed `categoryOrder` array (`{"Bass", "Lead", "Pad", "Drone", "Choir"}`), inserting a bold `addSectionHeading()` before each non-empty group; anything with a missing/unrecognized `category` attribute (all of the user's own manual `SAVE`-button presets, which never write that attribute) falls into a trailing "USER PRESETS" heading. A member `presetFilesInDisplayOrder` is filled in the exact order items are added to the combo (headings consume no id) and `onChange` indexes into that array instead of re-scanning/re-sorting the directory — this closes an index-desync hazard that existed before grouping was added. Must preserve the existing "Preset Combo — Key Pattern" (`setText()` before `addItem()`) exactly — section heading calls happen in the same phase as `addItem`, after `setText`.

### Categories (5 total, in dropdown order)
**Bass, Lead, Pad, Drone, Choir** — set in `categoryOrder` in `refreshPresetList()`. History: originally 5 categories with "Secondary Voice" instead of Bass; briefly became 6 with a "Textures" category (Secondary Voice removed, its one preset Ruby Veil deleted); Textures was then removed the same day since its remit overlapped too much with Drone. Net result: 5 categories, Secondary Voice → Bass is the real swap.

### Transpose (DONE — Sep 2026)
New `AudioParameterInt` `transpose` (-24..+24 semitones, default 0), applied once in `ElementsSynth::noteOn()` (`voice.frequency = midiNoteToFrequency(noteNumber + transposeSemitones)`) — not read continuously in `processBlock` like most params, so nudging it never bends a note already sounding, only new notes pick up the change. Deliberately **not** tied to any physics/material parameter — it's a plain register/design choice, same as how a bass guitar's range isn't derived from "physics of a bigger guitar." UI: a 5-item stepped `ComboBox` (`-2 OCT`..`+2 OCT`, exactly 12 semitones apart) at the left edge of the piano-roll strip, wired manually (gesture-based, like the rotation params) rather than via `ComboBoxAttachment` since the UI only exposes octave-quantized steps while the underlying parameter is continuous. The piano roll's own octave number labels (`C2`, `C3`...) are offset by `transpose/12` so they always show the note that will actually sound, not just the raw key position (`PianoRoll::paint()`); `PianoRoll::timerCallback()` tracks `lastKnownTranspose` to repaint immediately when it changes even with no notes playing. All 15 factory presets have an explicit `transpose` value (critical: `apvts.replaceState()` only updates parameters actually present in a loaded preset's XML — a preset missing the `transpose` PARAM would silently inherit whatever the *previously loaded* preset left it at). Currently only Sub Womb uses a non-zero value (`-24`, two octaves down, for a genuinely deep sub character).

### Current bank (15/15 — final, approved)
| File | Category | Material(s) | Geometry | Blend | Notes |
|---|---|---|---|---|---|
| Diamond Lead | Lead | Diamond | Cube | — (single osc) | `thickness=0.2`, `ampRelease=0.04` |
| Gold Spike | Lead | Gold | Sphere | — (single osc) | First metallic Lead. Sphere fixes quietness (Thickness inert on Gold). `ampRelease=0.04` |
| Hollow Lead | Lead | Diamond + Sapphire (B) | Dodecahedron | XOR | Sapphire never used as carrier (narrow-band lesson) |
| Amber Pad | Pad | Amber + Ruby | Sphere | AM, mix=75% | Physical env, depth=0.7, Alligator noise, thickness=0.5, freq=7.5, **transpose=+12** |
| Amethyst Veil | Pad | Amethyst + Water (B) | Teapot | Ring Mod, mix=0.2 | Static (no deform). First use of Teapot geometry |
| Copper Bloom | Pad | Copper + Diamond (B) | Sphere | FM, mix=80% | depth=1.0, thickness=0.25 |
| Obsidian Drone | Drone | Obsidian + Ruby (B) | Sphere | Ring Mod | materialB changed from Copper → Ruby (Copper's ~0.50 gain ceiling made it the quietest preset in the bank — see lesson below). deform=0.9, Worley noise, freq=1.5, rate=0.6, detune=50¢ |
| Alexandrite Hum | Drone | Alexandrite + Diamond (B) | Sphere | XOR, mix=75% | **transpose=-12**. thickness=0.5, deform=0.85, Alligator noise, rate=0.35 |
| Teapot Void | Drone | Water + Amethyst (B) | Teapot | FM, mix=0.5 | Static — movement from FM phase modulation, not geometry. thickness=0.35 |
| Water Choir | Choir | Water + Alexandrite (B) | Sphere | AM, mix=75% | materialB changed from Water → Alexandrite (was the one actual same-material-twice violation in the bank — see lesson below). detune=5¢, depth=0.5, thickness=0.5 |
| Molten Choir | Choir | Ruby + Amber (B) | Sphere | AM, mix=55% | Two different broad-warm materials, not one copied twice. detune=5¢, depth=0.55, thickness=0.7, freq=7 |
| Crystal Choir | Choir | Gold + Diamond (B) | Sphere | Ring Mod, mix=50% | materialB=Diamond guarantees healthy gain regardless of Gold's own weak output (dual-osc gain lesson, applied deliberately this time). Physical env, detune=5¢, thickness=0.5, deform=0.15, freq=8, rate=2.5 |
| Sub Womb | Bass | Amber | Sphere | — (single osc) | **transpose=-24** (2 octaves down). Muted/sub character from hard Lowpass (~180Hz, Q0.5), not low volume. `thickness=0.15`, `ampRelease=0.125` |
| Deep Current | Bass | Ruby | Cube | — (single osc) | **transpose=-12**. Lowpass ~600Hz, Q1.5. `thickness=0.3`, `ampRelease=0.075` |
| Resonant Fang | Bass | Copper | Sphere | — (single osc) | **transpose=-12**. Acid pluck: filterResonance=5.2, fast filter-env snap. Copper's gain caps ~0.50 regardless of tuning — compensated with `volume=1.0`. `ampRelease=0.0375` |

**First revision of Choir was rejected and rebuilt**: the original Amber Choir/Gold Choir used the "same material on both oscillators" mechanism (Water Choir's original design) and were reported as sounding indistinguishable from each other and "boring" — same-material-twice doesn't scale as a differentiation strategy across multiple presets in one category, even though it works fine as a single flagship example. Rebuilt as Molten Choir / Crystal Choir using genuine two-material contrast instead (see lesson below).

### Sound-design lessons learned
- **Narrow-band materials are dangerous as the carrier (materialA or single-osc), fine as materialB.** Emerald ("narrow green peak") as a Pad primary caused a nasal/formant complaint; Sapphire ("steep cutoff, only blue/high harmonics pass") as a solo carrier caused a "no low end" complaint — both traced to `generateFromSpectrum`'s emphasis curve (`pow(specAmp, 3.0)`) exaggerating an already-narrow curve into one dominant resonance. A narrow materialB is much safer (e.g. Sapphire as materialB in Hollow Lead, Ruby as materialB in Amber Pad). Narrow-band materials confirmed: Emerald, Sapphire, Malachite, Neodymium (comb spectrum), Copper (metallic, deep-red-only). Broad-safe-as-carrier materials confirmed: Diamond (flat), Water (flat), Amber (broad rolloff favoring red), Ruby (broad shelf favoring red), Amethyst (weak/nearly flat).
- **Thickness is inert on true metals — confirmed by design, not a bug.** `effectiveThickness = 1.0 + (thickness-1)*(1-metallicFactor)` collapses to exactly `1.0` whenever `metallicFactor=1` (Gold, Copper). Verified numerically with a standalone probe (thickness 0.1/1.0/2.0 all gave bit-identical output). This is intentional: `SynthEngine.cpp` (~1352) and `Physics.cpp` (~1027) both document that Beer-Lambert transmission only applies to the dielectric path — metals are opaque and interact via surface reflectance (Fresnel from complex IOR), not bulk transmission, so "thickness" has no physical quantity to act on. If a future Gold/Copper preset is too quiet, do NOT reach for Thickness — use geometry, filterCutoff, mixAmount, or volume instead (see below).
- **Geometry has a large, measurable effect on perceived loudness — bigger than expected.** Confirmed with the standalone probe (see below): holding material/lights/rotation/thickness fixed, Sphere gave ~2–2.3x the `spectralAmplitudeTarget` of Cube for both a dielectric (Diamond: 0.965 vs 0.487) and a metal (Gold: 0.677 vs 0.288), because Sphere samples far more surface normals so more of them catch light at favorable angles. Cube was consistently the quietest of the 5 geometries in testing. **When a preset needs more presence without changing its character, try Sphere before touching gain/volume/thickness.**
- **Dual-oscillator presets: materialB's spectrum — not materialA's — controls the overall output gain.** `calculateSpectrumForMaterial()` writes to a single shared `spectralAmplitudeTarget` member every call; `updateSpectrum()` calls it for A first, then for B whenever `mixAmount > 0.001`, and B's call unconditionally overwrites A's. So for any dual-osc preset, the real gain-determining spectrum is whichever material is in the **B** slot, regardless of which one is the perceptual "carrier." Confirmed via probe on Hollow Lead: Diamond (A) alone gives 0.585, Sapphire (B) alone gives 0.913 — B's number is what's actually used. **When tuning thickness/lights for a dual-osc preset's loudness, target materialB's spectrum, not materialA's.** This is existing, working engine behavior — not something to "fix" without being asked; design around it.
- **The Harmonic-to-wavelength mapping defines "warm" vs "bright" in this engine.** In `generateFromSpectrum`, harmonic 1 (fundamental) maps to the red end (~780nm) of the material's transmission curve, the highest harmonic maps to blue (~380nm). A material with strong content at the red end and rolloff toward blue reads as warm/full-bodied (Amber, Ruby); the inverse (strong blue, weak red — Sapphire) reads as bright/thin with little to no perceived fundamental/bass, regardless of filter settings.
- **Chorus/ensemble character**: `oscBDetune` (beating between two nearly-identical waveforms) is the mechanism, but the SAME material on both oscillators is not a good template to repeat across multiple presets in one category — tried for Amber Choir/Gold Choir, reported as sounding indistinguishable from each other and "boring." Same-material-twice removes the one thing (real harmonic contrast between A and B) that makes a dual-osc preset read as rich rather than just phase-shifted. Prefer two genuinely different materials (ideally same broad spectral family, e.g. Ruby+Amber for Molten Choir) with a small detune (5-10¢) for the beating, rather than relying on detune alone as the only differentiator. Water Choir (the one working same-material example) is the exception that stays, not the rule to extend.
- **"Mixing the same material twice" is only a real problem when `mixAmount > 0`.** Several single-osc presets have `material == materialB` (Diamond Lead, Gold Spike, Sub Womb, Deep Current, Resonant Fang) — harmless, since `materialB` is an inert unused placeholder whenever `mixAmount = 0` (materialB's spectrum is never blended in). The actual bug class is a dual-osc preset (`mixAmount > 0`) with `material == materialB` — found once, in the original Water Choir (`materialB` was Water, same as `material`), fixed by changing `materialB` to Alexandrite. Audit for this by checking `material != materialB` only among presets with `mixAmount > 0`.
- **Copper has a hard gain ceiling (~0.50) regardless of tuning, and it shows up wherever Copper is used, not just when it's the obvious "carrier."** Confirmed twice: Resonant Fang (Copper solo) and Obsidian Drone (Obsidian + Copper as materialB) were both measured as the quietest presets in the bank, with the exact same root cause — Copper's own `spectralAmpTarget` caps around 0.48-0.50 no matter what lights/rotation/geometry are tried (all already at their best settings), and being metallic it's also thickness-inert, so the two most obvious levers are both unavailable. Since materialB governs gain in dual-osc presets, putting Copper in the B slot silently imports this ceiling into the whole preset even when Copper is meant to be a minor color, not the main event. Fix used for Obsidian Drone: swap materialB away from Copper entirely (→ Ruby, chosen for a "molten obsidian" thematic fit, also fixed the loudness since Ruby responds normally to thickness). If Copper's darker metallic color is specifically wanted, treat its ~0.50 ceiling as a fixed constraint to design around (short/percussive envelopes, resonant filter peaks for perceived punch, `volume=1.0`) rather than something tuning can fix.
- **Bandpass filters double-narrow an already-narrow material** — cuts on both sides of the passband, compounding the material's own narrowness. Prefer Highpass or Lowpass (single-sided cut) on narrow-band materials.
- **Volume has a hard ceiling of 1.0** — once a preset is maxed there, further "make it more powerful" requests must come from thickness reduction, geometry choice (Sphere), filterCutoff increase, or mixAmount increase, not the volume parameter itself.
- **Standalone probe harness**: `Physics.cpp` has zero JUCE dependency and can be compiled standalone (`clang++ -std=c++17 -I Source probe.cpp Source/Physics.cpp`) alongside a small hand-ported copy of `SynthEngine.cpp`'s `calculateSpectrumForMaterial` post-processing math (light sum → thickness → materialGain → clip-prevention normalize → `spectralAmplitudeTarget`). This lets `spectralAmplitudeTarget` be measured exactly for any material/geometry/lights/rotation/thickness combo (and, for dual-osc, both A's and B's numbers) without needing to build/load the plugin or listen by ear — used to diagnose the Gold Spike/Diamond Lead loudness issue and to level-check the two new Pad presets before presenting them. Prefer this over one-at-a-time manual correction rounds by ear when a loudness question is measurable rather than purely aesthetic.
- **Differentiation axes for new presets within a category** (in priority order, so multiple presets in the same category sound genuinely different): 1) material spectral family — broad-warm, broad-neutral, metallic, chromism/bimodal (Alexandrite unused so far); 2) static vs moving (deform only works on Sphere; Teapot geometry now used once, in Amethyst Veil); 3) blend mode — Ring Mod and FM now each used once outside their original preset (Amethyst Veil, Copper Bloom), XOR used once (Hollow Lead); 4) envelope articulation — weakest axis, use last.

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
- `deformRate` — rate of noise animation

**Master**:
- `volume` (0.0 - 1.0) — **Added Apr 5, 2026**
- `transpose` (-24 to +24 semitones, `AudioParameterInt`) — **Added Sep 2026**. Applied once at `noteOn`, not physics-derived. See "Transpose" section above.

## Pending Work / Future Features

### Dual-Oscillator Material Mixing (COMPLETE — merged to main May 2026)

**Status**: All 6 points done and shipped in v0.9.4.

**Goal**: Two fully independent oscillators, each with its own material/spectrum, interacting through sample-level blend modes.

#### Implementation Plan — 6 Points

**[DONE] Point 1 — APVTS Parameters**
- `materialA` (0-12), `materialB` (0-12), `blendMode` (0-3), `mixAmount` (0.0-1.0)
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

#### Four Blend Modes (all sample-level, true dual-oscillator)

| # | Name | Formula | Character |
|---|------|---------|-----------|
| 0 | Ring Mod | `A * B` | Sidebands, metallic, inharmonic |
| 1 | AM | `A * (1 + depth * B)` | Classic AM, depth controls modulation |
| 2 | XOR | `\|A-B\|` with sign | Spectral subtraction, hollow timbres |
| 3 | FM | phase of A modulated by B | Rich inharmonics, depth = FM index (0..4 cycles at depth=1) |

#### Architecture Notes

**True dual-oscillator**: both oscillators always run independently. `currentWavetablesBlended` was removed — there is no pre-baked blend. All interaction happens in the per-sample inner loop.

**Backward Compatibility**: `mixAmount = 0.0` → `dualOscActive = false`, zero CPU overhead, identical to single-osc.

**State save/load pattern**: materialB and blendMode combos call synth setters directly (bypassing APVTS). They are therefore saved/loaded as manual XML attributes in `getStateInformation`/`setStateInformation`, same as materialA and geometry. Do NOT rely on APVTS for these values.

**blendMode APVTS**: `AudioParameterChoice` with 4 items (0=Ring Mod, 1=AM, 2=XOR, 3=FM) — Spectral Max and Crossfade were removed from the original 6-mode design (see "Current State" bullet list above). FM is fully automatable.

### Preset Combo — Key Pattern
`setText(name)` in JUCE ComboBox matches items by name and calls `setSelectedId(itemId)`, leaving the combo unable to re-fire for that item. Fix: call `setText(name)` BEFORE `addItem()` in `refreshPresetList()` so no match exists → `selectedId` stays 0 → any subsequent click fires `onChange`.

### State Save/Load — materialA/B/blendMode
These are NOT APVTS parameters. They are saved as manual XML attributes and read back in `setStateInformation`. Do NOT try to expose them via APVTS — it causes stale-value overwrites in `processBlock`.

### Future Features

**Deform Noise Controls** — all implemented
- `noiseType` — UI selector for Simplex / Alligator / Worley: **implemented**
- `deformFrequency` — exposed as APVTS parameter: **implemented**
- `deformRate` — exposed as APVTS parameter: **implemented**

**Filter B Bypass** — confirmed still accurate (verified against current code, Sep 2026). **Deferred by user: pinned for v1.1, not v1.0.**
Allow Material B to bypass the global filter. Currently there is exactly one filter instance, applied once per sample to the already-mixed A+B signal (`filter.process(sample)`, `SynthEngine.cpp` ~757) — no per-oscillator filter path exists. In FM mode the modulator (B) gets filtered alongside the carrier. Architectural split of the filter path required (~1 day).

**UI Feedback — Filter value labels (DONE — Sep 2026)**
Added `filterCutoffValueLabel`/`filterResonanceValueLabel` (`PluginEditor.h`/`.cpp`) — small read-only labels under the Cutoff/Reso knobs showing `"2.5 kHz"`/`"Q:1.5"`-style live values, updated every `timerCallback()` tick using the same formatting logic as the APVTS parameter's string formatter. Fits inside the existing 55px knob-row height by trading 10px off the rotary slider itself; consistent with the existing `rotXValue`-style live-readout pattern already used for rotation.

**PBR Spectral Accuracy — Alexandrite, Malachite, Neodymium** — confirmed still accurate (verified against current code, Sep 2026). **Deferred by user: pinned for v1.1, not v1.0.**
Current spectral curves are qualitatively correct but estimated. Sourcing USGS/literature data for these three would improve audio accuracy. See Material Accuracy Summary table below — all three still show ⚠️ estimated/qualitative.

## Materials: Scientific Data Policy

### Ground Rules
- **Authoritative source**: USGS Spectral Library (free, downloadable) — covers visible range (0.2-200μm), measured from real mineral samples
- **Never estimate spectral values without asking first** — if data is unavailable, say so and ask the user how to proceed
- **IOR values are reliable** — tabulated in mineralogy literature (e.g. mindat.org, Handbook of Optical Constants)
- **Transmission shapes need USGS or equivalent measurement** — do not invent

### Reflectance → Transmission Conversion (USGS Data)
USGS measures reflectance R of powdered samples, not crystal transmission. To convert:
1. **Kubelka-Munk**: `F(R) = (1-R)² / (2R)` — converts reflectance to absorption coefficient K (relative units)
2. **Beer-Lambert**: `T = exp(-K × thickness)` — converts absorption to transmission for a given crystal thickness
- Works best for opaque/scattering minerals (malachite, obsidian)
- Less precise for highly transparent gems (diamond, water) — those are better measured directly
- When using K-M data, normalize so max(T) ≈ 1.0 before storing in Material struct

### Sample Point Architecture
- **All 13 materials use 32 sample points** (380–780 nm, Δλ ≈ 12.9 nm)
- Simple materials (Diamond, Water, Amber, Gold, Amethyst, Copper, Obsidian) upsampled from 16-pt source data via linear interp; values stored directly in `Physics.cpp`
- **`MATERIAL_MAX_SAMPLES = 32`** — max array size in Material struct
- **`numSamples` field** in Material struct — actual count for this material
- **`interpolateMaterial()`** uses `numSamples` — no hardcoded count

> **Status**: Fully implemented as of May 7, 2026. See `science.md` for all 32-pt tables.

### Material Accuracy Summary (as of May 7, 2026)

| Material | IOR | Spectral Curve | Notes |
|----------|-----|----------------|-------|
| Diamond  | ✅ 2.417 | ✅ flat (near-uniform) | Well-established |
| Water    | ✅ 1.333 | ✅ flat visible, UV absorption | Well-established |
| Gold     | ✅ n+ik measured | ✅ measured data | Complex IOR from Palik |
| Copper   | ✅ n+ik measured | ✅ measured data | Complex IOR from Palik |
| Ruby     | ✅ 1.762 | ⚠️ directionally correct | Cr³⁺ absorption shape qualitative |
| Emerald  | ✅ 1.565 | ⚠️ directionally correct | Cr³⁺ absorption shape qualitative |
| Sapphire | ✅ 1.762 | ⚠️ directionally correct | Fe²⁺/Ti⁴⁺ absorption qualitative |
| Amber    | ✅ 1.539 | ⚠️ directionally correct | Organic polymer, shape approximate |
| Alexandrite | ✅ 1.745 | ⚠️ estimated | Cr³⁺ dual-peak correct in principle |
| Amethyst | ✅ 1.544 | ⚠️ physically weak | Fe³⁺ charge transfer barely visible |
| Obsidian | ✅ 1.49 | ⚠️ physically weak | Volcanic glass, nearly flat |
| Malachite | ✅ 1.85 | ⚠️ estimated | Cu²⁺ absorption qualitative |
| Neodymium | ✅ 1.636 | ⚠️ band positions approximate | Nd³⁺ f-f transitions undersampled at 8pts |

### New Material Workflow
1. **Audio first**: implement spectral data + materialGain entry → test sound before any PBR work
2. **Placeholder shader**: flat diffuse with material color, no reflections (roughness=1.0, metallic=0.0)
3. **Audio approved** → then work on PBR shader properties
4. **materialGain[] array** in SynthEngine.cpp MUST always have exactly `NUM_MATERIALS` entries — out-of-bounds → silence

### Current Materials (13 total, NUM_MATERIALS = 13)
Diamond, Water, Amber, Ruby, Gold, Emerald, Amethyst, Sapphire, Copper, Obsidian, Alexandrite, Malachite, Neodymium

### Neodymium Notes
- Nd³⁺ f-f transitions create 5-8 narrow absorption bands (~10-20nm wide)
- At 8-point sampling (50-80nm spacing) the comb-filter character is completely lost
- Needs 32 sample points to resolve properly
- Band positions approximate until USGS/literature data applied

---

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

---

## Testing Phase (v0.9.4)

### Status

| Step | Description | Status |
|------|-------------|--------|
| 1 | pluginval strictness-10 | ✅ Done — 0 assertions |
| 2 | Manual state round-trip (materialA/B, blendMode, geometry) | ⏭ Skipped |
| 3 | Automated unit test harness (ElementsTests) | ✅ Done — 14/14 pass |
| 4 | Manual regression + coverage matrix | ⏭ Not needed — superseded, see below |

### Step 1 — pluginval
Fixed 3 UTF-8 encoding bugs that caused JUCE `String(const char*)` ASCII assertion failures at strictness 10:
- `HelpContent::materials()` and `HelpContent::science()` in `PluginEditor.h` — wrapped in `juce::String(juce::CharPointer_UTF8(...))`
- Two `DBG()` strings with unicode arrows/dashes — replaced with ASCII equivalents

### Step 4 — Manual regression + coverage matrix (NOT NEEDED — decided by user, Sep 2026)
User decided this dedicated pass isn't needed: the extensive by-ear listening/tuning done while building and revising the 15-preset factory bank (multiple rounds across all 5 categories, all materials, all geometries, all 4 blend modes, both ADSR modes, all 3 deform noise types) already exercised effectively the same coverage this step was meant to provide. Original scope kept below for reference only, in case a future session wants a more formal pass:

- Targeted regression: `mixAmount=0` bleed, voice stealing >8 notes, filter toggle mid-note, same-note retrigger, rapid on/off, zipper noise on automated MIX/DETUNE
- Coverage matrix: 13 materials × 5 geometries × 4 blend modes, filter extremes, ADSR mode, deform noise type, lighting toggles

---

## Automated Test Suite — ElementsTests

Separate JUCE console app project at `/Users/matiasderose/Documents/JUCE_Projects/ElementsTests/`. Compiles `Physics.cpp` and `SynthEngine.cpp` directly from the Elements source tree — no duplication.

### Run tests

```bash
# Build
xcodebuild -project ../ElementsTests/Builds/MacOSX/ElementsTests.xcodeproj \
  -scheme "ElementsTests - ConsoleApp" -configuration Debug \
  build CONFIGURATION_BUILD_DIR=/tmp/ElementsTestsBuild

# Run (exit code 0 = all pass, 1 = failures)
/tmp/ElementsTestsBuild/ElementsTests
```

### Test groups (14 tests total)

| Group | What it checks |
|-------|---------------|
| **Physics** | `calculateSpectrum` for all 13 materials × 3 lights × 3 angles (no NaN/Inf/negative); `interpolateMaterial` range (0..1); Fresnel factor range; noise functions (simplex/alligator/worley) no NaN |
| **SynthEngine** | `BiquadFilter` stability at 5 cutoffs × 4 Q values × LP/HP/BP (impulse response stays bounded); `WavetableGenerator` output in −1..1; `ElementsSynth` produces audio after noteOn |
| **Mix** | `mixAmount=0` — changing MAT B produces zero bleed on output; all 4 blend modes (Ring Mod/AM/XOR/FM) produce valid audio at mix=1 |
| **Regression** | Voice stealing with >8 simultaneous notes (no NaN); rapid note on/off × 20 cycles; filter toggle mid-note (no NaN, no jump > 0.5 per sample); all 13 materials × 5 geometries × 3 blocks each |

### Structure

```
ElementsTests/
├── ElementsTests.jucer          # consoleapp, modules: juce_core/events/audio_basics/data_structures
├── Tests/
│   ├── main.cpp                 # UnitTestRunner, exits 0/1
│   ├── PhysicsTests.cpp
│   ├── SynthEngineTests.cpp
│   ├── MixTests.cpp
│   └── RegressionTests.cpp
```

Source files reference Elements via relative path (`../../Elements/Source/Physics.h` etc.). If the jucer is resaved with Projucer, no extra steps needed — include paths are baked into the xcodeproj.
