/*
  ==============================================================================
    PluginProcessor.cpp
    Elements - Audio Plugin Processor Implementation
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
ElementsAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // =====================================================================
    // 1. SYNTH PARAMETERS
    // =====================================================================

    // Filter Cutoff: 20 Hz – 20 kHz, log skew, default 2000 Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterCutoff", 1},
        "Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f),
        2000.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return (v >= 1000.0f) ? juce::String(v / 1000.0f, 1) + " kHz"
                                                   : juce::String(static_cast<int>(v)) + " Hz"; },
        nullptr));

    // Filter Resonance: 0.5 – 10, default 1.0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterResonance", 1},
        "Resonance",
        juce::NormalisableRange<float>(0.5f, 10.0f, 0.01f, 1.0f),
        1.0f));

    // Filter Type: 0=Lowpass, 1=Highpass, 2=Bandpass
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"filterType", 1},
        "Filter Type",
        juce::StringArray{"Lowpass", "Highpass", "Bandpass"},
        0));

    // Filter Envelope: Attack
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterAttack", 1},
        "Filter Attack",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.4f),
        0.01f));

    // Filter Envelope: Decay
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterDecay", 1},
        "Filter Decay",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.4f),
        0.3f));

    // Filter Envelope: Sustain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterSustain", 1},
        "Filter Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f),
        0.0f));

    // Filter Envelope: Release
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterRelease", 1},
        "Filter Release",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.4f),
        0.3f));

    // Filter Envelope: Amount
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterEnvAmount", 1},
        "Filter Env Amt",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f),
        0.0f));

    // Amp Envelope: Attack
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"ampAttack", 1},
        "Attack",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.4f),
        0.01f));

    // Amp Envelope: Decay
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"ampDecay", 1},
        "Decay",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.4f),
        0.1f));

    // Amp Envelope: Sustain
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"ampSustain", 1},
        "Sustain",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f),
        0.7f));

    // Amp Envelope: Release
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"ampRelease", 1},
        "Release",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.4f),
        0.3f));

    // =====================================================================
    // 2. 3D WORLD PARAMETERS
    // =====================================================================

    // Thickness: 0.1 (thin/bright) – 2.0 (thick/dark), default 0.5
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"thickness", 1},
        "Thickness",
        juce::NormalisableRange<float>(0.1f, 2.0f, 0.01f, 1.0f),
        0.5f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 2); },
        nullptr));

    // Rotation X: 0 – 360 degrees, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"rotationX", 1},
        "Rotation X",
        juce::NormalisableRange<float>(0.0f, 360.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + juce::String::fromUTF8("\xC2\xB0"); },
        nullptr));

    // Rotation Y: 0 – 360 degrees, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"rotationY", 1},
        "Rotation Y",
        juce::NormalisableRange<float>(0.0f, 360.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + juce::String::fromUTF8("\xC2\xB0"); },
        nullptr));

    // Rotation Z: 0 – 360 degrees, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"rotationZ", 1},
        "Rotation Z",
        juce::NormalisableRange<float>(0.0f, 360.0f, 0.1f, 1.0f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + juce::String::fromUTF8("\xC2\xB0"); },
        nullptr));

    // =====================================================================
    // 3. LIGHT INTENSITY PARAMETERS
    // =====================================================================

    // Key Light Intensity: 0.0 – 1.0, default 0.5 (equilibrium)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lightIntensityKey", 1},
        "Key Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f),
        0.5f));

    // Fill Light Intensity: 0.0 – 1.0, default 0.5
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lightIntensityFill", 1},
        "Fill Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f),
        0.5f));

    // Rim Light Intensity: 0.0 – 1.0, default 0.5
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"lightIntensityRim", 1},
        "Rim Intensity",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f),
        0.5f));

    // Envelope Mode: 0=Classic, 1=Physical
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"envMode", 1},
        "Envelope Mode",
        juce::StringArray{"Classic", "Physical"},
        0));

    // Deform / Wavefolding
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"deformAmount", 1},
        "Deform Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"deformFrequency", 1},
        "Deform Frequency",
        juce::NormalisableRange<float>(0.5f, 10.0f, 0.1f),
        2.0f));

    // Noise Type: 0=Simplex, 1=Alligator, 2=Worley (default Simplex)
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"deformNoiseType", 1},
        "Noise Type",
        juce::StringArray{"Simplex", "Alligator", "Worley"},
        0));

    // Deform Rate: animation speed multiplier 0.0–3.0, default 1.0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"deformRate", 1},
        "Deform Rate",
        juce::NormalisableRange<float>(0.0f, 3.0f, 0.01f, 0.7f),
        1.0f));

    // Volume: 0.0 – 1.0, default 0.95
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"volume", 1},
        "Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.95f));

    // =====================================================================
    // 4. DUAL-OSCILLATOR MATERIAL MIXING PARAMETERS
    // =====================================================================

    // materialA and materialB are NOT APVTS parameters — managed as manual XML
    // attributes (same as geometry and blendMode) because DAW automation is not
    // needed and APVTS would cause stale-value conflicts on preset load.

    // Blend Mode: 0=Ring Mod, 1=AM, 2=XOR, 3=FM
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"blendMode", 1},
        "Blend Mode",
        juce::StringArray{"Ring Mod", "AM", "XOR", "FM"},
        0));

    // Mix Amount: 0.0-1.0 (dry/wet), default 0.0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mixAmount", 1},
        "Mix Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f),
        0.0f,
        "%",
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(static_cast<int>(v * 100)) + "%"; },
        nullptr));

    // AM Depth: 0.0-1.0, default 0.5
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"amDepth", 1},
        "AM Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f, 1.0f),
        0.5f));

    // Oscillator B Detune: -100 to +100 cents, default 0
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"oscBDetune", 1},
        "Osc B Detune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        " cents",
        juce::AudioProcessorParameter::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " cents"; },
        nullptr));

    // =====================================================================
    // 5. NOTE / PITCH PARAMETERS
    // =====================================================================

    // Transpose: -24 to +24 semitones, default 0. Applied once at noteOn
    // (not per-block) — a preset's register is a design choice, not a
    // physics-derived quantity, so it stays a plain semitone offset rather
    // than being tied to any material/geometry parameter.
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"transpose", 1},
        "Transpose",
        -24, 24,
        0,
        juce::String(),
        [](int v, int) { return (v == 0) ? juce::String("0 st")
                                          : juce::String(v > 0 ? "+" : "") + juce::String(v) + " st"; }));

    // =====================================================================
    // CHORUS (EXPERIMENTAL — audio-only prototype, see CLAUDE.md
    // "Post-1.0.0 Parking Lot / Chorus (geometry-trail) feature")
    // Not yet wired to any custom UI; test via the DAW's generic parameter
    // list. Represents up to 4 duplicate copies of the object receding
    // along a light's direction: each one arrives later (Spread) and
    // loses energy (Decay, same exponential-falloff form as Beer-Lambert,
    // hop-based rather than distance-based since the engine has no spatial
    // scale). Wobble animates the delay time per tap (via the same noise
    // machinery Deform uses) — that animation, not the static gain/delay
    // alone, is what actually produces a chorus character rather than a
    // fixed comb filter.
    // =====================================================================

    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"chorusVoices", 1},
        "Chorus Voices",
        0, 4,
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"chorusSpread", 1},
        "Chorus Spread",
        juce::NormalisableRange<float>(5.0f, 40.0f, 0.1f),
        15.0f,
        "ms"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"chorusDecay", 1},
        "Chorus Decay",
        juce::NormalisableRange<float>(0.0f, 0.9f, 0.01f),
        0.6f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"chorusWobble", 1},
        "Chorus Wobble",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.4f));

    return layout;
}

ElementsAudioProcessor::ElementsAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

ElementsAudioProcessor::~ElementsAudioProcessor()
{
}

// ==============================================================================
// PLUGIN INFO
// ==============================================================================

const juce::String ElementsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ElementsAudioProcessor::acceptsMidi() const
{
    return true;  // Somos un sintetizador, necesitamos MIDI
}

bool ElementsAudioProcessor::producesMidi() const
{
    return false;
}

bool ElementsAudioProcessor::isMidiEffect() const
{
    return false;
}

double ElementsAudioProcessor::getTailLengthSeconds() const
{
    // Retorna el tiempo de release máximo para que el host no corte el audio
    return 5.0;  // 5 segundos de cola máxima
}

// ==============================================================================
// PROGRAMS (PRESETS)
// ==============================================================================

int ElementsAudioProcessor::getNumPrograms()
{
    return 1;  // Por ahora solo un programa (sin presets)
}

int ElementsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ElementsAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String ElementsAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void ElementsAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

// ==============================================================================
// AUDIO PROCESSING
// ==============================================================================

void ElementsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Preparar el motor de síntesis
    synth.prepareToPlay(sampleRate, samplesPerBlock);

    // Chorus delay line: sized for the worst case (max voices * max spread,
    // plus wobble headroom so the modulated read never runs past what's
    // been written), rounded up with a small safety margin.
    int maxDelaySamples = static_cast<int>(std::ceil(
        (kChorusMaxSpreadMs * kChorusMaxVoices + kChorusModRangeMs) * 0.001 * sampleRate)) + 8;
    chorusDelayLine.assign(static_cast<size_t>(maxDelaySamples), 0.0f);
    chorusWritePos = 0;
}

void ElementsAudioProcessor::releaseResources()
{
    synth.releaseResources();
}

bool ElementsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Soportamos mono y stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

/**
 * processBlock - El corazón del plugin
 *
 * Esta función es llamada por el host (DAW) repetidamente para generar audio.
 * Típicamente se llama 44100/512 ≈ 86 veces por segundo.
 *
 * @param buffer      Buffer de audio a llenar con samples
 * @param midiMessages  Mensajes MIDI recibidos durante este bloque
 */
void ElementsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    /**
     * ScopedNoDenormals evita números "denormales" que causan
     * alto uso de CPU en algunos procesadores.
     *
     * Números denormales son valores muy pequeños (cercanos a cero)
     * que el CPU procesa muy lentamente.
     */
    juce::ScopedNoDenormals noDenormals;

    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    // Limpiar el buffer (empezamos desde silencio)
    buffer.clear();

    // =========================================================================
    // PROCESAR MIDI
    // =========================================================================

    /**
     * Iteramos sobre todos los mensajes MIDI en este bloque.
     *
     * MidiBuffer contiene mensajes con timestamp (posición en samples).
     * Esto permite precisión sample-accurate, pero por simplicidad
     * procesamos todos los mensajes al inicio del bloque.
     */
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            // Nota presionada
            int noteNumber = message.getNoteNumber();
            float velocity = message.getFloatVelocity();  // 0.0 - 1.0

            synth.noteOn(noteNumber, velocity);
        }
        else if (message.isNoteOff())
        {
            // Nota liberada
            int noteNumber = message.getNoteNumber();
            synth.noteOff(noteNumber);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            // Panic - apagar todo
            synth.allNotesOff();
        }
        // Podríamos manejar más mensajes: pitch bend, mod wheel, etc.
    }

    // =========================================================================
    // SYNC AUTOMATABLE PARAMETERS → SYNTH ENGINE
    // =========================================================================

    synth.setTranspose(static_cast<int>(apvts.getRawParameterValue("transpose")->load()));

    synth.setFilterCutoff(apvts.getRawParameterValue("filterCutoff")->load());
    synth.setFilterResonance(apvts.getRawParameterValue("filterResonance")->load());
    synth.setFilterType(static_cast<FilterType>(
        apvts.getRawParameterValue("filterType")->load()));

    // Filter envelope
    synth.setFilterAttack(apvts.getRawParameterValue("filterAttack")->load());
    synth.setFilterDecay(apvts.getRawParameterValue("filterDecay")->load());
    synth.setFilterSustain(apvts.getRawParameterValue("filterSustain")->load());
    synth.setFilterRelease(apvts.getRawParameterValue("filterRelease")->load());
    synth.setFilterEnvAmount(apvts.getRawParameterValue("filterEnvAmount")->load());

    // Amp envelope
    synth.setAttack(apvts.getRawParameterValue("ampAttack")->load());
    synth.setDecay(apvts.getRawParameterValue("ampDecay")->load());
    synth.setSustain(apvts.getRawParameterValue("ampSustain")->load());
    synth.setRelease(apvts.getRawParameterValue("ampRelease")->load());

    // Rotation: only update synth when APVTS values actually change
    {
        float newRotX = apvts.getRawParameterValue("rotationX")->load();
        float newRotY = apvts.getRawParameterValue("rotationY")->load();
        float newRotZ = apvts.getRawParameterValue("rotationZ")->load();

        if (newRotX != lastRotX || newRotY != lastRotY || newRotZ != lastRotZ)
        {
            lastRotX = newRotX;
            lastRotY = newRotY;
            lastRotZ = newRotZ;
            synth.setObjectRotation(newRotX, newRotY, newRotZ);
        }
    }

    // Thickness: only update synth when APVTS value actually changes
    {
        float newThickness = apvts.getRawParameterValue("thickness")->load();
        if (newThickness != lastThickness)
        {
            lastThickness = newThickness;
            synth.setThickness(newThickness);
        }
    }

    // Light intensities: sync from APVTS to synth
    {
        static const char* intensityParamIds[] = {
            "lightIntensityKey", "lightIntensityFill", "lightIntensityRim"
        };
        for (int i = 0; i < 3; ++i)
        {
            float newInt = apvts.getRawParameterValue(intensityParamIds[i])->load();
            if (newInt != lastLightIntensity[i])
            {
                lastLightIntensity[i] = newInt;
                synth.setLightIntensity(i, newInt);
            }
        }
    }

    // Envelope mode
    {
        int newMode = static_cast<int>(apvts.getRawParameterValue("envMode")->load());
        if (newMode != lastEnvMode)
        {
            lastEnvMode = newMode;
            synth.setEnvelopeMode(newMode);
        }
    }

    // Deform / Wavefolding
    {
        float newDeformAmt = apvts.getRawParameterValue("deformAmount")->load();
        if (std::abs(newDeformAmt - lastDeformAmount) > 0.001f)
        {
            lastDeformAmount = newDeformAmt;
            synth.setDeformAmount(newDeformAmt);
        }

        float newDeformFreq = apvts.getRawParameterValue("deformFrequency")->load();
        if (std::abs(newDeformFreq - lastDeformFrequency) > 0.01f)
        {
            lastDeformFrequency = newDeformFreq;
            synth.setDeformFrequency(newDeformFreq);
        }

        int newNoiseType = static_cast<int>(apvts.getRawParameterValue("deformNoiseType")->load());
        if (newNoiseType != lastDeformNoiseType)
        {
            lastDeformNoiseType = newNoiseType;
            synth.setDeformNoiseType(newNoiseType);
        }

        float newDeformRate = apvts.getRawParameterValue("deformRate")->load();
        if (std::abs(newDeformRate - lastDeformRate) > 0.01f)
        {
            lastDeformRate = newDeformRate;
            synth.setDeformRate(newDeformRate);
        }
    }

    // Volume
    {
        float newVolume = apvts.getRawParameterValue("volume")->load();
        if (std::abs(newVolume - lastVolume) > 0.001f)
        {
            lastVolume = newVolume;
            synth.setVolume(newVolume);
        }
    }

    // Dual-oscillator material mixing
    {
        // materialA and materialB are set directly via setMaterial/setMaterialB
        // (not read from APVTS) — see getStateInformation/setStateInformation.

        // Blend Mode
        int mode = apvts.getRawParameterValue("blendMode")->load();
        if (mode != lastBlendMode)
        {
            synth.setBlendMode(mode);
            lastBlendMode = mode;
        }

        // Mix Amount (smooth parameter, no caching - allows smooth automation)
        synth.setMixAmount(apvts.getRawParameterValue("mixAmount")->load());

        // AM Depth
        float depth = apvts.getRawParameterValue("amDepth")->load();
        if (std::abs(depth - lastAmDepth) > 0.01f)
        {
            synth.setAMDepth(depth);
            lastAmDepth = depth;
        }

        // Osc B Detune
        float detune = apvts.getRawParameterValue("oscBDetune")->load();
        if (std::abs(detune - lastOscBDetune) > 0.1f)
        {
            synth.setOscBDetune(detune);
            lastOscBDetune = detune;
        }
    }

    // =========================================================================
    // GENERAR AUDIO
    // =========================================================================

    // El synth genera audio mono, lo copiamos a todos los canales después
    auto* channelData = buffer.getWritePointer(0);

    // Generar samples
    synth.processBlock(channelData, numSamples);

    // =========================================================================
    // COPIAR A STEREO (si hay más de un canal) + CHORUS (EXPERIMENTAL)
    // =========================================================================

    int chorusVoices = static_cast<int>(apvts.getRawParameterValue("chorusVoices")->load());

    if (chorusVoices > 0)
    {
        // processChorus writes channel 0 (and channel 1, if stereo) itself —
        // it reads the mono dry signal from channel 0 before overwriting it.
        processChorus(buffer, numSamples);
    }
    else if (numChannels > 1)
    {
        // Nuestro synth genera mono. Sin chorus, simplemente copiamos
        // el canal izquierdo al derecho.
        for (int ch = 1; ch < numChannels; ++ch)
        {
            buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
        }
    }
}

// ==============================================================================
// CHORUS (EXPERIMENTAL — audio-only prototype, see CLAUDE.md "Post-1.0.0
// Parking Lot / Chorus (geometry-trail) feature")
//
// Models up to 4 duplicate copies of the object receding along a light's
// direction: each copy arrives later (Spread) and quieter (Decay, an
// exponential per-hop falloff — same mathematical form as Beer-Lambert,
// but hop-based rather than distance-based since the engine has no spatial
// scale to compute a real distance from). The delay time for each copy is
// animated (Wobble) using the same simplex-noise machinery the Deformer
// uses elsewhere — a static delay+gain sum alone would just be a fixed comb
// filter, not a chorus; the movement is what gives it life.
//
// Single shared mono delay line, read independently by L and R at different
// noise phases, so a genuinely mono source produces real stereo width
// rather than an identical comb filter duplicated to both ears.
// ==============================================================================
void ElementsAudioProcessor::processChorus(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const int numChannels = buffer.getNumChannels();
    const int lineSize = static_cast<int>(chorusDelayLine.size());
    if (lineSize == 0)
        return;

    const int   voices   = juce::jlimit(0, kChorusMaxVoices,
                                static_cast<int>(apvts.getRawParameterValue("chorusVoices")->load()));
    const float spreadMs = apvts.getRawParameterValue("chorusSpread")->load();
    const float decay    = apvts.getRawParameterValue("chorusDecay")->load();
    const float wobble   = apvts.getRawParameterValue("chorusWobble")->load();

    const float sr = static_cast<float>(getSampleRate());
    const float modRangeSamples = kChorusModRangeMs * 0.001f * sr;

    // Wobble rate: slow enough to read as a chorus "breathe," not a tremolo.
    constexpr float kWobbleHz = 0.15f;
    const double noiseAdvancePerSample = static_cast<double>(kWobbleHz) / sr;

    auto* left  = buffer.getWritePointer(0);
    auto* right = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    auto readDelayLine = [&](float delaySamples) -> float
    {
        delaySamples = juce::jlimit(0.0f, static_cast<float>(lineSize - 2), delaySamples);
        float readPosF = static_cast<float>(chorusWritePos) - delaySamples;
        while (readPosF < 0.0f)
            readPosF += static_cast<float>(lineSize);

        int   idx0 = static_cast<int>(readPosF) % lineSize;
        int   idx1 = (idx0 + 1) % lineSize;
        float frac = readPosF - std::floor(readPosF);
        return chorusDelayLine[static_cast<size_t>(idx0)] * (1.0f - frac)
             + chorusDelayLine[static_cast<size_t>(idx1)] * frac;
    };

    auto computeWetSample = [&](double& noiseTime) -> float
    {
        float wet = 0.0f;
        float gain = 1.0f;
        for (int v = 1; v <= voices; ++v)
        {
            gain *= decay;  // hop-based exponential falloff (Beer-Lambert-style form)

            float baseDelayMs = spreadMs * static_cast<float>(v);
            float noise = simplex3D(static_cast<float>(v) * 1.7f,
                                     static_cast<float>(noiseTime), 0.0f);
            float delaySamples = (baseDelayMs * 0.001f * sr) + wobble * modRangeSamples * noise;

            wet += gain * readDelayLine(delaySamples);
        }
        return wet;
    };

    for (int i = 0; i < numSamples; ++i)
    {
        float dry = left[i];

        chorusDelayLine[static_cast<size_t>(chorusWritePos)] = dry;

        float wetL = computeWetSample(chorusNoiseTimeL);
        float outL = std::tanh(dry + wetL);
        left[i] = outL;

        if (right != nullptr)
        {
            float wetR = computeWetSample(chorusNoiseTimeR);
            right[i] = std::tanh(dry + wetR);
        }

        chorusWritePos = (chorusWritePos + 1) % lineSize;
        chorusNoiseTimeL += noiseAdvancePerSample;
        chorusNoiseTimeR += noiseAdvancePerSample;
    }
}

// ==============================================================================
// EDITOR
// ==============================================================================

juce::AudioProcessorEditor* ElementsAudioProcessor::createEditor()
{
    return new ElementsAudioProcessorEditor (*this);
}

bool ElementsAudioProcessor::hasEditor() const
{
    return true;
}

// ==============================================================================
// STATE (SAVE / LOAD)
// ==============================================================================

/**
 * getStateInformation - Guardar estado del plugin
 *
 * Llamado cuando el usuario guarda el proyecto en el DAW.
 * Debemos serializar todos los parámetros a un bloque de memoria.
 */
void ElementsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Crear un objeto XML para guardar el estado
    auto state = std::make_unique<juce::XmlElement>("ElementsState");

    // Guardar parámetros manuales (stored directly from synth, not via APVTS,
    // because their combos call synth setters directly rather than through APVTS)
    state->setAttribute("material",   synth.getMaterial());
    state->setAttribute("materialB",  synth.getMaterialB());
    state->setAttribute("blendMode",  synth.getBlendMode());
    state->setAttribute("geometry",   static_cast<int>(synth.getGeometry()));

    // Guardar estado de luces (enabled + source)
    for (int i = 0; i < 3; ++i)
    {
        state->setAttribute("lightEnabled" + juce::String(i), synth.isLightEnabled(i));
        state->setAttribute("lightSource" + juce::String(i), synth.getLightSource(i));
    }

    // Guardar parámetros APVTS (filter cutoff, resonance, type)
    auto apvtsState = apvts.copyState();
    auto apvtsXml = apvtsState.createXml();
    if (apvtsXml != nullptr)
        state->addChildElement(apvtsXml.release());

    // Convertir XML a datos binarios
    copyXmlToBinary(*state, destData);
}

/**
 * setStateInformation - Restaurar estado del plugin
 *
 * Llamado cuando el usuario abre un proyecto guardado.
 * Debemos deserializar los parámetros desde el bloque de memoria.
 */
void ElementsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Convertir datos binarios a XML
    auto state = getXmlFromBinary(data, sizeInBytes);

    if (state != nullptr && state->hasTagName("ElementsState"))
    {
        // Restaurar parámetros manuales (material, geometry, blendMode — not in APVTS)
        synth.setMaterial(state->getIntAttribute("material", 0));
        synth.setMaterialB(state->getIntAttribute("materialB", 0));
        synth.setBlendMode(state->getIntAttribute("blendMode", 0));
        lastBlendMode = synth.getBlendMode();
        synth.setGeometry(static_cast<Geometry>(state->getIntAttribute("geometry", 0)));

        // Restaurar estado de luces (enabled + source)
        for (int i = 0; i < 3; ++i)
        {
            bool defaultEnabled = (i == 0);
            synth.setLightEnabled(i, state->getBoolAttribute("lightEnabled" + juce::String(i), defaultEnabled));
            synth.setLightSource(i, state->getIntAttribute("lightSource" + juce::String(i), i));
        }

        // Restaurar parámetros APVTS
        auto* apvtsXml = state->getChildByName(apvts.state.getType());
        if (apvtsXml != nullptr)
            apvts.replaceState(juce::ValueTree::fromXml(*apvtsXml));

        // Migration: old projects stored rotation as XML attributes (not in APVTS)
        if (state->hasAttribute("rotationX"))
        {
            auto* px = apvts.getParameter("rotationX");
            auto* py = apvts.getParameter("rotationY");
            auto* pz = apvts.getParameter("rotationZ");
            px->setValueNotifyingHost(px->convertTo0to1(static_cast<float>(state->getDoubleAttribute("rotationX", 0.0))));
            py->setValueNotifyingHost(py->convertTo0to1(static_cast<float>(state->getDoubleAttribute("rotationY", 0.0))));
            pz->setValueNotifyingHost(pz->convertTo0to1(static_cast<float>(state->getDoubleAttribute("rotationZ", 0.0))));
        }

        // Migration: old projects stored volume as XML attribute (not in APVTS)
        if (state->hasAttribute("volume"))
        {
            auto* pv = apvts.getParameter("volume");
            pv->setValueNotifyingHost(pv->convertTo0to1(static_cast<float>(state->getDoubleAttribute("volume", 0.95))));
        }
    }
}

// ==============================================================================
// PLUGIN INSTANTIATION
// ==============================================================================

/**
 * createPluginFilter - Factory function
 *
 * JUCE llama a esta función para crear una instancia del plugin.
 * Es el "punto de entrada" del plugin.
 */
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ElementsAudioProcessor();
}
