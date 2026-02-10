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
// BIQUAD FILTER
// ==============================================================================

/**
 * Biquad filter with resonance.
 *
 * Un filtro biquad es un filtro digital de segundo orden.
 * "Bi-quad" = dos polos y dos ceros (quad = cuadrático).
 *
 * Usamos coeficientes en formato "Direct Form I":
 * y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]) / a0
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
    // Filter coefficients (normalized, a0 = 1)
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    // Filter state (previous samples)
    float x1 = 0.0f, x2 = 0.0f;  // Previous inputs
    float y1 = 0.0f, y2 = 0.0f;  // Previous outputs
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

    // --- Material & Light ---

    void setMaterial(int materialIndex);
    int getMaterial() const { return currentMaterialIndex; }

    void setGeometry(Geometry geom);
    Geometry getGeometry() const { return currentGeometry; }

    void setLightEnabled(int lightIndex, bool enabled);
    void setLightSource(int lightIndex, int sourceIndex);
    bool isLightEnabled(int lightIndex) const { return lightIndex >= 0 && lightIndex < 3 && lights[static_cast<size_t>(lightIndex)].enabled; }
    int getLightSource(int lightIndex) const { return (lightIndex >= 0 && lightIndex < 3) ? lights[static_cast<size_t>(lightIndex)].sourceIndex : 0; }

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

    // ADSR getters (for UI visualization)
    float getAttack() const { return envelope.attack; }
    float getDecay() const { return envelope.decay; }
    float getSustain() const { return envelope.sustain; }
    float getRelease() const { return envelope.release; }

    // --- Volume ---

    void setVolume(float vol) { volume = clamp(vol, 0.0f, 1.0f); }
    float getVolume() const { return volume; }

    // --- Spectrum Access (for visualization) ---

    const std::array<float, NUM_WAVELENGTHS>& getCurrentSpectrum() const { return spectrum; }

    // --- Oscilloscope Access (for visualization) ---

    const std::array<float, 512>& getOscilloscopeBuffer() const { return oscilloscopeBuffer; }

private:
    // --- Internal Methods ---

    void updateSpectrum();
    void regenerateWavetables();

    float generateEnvelopeSample(Voice& voice);
    float readWavetable(float phase, const std::array<float, WAVETABLE_SIZE>& wavetable);

    int findFreeVoice();
    int findVoiceForNote(int noteNumber);
    void stealOldestVoice();  // Marks oldest voice for fade-out (soft steal)

    float midiNoteToFrequency(int noteNumber);

    // --- State ---

    double sampleRate = 44100.0;
    int blockSize = 512;

    // Material and lighting
    int currentMaterialIndex = 0;           // Diamond by default
    Geometry currentGeometry = Geometry::Cube;
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

    // Envelope
    ADSREnvelope envelope;

    // Filter
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
