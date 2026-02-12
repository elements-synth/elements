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

    // Material
    void setMaterial(int index) { synth.setMaterial(index); }
    int getMaterial() const { return synth.getMaterial(); }

    // Geometry
    void setGeometry(Geometry geom) { synth.setGeometry(geom); }
    Geometry getGeometry() const { return synth.getGeometry(); }

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

    // Volume
    void setVolume(float vol) { synth.setVolume(vol); }
    float getVolume() const { return synth.getVolume(); }

    // Spectrum (for visualization)
    const std::array<float, NUM_WAVELENGTHS>& getSpectrum() const { return synth.getCurrentSpectrum(); }

    // Oscilloscope (for visualization)
    const std::array<float, 512>& getOscilloscopeBuffer() const { return synth.getOscilloscopeBuffer(); }

    // ==============================================================================
    // AUTOMATABLE PARAMETERS (exposed to DAW / Bitwig modulators)
    // ==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    // The synthesis engine
    ElementsSynth synth;

    // Change-detection cache for rotation APVTS parameters in processBlock
    float lastRotX = 0.0f;
    float lastRotY = 0.0f;
    float lastRotZ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ElementsAudioProcessor)
};
