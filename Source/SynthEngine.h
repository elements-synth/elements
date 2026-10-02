/*
  ==============================================================================
    SynthEngine.h
    Elements - Wavetable synthesis engine with spectral morphing
    Ported from Python prototype
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Physics.h"
#include <array>
#include <vector>
#include <map>
#include <cmath>
#include <atomic>

// ==============================================================================
// CONSTANTS
// ==============================================================================

constexpr int WAVETABLE_SIZE = 2048;

// Band-limited wavetable bands. One table per octave: band 0 serves notes below
// 32 Hz, band i serves [32*2^(i-1), 32*2^i) Hz, and the last band everything
// from 8192 Hz up. Each table holds every harmonic that fits below ~0.45*sr for
// the TOP of its band (capped at MAX_TABLE_HARMONICS), so the oscillator always
// has content up to roughly Nyquist for the filter to act on, whatever the
// material.
constexpr int NUM_WAVETABLE_BANDS = 10;
constexpr float WAVETABLE_BAND0_TOP_HZ = 32.0f;
constexpr int MAX_TABLE_HARMONICS = 512;
constexpr int MAX_POLYPHONY = 8;

// ==============================================================================
// ENUMS
// ==============================================================================

/**
 * Filter types for the synth.
 */
enum class FilterType
{
    Lowpass,
    Highpass,
    Bandpass
};

// ==============================================================================
// BIQUAD FILTER
// ==============================================================================

/**
 * TPT State Variable Filter (SVF).
 *
 * Topology-Preserving Transform SVF based on Vadim Zavalishin's design.
 * Uses integrator states instead of delayed I/O samples, which means:
 * - No transients when input signal changes character (note transitions)
 * - No transients when coefficients change (cutoff modulation)
 * - Inherently stable — integrators are continuous by nature
 *
 * Replaces the original Direct Form I biquad which produced clicks
 * during polyphonic note transitions at low cutoff frequencies.
 */
class BiquadFilter
{
public:
    BiquadFilter() { reset(); }

    void setLowpass(float cutoffHz, float Q, float sampleRate);
    void setHighpass(float cutoffHz, float Q, float sampleRate);
    void setBandpass(float cutoffHz, float Q, float sampleRate);

    float process(float input);
    void reset();

private:
    // SVF coefficients
    float g = 0.0f;    // tan(pi * fc / fs) — tuning
    float k = 2.0f;    // 1/Q — damping
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

    // SVF integrator states (continuous, no transients)
    float ic1eq = 0.0f;
    float ic2eq = 0.0f;

    // Output mode
    enum class Mode { Low, High, Band } mode = Mode::Low;
};

// ==============================================================================
// VOICE STRUCTURE
// ==============================================================================

/**
 * Represents a single playing voice (note).
 *
 * En un sintetizador, cada nota que suena es una "voz".
 * Esta estructura guarda todo el estado necesario para generar el sonido.
 */
struct Voice
{
    static constexpr int FADE_SAMPLES = 256;  // ~5.8ms at 44.1kHz for anti-click fades

    float frequency = 440.0f;       // Frequency in Hz
    float phase = 0.0f;             // Current phase (0.0 - 1.0) - Oscillator A
    float phaseB = 0.0f;            // Oscillator B phase (independent)
    int age = 0;                    // Samples since note started
    bool releasing = false;         // In release phase?
    int releaseAge = 0;             // Samples since release started
    float releaseStartLevel = 1.0f; // Envelope level when release started (prevents clicks)
    float amplitude = 1.0f;         // Global amplitude from spectrum
    float velocity = 1.0f;          // MIDI velocity (0.0 - 1.0)
    bool active = false;            // Is this voice in use?
    int noteId = -1;                // MIDI note number for identification

    // Anti-click fade-in/out
    int fadeInRemaining = 0;        // Samples remaining for anti-click fade-in
    bool stealing = false;          // Voice is being stolen (fade-out in progress)
    int stealFadeRemaining = 0;     // Samples remaining for steal fade-out

    // Retrigger crossfade (smooth transition when same note is pressed again)
    bool retriggering = false;      // In retrigger crossfade?
    int retriggerFadeRemaining = 0; // Samples remaining for retrigger crossfade
    float retriggerStartLevel = 0.0f; // Envelope level when retrigger started

    void reset()
    {
        frequency = 440.0f;
        phase = 0.0f;
        phaseB = 0.0f;
        age = 0;
        releasing = false;
        releaseAge = 0;
        releaseStartLevel = 1.0f;
        amplitude = 1.0f;
        velocity = 1.0f;
        active = false;
        noteId = -1;
        fadeInRemaining = 0;
        stealing = false;
        stealFadeRemaining = 0;
        retriggering = false;
        retriggerFadeRemaining = 0;
        retriggerStartLevel = 0.0f;
    }
};

// ==============================================================================
// ADSR ENVELOPE
// ==============================================================================

/**
 * ADSR envelope parameters.
 *
 * ADSR = Attack, Decay, Sustain, Release
 * Controla cómo evoluciona el volumen de cada nota en el tiempo.
 */
struct ADSREnvelope
{
    float attack = 0.01f;    // Attack time in seconds
    float decay = 0.1f;      // Decay time in seconds
    float sustain = 0.7f;    // Sustain level (0.0 - 1.0)
    float release = 0.2f;    // Release time in seconds
};

// ==============================================================================
// WAVETABLE SET
// ==============================================================================

/**
 * Set of band-limited wavetables, one per octave band (see NUM_WAVETABLE_BANDS).
 * Higher bands carry fewer harmonics so nothing aliases past Nyquist.
 */
struct WavetableSet
{
    std::array<std::array<float, WAVETABLE_SIZE>, NUM_WAVETABLE_BANDS> bands;

    /** Index of the band whose table is band-limited for `freq`. */
    static int bandForFrequency(float freq)
    {
        int band = 0;
        float top = WAVETABLE_BAND0_TOP_HZ;
        while (band < NUM_WAVETABLE_BANDS - 1 && freq >= top)
        {
            ++band;
            top *= 2.0f;
        }
        return band;
    }

    /** Highest frequency (Hz) the given band's table is built to serve. */
    static float bandTopFrequency(int band)
    {
        return WAVETABLE_BAND0_TOP_HZ * static_cast<float>(1 << band);
    }

    const std::array<float, WAVETABLE_SIZE>& getForFrequency(float freq) const
    {
        return bands[static_cast<size_t>(bandForFrequency(freq))];
    }
};

// ==============================================================================
// WAVETABLE GENERATOR
// ==============================================================================

/**
 * Generates band-limited wavetables from spectral data.
 *
 * Convierte el espectro óptico (luz filtrada por material)
 * en forma de onda de audio usando síntesis aditiva.
 */
class WavetableGenerator
{
public:
    WavetableGenerator();

    /**
     * Number of harmonics (1..H) a table for notes up to `topFreq` can hold
     * without aliasing: floor(0.45*sr / topFreq), capped at MAX_TABLE_HARMONICS.
     */
    static int harmonicLimit(float topFreq, float sampleRate);

    /**
     * Generate a single band-limited wavetable for notes up to `topFreq`.
     *
     * Spectrum -> harmonic mapping ("body"): the first min(BODY_HARMONICS, H)
     * harmonics carry the material's spectrum (harmonic 1 = red end, last body
     * harmonic = blue end). Above the body, a material-independent ~1/h tail
     * fills the table up to H, so a filter always has high-frequency content
     * to shape regardless of material or note.
     */
    void generateFromSpectrum(const std::array<float, NUM_WAVELENGTHS>& spectrum,
                              float topFreq,
                              float sampleRate,
                              std::array<float, WAVETABLE_SIZE>& output,
                              const std::array<float, NUM_WAVELENGTHS>* harmonicPhases = nullptr);

    /**
     * Generate a complete set of band-limited wavetables.
     *
     * harmonicPhases (optional): per-wavelength phase offsets in radians,
     * interpolated per harmonic exactly like amplitude. Models interference
     * path differences from surface roughness (deform) — shorter wavelengths
     * (higher harmonics) swing more, so the waveform shape morphs over time.
     */
    void generateBandLimitedSet(const std::array<float, NUM_WAVELENGTHS>& spectrum,
                                float sampleRate,
                                WavetableSet& output,
                                const std::array<float, NUM_WAVELENGTHS>* harmonicPhases = nullptr);

    /** Number of harmonics that carry the material's spectrum (the "body"). */
    static constexpr int BODY_HARMONICS = 20;

    /** Amplitude of the first tail harmonic, before the natural 1/h rolloff. */
    static constexpr float TAIL_LEVEL = 0.03f;

private:
    using Spectrum = std::array<float, WAVETABLE_SIZE>;

    // In-place radix-2 complex FFT. inverse=true uses the opposite twiddle
    // sign and applies no 1/N scaling.
    void fft(Spectrum& re, Spectrum& im, bool inverse) const;

    // Material-shaped, saturated, peak-normalised body for `bodyHarmonics`
    // harmonics, returned as its DFT (re/im).
    void buildBodySpectrum(const std::array<float, NUM_WAVELENGTHS>& spectrum,
                           int bodyHarmonics,
                           const std::array<float, NUM_WAVELENGTHS>* harmonicPhases,
                           Spectrum& bodyRe, Spectrum& bodyIm);

    // Band-limit a body spectrum to H harmonics, add the tail above
    // bodyHarmonics, inverse transform and peak-normalise into `output`.
    void finishTable(const Spectrum& bodyRe, const Spectrum& bodyIm,
                     int bodyHarmonics, int harmonicLimit, Spectrum& output);

    // Scratch buffers (the generator runs on the audio thread: no allocation).
    Spectrum re, im;
    Spectrum sharedBodyRe, sharedBodyIm;
    std::array<float, WAVETABLE_SIZE / 2> twiddleCos, twiddleSin;
    std::array<uint16_t, WAVETABLE_SIZE> bitReverse;
    std::array<float, MAX_TABLE_HARMONICS + 1> tailPhaseSin, tailPhaseCos;
};

// ==============================================================================
// CROSSFADE STATE
// ==============================================================================

/**
 * State for crossfading between wavetable sets.
 *
 * Cuando cambia el espectro, hacemos un crossfade suave
 * entre el wavetable viejo y el nuevo para evitar clicks.
 */
struct CrossfadeState
{
    WavetableSet oldTables;
    float progress = 0.0f;       // 0.0 = old, 1.0 = new
    float increment = 0.0f;      // Progress per sample
    bool active = false;
};

// ==============================================================================
// LIGHT CONFIGURATION
// ==============================================================================

/**
 * Configuration for a single light in the 3-point lighting system.
 */
struct LightConfig
{
    bool enabled = false;
    int sourceIndex = 0;     // Index into getLightSources()
    int positionIndex = 0;   // 0=key, 1=fill, 2=rim
    float intensity = 0.5f;  // 0.0-1.0, default 0.5 (equilibrium — no pitch effect)
};

// ==============================================================================
// ELEMENTS SYNTH ENGINE
// ==============================================================================

/**
 * Main synthesis engine.
 *
 * Esta es la clase principal que JUCE llamará para generar audio.
 * Hereda de juce::AudioSource para integrarse con el sistema de audio de JUCE.
 */
class ElementsSynth
{
public:
    ElementsSynth();
    ~ElementsSynth() = default;

    // --- Audio Processing ---

    /**
     * Prepare the synth for playback.
     * Called by JUCE before audio starts.
     */
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    /**
     * Generate audio samples.
     *
     * Esta es la función principal que genera el sonido.
     * JUCE la llama repetidamente con un buffer para llenar.
     *
     * @param buffer Buffer to fill with audio samples
     * @param numSamples Number of samples to generate
     */
    void processBlock(float* buffer, int numSamples);

    /**
     * Release resources when audio stops.
     */
    void releaseResources();

    // --- Note Control ---

    void noteOn(int noteNumber, float velocity);
    void noteOff(int noteNumber);
    void allNotesOff();

    /** Query which notes are currently active (for piano roll visualization). */
    void getActiveNotes(std::array<bool, 128>& notes) const
    {
        notes.fill(false);
        for (const auto& voice : voices)
        {
            if (voice.active && !voice.releasing && voice.noteId >= 0 && voice.noteId < 128)
                notes[static_cast<size_t>(voice.noteId)] = true;
        }
    }

    // --- Material & Light ---

    // Dual-oscillator material control
    void setMaterialA(int materialIndex);
    void setMaterialB(int materialIndex);
    int getMaterialA() const { return currentMaterialIndexA; }
    int getMaterialB() const { return currentMaterialIndexB; }

    // Blend mode control
    void setBlendMode(int mode);
    void setMixAmount(float amount);
    void setAMDepth(float depth);
    void setOscBDetune(float cents);
    void setOscAMuted(bool muted) { oscAMuted = muted; }
    bool isOscAMuted() const { return oscAMuted; }

    // Legacy single-material support (maps to materialA for backward compatibility)
    void setMaterial(int materialIndex) { setMaterialA(materialIndex); }
    int getMaterial() const { return currentMaterialIndexA; }

    void setGeometry(Geometry geom);
    Geometry getGeometry() const { return currentGeometry; }

    void setLightEnabled(int lightIndex, bool enabled);
    void setLightSource(int lightIndex, int sourceIndex);
    void setLightIntensity(int lightIndex, float intensity);
    bool isLightEnabled(int lightIndex) const { return lightIndex >= 0 && lightIndex < 3 && lights[static_cast<size_t>(lightIndex)].enabled; }
    int getLightSource(int lightIndex) const { return (lightIndex >= 0 && lightIndex < 3) ? lights[static_cast<size_t>(lightIndex)].sourceIndex : 0; }
    float getLightIntensity(int lightIndex) const { return (lightIndex >= 0 && lightIndex < 3) ? lights[static_cast<size_t>(lightIndex)].intensity : 0.5f; }

    // Rotation - use matrix version for gimbal-lock-free rotation
    void setObjectRotation(float x, float y, float z);  // DEPRECATED: has gimbal lock
    void setObjectRotationMatrix(const float* matrix4x4);  // Pass OpenGL column-major 4x4 matrix
    void setObjectRotationMatrix(const RotationMatrix& matrix);  // Pass 3x3 rotation matrix directly

    // --- Filter ---

    void setFilterType(FilterType type);
    void setFilterCutoff(float cutoffHz);
    void setFilterResonance(float Q);
    void setFilterEnabled(bool enabled);

    // --- Envelope ---

    void setAttack(float seconds);
    void setDecay(float seconds);
    void setSustain(float level);
    void setRelease(float seconds);

    // Envelope mode: 0=Classic ADSR, 1=Physical (derived from optics)
    void setEnvelopeMode(int mode);

    // ADSR getters — return active envelope values (Classic or Physical)
    float getAttack() const { return getActiveEnvelope().attack; }
    float getDecay() const { return getActiveEnvelope().decay; }
    float getSustain() const { return getActiveEnvelope().sustain; }
    float getRelease() const { return getActiveEnvelope().release; }

    // --- Filter Envelope ---

    void setFilterAttack(float seconds);
    void setFilterDecay(float seconds);
    void setFilterSustain(float level);
    void setFilterRelease(float seconds);
    void setFilterEnvAmount(float amount);

    float getFilterAttack() const { return filterEnvelope.attack; }
    float getFilterDecay() const { return filterEnvelope.decay; }
    float getFilterSustain() const { return filterEnvelope.sustain; }
    float getFilterRelease() const { return filterEnvelope.release; }
    float getFilterEnvAmount() const { return filterEnvAmount; }

    // --- Wavefolding ---

    void setDeformAmount(float amount);
    void setDeformFrequency(float freq);
    float getDeformAmount() const { return deformAmount; }
    float getDeformFrequency() const { return deformFrequency; }

    void setDeformNoiseType(int type);
    void setDeformRate(float rate)    { deformRate = juce::jlimit(0.0f, 3.0f, rate); }
    int   getDeformNoiseType() const  { return deformNoiseType; }
    float getDeformRate() const       { return deformRate; }

    // --- Thickness (Beer-Lambert) ---

    void setThickness(float t);
    float getThickness() const { return thickness; }

    // --- Transpose (semitones, applied once at noteOn — not physics-derived) ---

    void setTranspose(int semitones) { transposeSemitones = semitones; }
    int getTranspose() const { return transposeSemitones; }

    // --- Volume ---

    void setVolume(float vol) { volume = clamp(vol, 0.0f, 1.0f); }
    float getVolume() const { return volume; }

    // --- Spectrum Access (for visualization) ---

    const std::array<float, NUM_WAVELENGTHS>& getSpectrumA() const { return spectrumA; }
    const std::array<float, NUM_WAVELENGTHS>& getSpectrumB() const { return spectrumB; }

    // --- Wavetable Diagnostics (for regen/race verification, e.g. ElementsTests) ---
    // Cheap fingerprint of currentWavetablesA/B's 256-512Hz band (index 4) — changes iff
    // regenerateWavetables() actually rebuilt the tables with different content.
    float getWavetableChecksumA() const
    {
        float sum = 0.0f;
        for (float v : currentWavetablesA.bands[4]) sum += std::abs(v);
        return sum;
    }
    float getWavetableChecksumB() const
    {
        float sum = 0.0f;
        for (float v : currentWavetablesB.bands[4]) sum += std::abs(v);
        return sum;
    }
    int getRegenCount() const { return regenCount; }

    // --- Oscilloscope Access (for visualization) ---

    const std::array<float, 512>& getOscilloscopeBuffer() const { return oscilloscopeBuffer; }
    int getOscilloscopeWritePos() const { return oscilloscopeWritePos; }
    const std::array<float, 512>& getOscilloscopeBufferB() const { return oscilloscopeBufferB; }
    int getOscilloscopeWritePosB() const { return oscilloscopeWritePosB; }
    int getBlendMode() const { return blendMode; }
    float getMixAmount() const { return mixAmount; }

private:
    // --- Internal Methods ---

    void calculateSpectrumForMaterial(int matIndex, std::array<float, NUM_WAVELENGTHS>& outputSpectrum);
    void updateSpectrum();
    void regenerateWavetables();
    void updatePhysicalEnvelope();
    const ADSREnvelope& getActiveEnvelope() const;

    float generateEnvelopeSample(Voice& voice);
    float generateFilterEnvelopeSample(int numSamples);
    float peekEnvelopeLevel(const Voice& voice) const;
    float readWavetable(float phase, const std::array<float, WAVETABLE_SIZE>& wavetable);

    int findFreeVoice();
    int findVoiceForNote(int noteNumber);
    void stealOldestVoice();  // Marks oldest voice for fade-out (soft steal)

    float midiNoteToFrequency(int noteNumber);

    // --- State ---

    double sampleRate = 44100.0;
    int blockSize = 512;

    // Thickness (Beer-Lambert)
    float thickness = 1.0f;  // 0.1 = thin/bright, 1.0 = reference, 3.0 = thick/dark

    // Transpose (semitones, -24..24). Applied once per noteOn, not per-block —
    // does not touch the physics/spectrum pipeline at all.
    int transposeSemitones = 0;

    // Material and lighting
    int currentMaterialIndexA = 0;          // Diamond by default (Oscillator A)
    int currentMaterialIndexB = 0;          // Diamond by default (Oscillator B)
    Geometry currentGeometry = Geometry::Sphere;
    std::array<LightConfig, 3> lights;
    RotationMatrix objectRotationMatrix;  // Gimbal-lock-free rotation storage

    // updateSpectrum() runs on both the message thread (material/light/geometry
    // setters) and the audio thread (continuous deform-evolution regen in
    // processBlock) — this guards spectrumA/B and pendingSpectrumA/B, which are
    // written by both, from torn/lost concurrent updates.
    juce::SpinLock spectrumLock;

    // Oscillator A: Spectrum and Wavetables
    std::array<float, NUM_WAVELENGTHS> spectrumA;
    std::array<float, NUM_WAVELENGTHS> pendingSpectrumA{};
    WavetableSet currentWavetablesA;
    CrossfadeState crossfadeA;

    // Oscillator B: Spectrum and Wavetables
    std::array<float, NUM_WAVELENGTHS> spectrumB;
    std::array<float, NUM_WAVELENGTHS> pendingSpectrumB{};
    WavetableSet currentWavetablesB;
    CrossfadeState crossfadeB;

    // Blended wavetable for spectral modes (0=Ring, 1=Max, 3=XOR, 4=Interleave)

    // Blend parameters
    int blendMode = 0;                      // 0=Ring Mod, 1=Spectral Max, 2=AM, 3=XOR, 4=Interleaving
    float mixAmount = 0.0f;                 // 0.0-1.0 dry/wet mix (0=only A, 1=full blend)
    float mixAmountSmooth = 0.0f;           // Smoothed version to avoid zipper noise
    float amDepth = 0.5f;                   // AM modulation depth (0.0-1.0)
    float oscBDetune = 0.0f;                // Oscillator B detune in cents (±100)
    bool oscAMuted = false;                 // Mute Oscillator A for monitoring Osc B in isolation

    // Spectral amplitude - overall strength of the spectrum (affected by angle)
    // Used to modulate voice amplitude so rotation affects volume
    float spectralAmplitude = 1.0f;
    float spectralAmplitudeTarget = 1.0f;

    // Wavetable generation and crossfade
    WavetableGenerator wavetableGen;
    float crossfadeDuration = 0.20f;  // 200ms crossfade for smoother transitions

    // Throttling for wavetable regeneration (prevents clicks during fast rotation)
    int samplesSinceLastRegen = 0;
    int regenThrottleSamples = 882;   // ~20ms at 44.1kHz — fast enough for deform animation at high Rate
    bool regenPending = false;
    std::atomic<int> regenCount { 0 };  // bumped once per regenerateWavetables() call — diagnostic only

    // Voices
    std::array<Voice, MAX_POLYPHONY> voices;
    std::array<int, MAX_POLYPHONY> voiceOrder;  // For voice stealing (oldest first)
    int voiceOrderCount = 0;

    // Amplitude Envelope
    ADSREnvelope envelope;

    // Physical envelope mode
    int envelopeMode = 0;  // 0=Classic ADSR, 1=Physical (derived from optics)
    ADSREnvelope physicalEnvelope;

    // Pitch modulation from light intensity
    float pitchOffsetSemitones = 0.0f;
    float pitchOffsetTarget = 0.0f;

    // Wavefolding / Deform
    float deformAmount = 0.0f;
    float deformAmountTarget = 0.0f;
    float deformFrequency = 2.0f;
    float lastSpectrumDeformAmount = 0.0f;
    float deformNoiseTimeOffset = 0.0f;
    int   deformNoiseType = 0;    // 0=Simplex, 1=Alligator, 2=Worley
    float deformRate = 1.0f;      // animation speed multiplier
    float deformRateSmooth = 1.0f;
    bool deformEvolutionRegen = false;  // true when regen triggered by deform time evolution (uses short crossfade)

    // Per-wavelength shimmer state: smoothed noise values that modulate the spectrum at regen time.
    // Low deformFrequency → small filterCoeff → slow evolution → wobbly.
    // High deformFrequency → large filterCoeff → fast evolution → shimmery.
    std::array<float, NUM_WAVELENGTHS> shimmerCurrentAmp{};

    // Layer 3 wavefolder: smoothed fold drive derived from shimmerCurrentAmp mean (coupled noise).
    float foldDriveSmooth = 0.0f;

    // Filter Envelope (global, not per-voice)
    ADSREnvelope filterEnvelope{0.01f, 0.3f, 0.0f, 0.3f};
    float filterEnvValue = 0.0f;
    int filterEnvAge = 0;
    bool filterEnvReleasing = false;
    int filterEnvReleaseAge = 0;
    float filterEnvReleaseStartLevel = 0.0f;
    float filterEnvAmount = 0.0f;

    // Filter (global, applied to mixed output)
    BiquadFilter filter;
    FilterType filterType = FilterType::Lowpass;
    float filterCutoff = 1000.0f;
    float filterCutoffTarget = 1000.0f;
    float filterResonance = 1.0f;
    float filterResonanceTarget = 1.0f;
    bool filterEnabled = true;
    float filterEnabledMix = 1.0f;      // Smooth crossfade for filter bypass (0=bypass, 1=enabled)
    float filterEnabledTarget = 1.0f;   // Target value for smooth transition

    // Master
    float volume = 0.95f;
    bool hasActiveLights = true;  // False when all lights are OFF → silence

    // Oscilloscope buffers (A = main output, B = raw Osc B signal from first active voice)
    std::array<float, 512> oscilloscopeBuffer{};
    int oscilloscopeWritePos = 0;
    std::array<float, 512> oscilloscopeBufferB{};
    int oscilloscopeWritePosB = 0;

    // Track previous active voice count (for filter reset on silence→sound)
    int prevActiveVoiceCount = 0;

    // Smoothed voice scaling (prevents clicks when voice count changes)
    float voiceScaleSmoothed = 1.0f;

    // Smoothing coefficient for filter parameters
    static constexpr float FILTER_SMOOTH = 0.92f;
};
