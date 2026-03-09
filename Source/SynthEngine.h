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

// ==============================================================================
// CONSTANTS
// ==============================================================================

constexpr int WAVETABLE_SIZE = 2048;
constexpr int MAX_POLYPHONY = 8;
constexpr int NUM_FREQUENCY_BANDS = 5;

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

/**
 * Frequency bands for band-limited wavetables.
 * Cada banda tiene un wavetable optimizado para evitar aliasing.
 */
enum class FrequencyBand
{
    Low,        // < 200 Hz - full harmonics
    MidLow,     // 200-400 Hz
    Mid,        // 400-800 Hz
    MidHigh,    // 800-1600 Hz
    High        // > 1600 Hz - minimal harmonics
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
    float phase = 0.0f;             // Current phase (0.0 - 1.0)
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
 * Set of band-limited wavetables.
 *
 * Contiene 5 wavetables, uno para cada rango de frecuencia.
 * Esto evita aliasing (ruido de alta frecuencia no deseado).
 */
struct WavetableSet
{
    std::array<float, WAVETABLE_SIZE> low;
    std::array<float, WAVETABLE_SIZE> midLow;
    std::array<float, WAVETABLE_SIZE> mid;
    std::array<float, WAVETABLE_SIZE> midHigh;
    std::array<float, WAVETABLE_SIZE> high;

    /**
     * Get the appropriate wavetable for a frequency.
     */
    const std::array<float, WAVETABLE_SIZE>& getForFrequency(float freq) const
    {
        if (freq < 200.0f) return low;
        if (freq < 400.0f) return midLow;
        if (freq < 800.0f) return mid;
        if (freq < 1600.0f) return midHigh;
        return high;
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
    WavetableGenerator() = default;

    /**
     * Generate a single wavetable from spectrum.
     *
     * El mapeo espectro → armónicos:
     * - 380-500nm (azul/violeta) → armónicos 8-20 (brillo)
     * - 500-600nm (verde/amarillo) → armónicos 3-8 (cuerpo)
     * - 600-780nm (rojo/naranja) → armónicos 1-3 (calidez)
     */
    void generateFromSpectrum(const std::array<float, NUM_WAVELENGTHS>& spectrum,
                              float fundamentalFreq,
                              float sampleRate,
                              int maxHarmonics,
                              std::array<float, WAVETABLE_SIZE>& output);

    /**
     * Generate a complete set of band-limited wavetables.
     */
    void generateBandLimitedSet(const std::array<float, NUM_WAVELENGTHS>& spectrum,
                                float sampleRate,
                                WavetableSet& output);

private:
    /**
     * Apply soft saturation for analog warmth.
     * tanh(x * drive) / tanh(drive)
     */
    void applySoftSaturation(std::array<float, WAVETABLE_SIZE>& wavetable, float drive = 1.5f);

    /**
     * Normalize wavetable to -1.0 to 1.0 range.
     */
    void normalize(std::array<float, WAVETABLE_SIZE>& wavetable);
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
            if (voice.active && voice.noteId >= 0 && voice.noteId < 128)
                notes[static_cast<size_t>(voice.noteId)] = true;
        }
    }

    // --- Material & Light ---

    void setMaterial(int materialIndex);
    int getMaterial() const { return currentMaterialIndex; }

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
    int getEnvelopeMode() const { return envelopeMode; }

    // ADSR getters — return active envelope values (Classic or Physical)
    float getAttack() const { return getActiveEnvelope().attack; }
    float getDecay() const { return getActiveEnvelope().decay; }
    float getSustain() const { return getActiveEnvelope().sustain; }
    float getRelease() const { return getActiveEnvelope().release; }

    // Pitch offset from light intensity (for UI display)
    float getPitchOffsetSemitones() const { return pitchOffsetSemitones; }

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

    // --- Thickness (Beer-Lambert) ---

    void setThickness(float t);
    float getThickness() const { return thickness; }

    // --- Volume ---

    void setVolume(float vol) { volume = clamp(vol, 0.0f, 1.0f); }
    float getVolume() const { return volume; }

    // --- Spectrum Access (for visualization) ---

    const std::array<float, NUM_WAVELENGTHS>& getCurrentSpectrum() const { return spectrum; }

    // --- Oscilloscope Access (for visualization) ---

    const std::array<float, 512>& getOscilloscopeBuffer() const { return oscilloscopeBuffer; }
    int getOscilloscopeWritePos() const { return oscilloscopeWritePos; }

private:
    // --- Internal Methods ---

    void updateSpectrum();
    void regenerateWavetables();
    void updatePhysicalEnvelope();
    const ADSREnvelope& getActiveEnvelope() const;

    float generateEnvelopeSample(Voice& voice);
    float generateFilterEnvelopeSample(int numSamples);
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

    // Material and lighting
    int currentMaterialIndex = 0;           // Diamond by default
    Geometry currentGeometry = Geometry::Sphere;
    std::array<LightConfig, 3> lights;
    RotationMatrix objectRotationMatrix;  // Gimbal-lock-free rotation storage

    // Current spectrum
    std::array<float, NUM_WAVELENGTHS> spectrum;

    // Spectral amplitude - overall strength of the spectrum (affected by angle)
    // Used to modulate voice amplitude so rotation affects volume
    float spectralAmplitude = 1.0f;
    float spectralAmplitudeTarget = 1.0f;

    // Wavetables
    WavetableGenerator wavetableGen;
    WavetableSet currentWavetables;
    CrossfadeState crossfade;
    float crossfadeDuration = 0.20f;  // 200ms crossfade for smoother transitions

    // Throttling for wavetable regeneration (prevents clicks during fast rotation)
    int samplesSinceLastRegen = 0;
    int regenThrottleSamples = 4410;  // ~100ms at 44.1kHz - minimum time between regenerations
    bool regenPending = false;
    std::array<float, NUM_WAVELENGTHS> pendingSpectrum{};

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

    // Oscilloscope buffer
    std::array<float, 512> oscilloscopeBuffer{};
    int oscilloscopeWritePos = 0;

    // Track previous active voice count (for filter reset on silence→sound)
    int prevActiveVoiceCount = 0;

    // Smoothing coefficient for filter parameters
    static constexpr float FILTER_SMOOTH = 0.92f;
};
