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

    // =========================================================================
    // GENERAR AUDIO
    // =========================================================================

    // El synth genera audio mono, lo copiamos a todos los canales después
    auto* channelData = buffer.getWritePointer(0);

    // Generar samples
    synth.processBlock(channelData, numSamples);

    // =========================================================================
    // COPIAR A STEREO (si hay más de un canal)
    // =========================================================================

    /**
     * Nuestro synth genera mono. Para stereo, simplemente copiamos
     * el canal izquierdo al derecho.
     *
     * En el futuro podríamos agregar panning o efectos stereo.
     */
    if (numChannels > 1)
    {
        for (int ch = 1; ch < numChannels; ++ch)
        {
            buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
        }
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

    // Guardar parámetros manuales
    state->setAttribute("material", synth.getMaterial());
    state->setAttribute("geometry", static_cast<int>(synth.getGeometry()));
    state->setAttribute("volume", static_cast<double>(synth.getVolume()));

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
        // Restaurar parámetros manuales
        synth.setMaterial(state->getIntAttribute("material", 0));
        synth.setGeometry(static_cast<Geometry>(state->getIntAttribute("geometry", 0)));

        synth.setVolume(static_cast<float>(state->getDoubleAttribute("volume", 0.95)));

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
