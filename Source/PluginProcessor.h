/*
  ==============================================================================
    PluginProcessor.h
    Elements - Audio Plugin Processor
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SynthEngine.h"

/**
 * Main audio processor for the Elements plugin.
 *
 * Esta clase es el "cerebro" del plugin. JUCE la llama para:
 * - Procesar audio (processBlock)
 * - Recibir MIDI
 * - Guardar/cargar estado
 *
 * Contiene una instancia de ElementsSynth que hace el trabajo real.
 */
class ElementsAudioProcessor : public juce::AudioProcessor
{
public:
    ElementsAudioProcessor();
    ~ElementsAudioProcessor() override;

    // --- Audio Processing (llamados por JUCE) ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    // --- Editor ---
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    // --- Plugin Info ---
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    // --- Programs (presets) ---
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    // --- State (save/load) ---
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ==============================================================================
    // SYNTH ACCESS (para el Editor)
    // ==============================================================================

    /**
     * Get reference to the synth engine.
     * El Editor usa esto para controlar el sintetizador.
     */
    ElementsSynth& getSynth() { return synth; }
    const ElementsSynth& getSynth() const { return synth; }

    // --- Convenience methods (wrappers around synth) ---

    // Material (legacy single-material support, maps to materialA)
    void setMaterial(int index) { synth.setMaterial(index); }
    int getMaterial() const { return synth.getMaterial(); }

    // Dual-oscillator material control
    void setMaterialA(int index) { synth.setMaterialA(index); }
    void setMaterialB(int index) { synth.setMaterialB(index); }
    int getMaterialA() const { return synth.getMaterialA(); }
    int getMaterialB() const { return synth.getMaterialB(); }

    // Geometry
    void setGeometry(Geometry geom) { synth.setGeometry(geom); }
    Geometry getGeometry() const { return synth.getGeometry(); }

    // Thickness — APVTS is the source of truth (automatable from DAW)
    float getThickness() const { return apvts.getRawParameterValue("thickness")->load(); }

    // Rotation — APVTS is the source of truth (automatable from DAW)
    float getRotationX() const { return apvts.getRawParameterValue("rotationX")->load(); }
    float getRotationY() const { return apvts.getRawParameterValue("rotationY")->load(); }
    float getRotationZ() const { return apvts.getRawParameterValue("rotationZ")->load(); }

    // Parameter objects for beginChangeGesture/endChangeGesture from Editor
    juce::RangedAudioParameter* getRotationXParam() { return apvts.getParameter("rotationX"); }
    juce::RangedAudioParameter* getRotationYParam() { return apvts.getParameter("rotationY"); }
    juce::RangedAudioParameter* getRotationZParam() { return apvts.getParameter("rotationZ"); }

    // Lights
    void setLightEnabled(int index, bool enabled) { synth.setLightEnabled(index, enabled); }
    void setLightSource(int index, int sourceIndex) { synth.setLightSource(index, sourceIndex); }
    bool isLightEnabled(int index) const { return synth.isLightEnabled(index); }
    int getLightSource(int index) const { return synth.getLightSource(index); }

    // Filter
    void setFilterType(FilterType type) { synth.setFilterType(type); }
    void setFilterCutoff(float hz) { synth.setFilterCutoff(hz); }
    void setFilterResonance(float q) { synth.setFilterResonance(q); }
    void setFilterEnabled(bool enabled) { synth.setFilterEnabled(enabled); }

    // Amplitude Envelope
    void setAttack(float seconds) { synth.setAttack(seconds); }
    void setDecay(float seconds) { synth.setDecay(seconds); }
    void setSustain(float level) { synth.setSustain(level); }
    void setRelease(float seconds) { synth.setRelease(seconds); }
    float getAttack() const { return synth.getAttack(); }
    float getDecay() const { return synth.getDecay(); }
    float getSustain() const { return synth.getSustain(); }
    float getRelease() const { return synth.getRelease(); }

    // Filter Envelope
    void setFilterAttack(float seconds) { synth.setFilterAttack(seconds); }
    void setFilterDecay(float seconds) { synth.setFilterDecay(seconds); }
    void setFilterSustain(float level) { synth.setFilterSustain(level); }
    void setFilterRelease(float seconds) { synth.setFilterRelease(seconds); }
    void setFilterEnvAmount(float amount) { synth.setFilterEnvAmount(amount); }
    float getFilterAttack() const { return synth.getFilterAttack(); }
    float getFilterDecay() const { return synth.getFilterDecay(); }
    float getFilterSustain() const { return synth.getFilterSustain(); }
    float getFilterRelease() const { return synth.getFilterRelease(); }
    float getFilterEnvAmount() const { return synth.getFilterEnvAmount(); }

    // Volume — APVTS is the source of truth (automatable from DAW)
    float getVolume() const { return apvts.getRawParameterValue("volume")->load(); }

    // Spectrum (for visualization)
    const std::array<float, NUM_WAVELENGTHS>& getSpectrum()  const { return synth.getSpectrumA(); }
    const std::array<float, NUM_WAVELENGTHS>& getSpectrumB() const { return synth.getSpectrumB(); }

    // Oscilloscope (for visualization)
    const std::array<float, 512>& getOscilloscopeBuffer() const { return synth.getOscilloscopeBuffer(); }
    int getOscilloscopeWritePos() const { return synth.getOscilloscopeWritePos(); }
    const std::array<float, 512>& getOscilloscopeBufferB() const { return synth.getOscilloscopeBufferB(); }
    int getOscilloscopeWritePosB() const { return synth.getOscilloscopeWritePosB(); }

    void setBlendModeUI(int mode) { synth.setBlendMode(mode); }
    int getBlendMode() const { return synth.getBlendMode(); }
    float getMixAmount() const { return synth.getMixAmount(); }
    void setOscAMuted(bool muted) { synth.setOscAMuted(muted); }
    bool isOscAMuted() const { return synth.isOscAMuted(); }

    // ==============================================================================
    // AUTOMATABLE PARAMETERS (exposed to DAW / Bitwig modulators)
    // ==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Splash screen: only show once per plugin instance (survives editor destroy/recreate)
    bool splashShown = false;

    // Chorus voice cap — public so the viewport's trail-rendering loop
    // (PluginEditor.cpp) can share the same bound as the DSP, rather than
    // duplicating the number in two places.
    static constexpr int kChorusMaxVoices = 5;

private:
    // The synthesis engine
    ElementsSynth synth;

    // Change-detection cache for rotation APVTS parameters in processBlock
    float lastRotX = 0.0f;
    float lastRotY = 0.0f;
    float lastRotZ = 0.0f;
    float lastThickness = 1.0f;
    float lastLightIntensity[3] = {0.5f, 0.5f, 0.5f};
    int lastEnvMode = 0;
    float lastDeformAmount = 0.0f;
    float lastDeformFrequency = 2.0f;
    float lastVolume = 0.95f;

    // Dual-oscillator blend/mod cache (materialA/B managed directly via synth)
    int lastBlendMode = 0;
    float lastAmDepth = 0.5f;
    float lastOscBDetune = 0.0f;

    // Deform noise cache
    int   lastDeformNoiseType = 1;
    float lastDeformRate = 1.0f;

    // Chorus (shipping in v1.0.0 — see PluginEditor.cpp for the viewport UI).
    // Single shared mono delay line: both channels read it at independently
    // wobbling delay times (different noise phase), which is what produces
    // stereo width from one mono source, rather than needing separate L/R lines.
    static constexpr float kChorusMaxSpreadMs  = 40.0f;
    static constexpr float kChorusModRangeMs   = 6.0f;  // max wobble excursion added on top of base delay

    // The delay line is now always fed live audio, even while Chorus is
    // "off" (chorusActiveSmoothed just crossfades the wet contribution to
    // 0) — mirrors SynthEngine's filterEnabledMix/Target pattern: never
    // hard-branch between two code paths, always compute both, blend via a
    // smoothly-ramped mix. This also means the delay line never goes stale,
    // so re-enabling always reads recent, musically-continuous audio.
    std::vector<float> chorusDelayLine;
    int    chorusWritePos = 0;

    float chorusActiveSmoothed = 0.0f;  // 0=fully bypassed, 1=fully wet — ramps
                                         // toward chorusEnabled's target, same
                                         // role as SynthEngine's filterEnabledMix
    // Per-tap presence (0..1), indexed by tap v-1 — ramps toward 1 if tap v
    // should be active (v <= voices-1) else 0, so changing Voices fades a
    // tap in/out instead of it appearing/disappearing at full gain in one
    // block.
    std::array<float, kChorusMaxVoices - 1> chorusTapPresence{};

    // Smoothed toward their APVTS targets each block (see processChorus) —
    // Spread/Wobble feed directly into the delay-line read position, so an
    // abrupt step (e.g. mid-drag) reads a discontinuous point in the
    // buffer's history and clicks; Decay smoothed too for the same reason
    // applied to tap gain.
    float chorusSpreadSmoothed = 15.0f;
    float chorusWobbleSmoothed = 0.4f;
    float chorusDecaySmoothed  = 0.4f;
    double chorusNoiseTimeL = 0.0;
    double chorusNoiseTimeR = 1000.0;  // arbitrary offset seed so L/R decorrelate

    void processChorus(juce::AudioBuffer<float>& buffer, int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ElementsAudioProcessor)
};
