/*
  ==============================================================================
    SynthEngine.cpp
    Elements - Wavetable synthesis engine implementation
    Ported from Python prototype
  ==============================================================================
*/

#include "SynthEngine.h"
#include <algorithm>
#include <cstring>

// ==============================================================================
// TPT SVF FILTER IMPLEMENTATION
// ==============================================================================

void BiquadFilter::setLowpass(float cutoffHz, float Q, float sampleRate)
{
    cutoffHz = clamp(cutoffHz, 20.0f, sampleRate * 0.45f);
    Q = clamp(Q, 0.5f, 10.0f);

    g = std::tan(3.14159265359f * cutoffHz / sampleRate);
    k = 1.0f / Q;
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
    mode = Mode::Low;
}

void BiquadFilter::setHighpass(float cutoffHz, float Q, float sampleRate)
{
    cutoffHz = clamp(cutoffHz, 20.0f, sampleRate * 0.45f);
    Q = clamp(Q, 0.5f, 10.0f);

    g = std::tan(3.14159265359f * cutoffHz / sampleRate);
    k = 1.0f / Q;
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
    mode = Mode::High;
}

void BiquadFilter::setBandpass(float cutoffHz, float Q, float sampleRate)
{
    cutoffHz = clamp(cutoffHz, 20.0f, sampleRate * 0.45f);
    Q = clamp(Q, 0.5f, 10.0f);

    g = std::tan(3.14159265359f * cutoffHz / sampleRate);
    k = 1.0f / Q;
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
    a3 = g * a2;
    mode = Mode::Band;
}

float BiquadFilter::process(float input)
{
    // TPT SVF: integrator states are continuous → no transients on signal changes
    float v3 = input - ic2eq;
    float v1 = a1 * ic1eq + a2 * v3;
    float v2 = ic2eq + a2 * ic1eq + a3 * v3;

    // Update integrator states
    ic1eq = 2.0f * v1 - ic1eq;
    ic2eq = 2.0f * v2 - ic2eq;

    switch (mode)
    {
        case Mode::Low:  return v2;
        case Mode::High: return input - k * v1 - v2;
        case Mode::Band: return v1;
    }
    return v2;
}

void BiquadFilter::reset()
{
    ic1eq = 0.0f;
    ic2eq = 0.0f;
}

// ==============================================================================
// WAVETABLE GENERATOR IMPLEMENTATION
// ==============================================================================

void WavetableGenerator::generateFromSpectrum(
    const std::array<float, NUM_WAVELENGTHS>& spectrum,
    float fundamentalFreq,
    float sampleRate,
    int maxHarmonics,
    std::array<float, WAVETABLE_SIZE>& output)
{
    // Clear output
    std::fill(output.begin(), output.end(), 0.0f);

    // Calculate maximum harmonic before Nyquist
    float nyquist = sampleRate / 2.0f;
    int nyquistLimit = static_cast<int>(nyquist / fundamentalFreq);
    int maxHarmonic = std::min(maxHarmonics, nyquistLimit);
    if (maxHarmonic < 1) maxHarmonic = 1;

    // ==========================================================================
    // HIGH-RESOLUTION SPECTRAL → HARMONIC MAPPING
    //
    // Each harmonic maps to a specific wavelength via continuous interpolation:
    //   Harmonic 1 (fundamental) → red wavelengths (750nm) → warmth
    //   Harmonic N (highest)     → blue wavelengths (400nm) → brightness
    //
    // This preserves the full 50-wavelength spectral shape instead of
    // collapsing it to 3 bands. When rotation changes the spectrum,
    // EVERY harmonic responds individually → audible timbre changes.
    // ==========================================================================

    constexpr float TWO_PI = 2.0f * 3.14159265359f;

    for (int h = 1; h <= maxHarmonic; ++h)
    {
        // Map harmonic to spectrum position: h=1 → red (end), h=max → blue (start)
        float specPos;
        if (maxHarmonic > 1)
            specPos = 1.0f - static_cast<float>(h - 1) / static_cast<float>(maxHarmonic - 1);
        else
            specPos = 0.5f;

        // Convert to spectrum array index with linear interpolation
        float idx = specPos * static_cast<float>(NUM_WAVELENGTHS - 1);
        int i0 = static_cast<int>(idx);
        int i1 = std::min(i0 + 1, NUM_WAVELENGTHS - 1);
        float frac = idx - static_cast<float>(i0);

        // Interpolate amplitude from spectrum
        float specAmp = spectrum[static_cast<size_t>(i0)] * (1.0f - frac)
                       + spectrum[static_cast<size_t>(i1)] * frac;

        // Emphasis curve: exaggerate spectral differences for audible contrast
        // Higher exponent = more contrast between transparent and opaque regions
        specAmp = std::pow(specAmp, 3.0f);

        // Natural harmonic rolloff (higher harmonics are quieter)
        // Moderate rolloff: preserves natural sound while allowing spectral character
        float rolloff = 1.0f / (1.0f + (h - 1) * 0.05f);
        float amplitude = specAmp * rolloff;

        if (amplitude > 0.001f)
        {
            for (int i = 0; i < WAVETABLE_SIZE; ++i)
            {
                float phase = static_cast<float>(i) / WAVETABLE_SIZE * TWO_PI * h;
                output[i] += amplitude * std::sin(phase);
            }
        }
    }

    // Apply soft saturation and normalize
    applySoftSaturation(output, 1.5f);
    normalize(output);
}

void WavetableGenerator::generateBandLimitedSet(
    const std::array<float, NUM_WAVELENGTHS>& spectrum,
    float sampleRate,
    WavetableSet& output)
{
    // Low frequencies (< 200Hz): Full harmonics
    generateFromSpectrum(spectrum, 100.0f, sampleRate, 20, output.low);

    // Mid-low frequencies (200-400Hz)
    generateFromSpectrum(spectrum, 300.0f, sampleRate, 20, output.midLow);

    // Mid frequencies (400-800Hz)
    generateFromSpectrum(spectrum, 600.0f, sampleRate, 16, output.mid);

    // Mid-high frequencies (800-1600Hz)
    generateFromSpectrum(spectrum, 1200.0f, sampleRate, 12, output.midHigh);

    // High frequencies (> 1600Hz)
    generateFromSpectrum(spectrum, 2000.0f, sampleRate, 8, output.high);
}

void WavetableGenerator::applySoftSaturation(std::array<float, WAVETABLE_SIZE>& wavetable, float drive)
{
    float tanhDrive = std::tanh(drive);
    for (int i = 0; i < WAVETABLE_SIZE; ++i)
    {
        wavetable[i] = std::tanh(wavetable[i] * drive) / tanhDrive;
    }
}

void WavetableGenerator::normalize(std::array<float, WAVETABLE_SIZE>& wavetable)
{
    // Normalize to -1.0 to 1.0 range
    float maxVal = 0.0f;
    for (int i = 0; i < WAVETABLE_SIZE; ++i)
    {
        maxVal = std::max(maxVal, std::abs(wavetable[i]));
    }
    if (maxVal > 0.0f)
    {
        for (int i = 0; i < WAVETABLE_SIZE; ++i)
        {
            wavetable[i] /= maxVal;
        }
    }
}

// ==============================================================================
// ELEMENTS SYNTH IMPLEMENTATION
// ==============================================================================

ElementsSynth::ElementsSynth()
{
    // Initialize lights (key light enabled by default with Sunset)
    lights[0] = { true, 0, 0, 0.5f };   // Key light, Sunset, position 0, intensity 0.5
    lights[1] = { false, 1, 1, 0.5f };  // Fill light, Daylight, position 1, intensity 0.5
    lights[2] = { false, 2, 2, 0.5f };  // Rim light, LED Cool, position 2, intensity 0.5

    // Initialize voice order tracking
    voiceOrder.fill(-1);
    voiceOrderCount = 0;

    // Calculate initial spectrum
    updateSpectrum();

    // Force immediate wavetable generation on startup (bypass throttling)
    if (regenPending)
    {
        spectrum = pendingSpectrum;
        regenerateWavetables();
        regenPending = false;
    }
}

void ElementsSynth::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = samplesPerBlock;

    // Reset all voices
    for (auto& voice : voices)
    {
        voice.reset();
    }

    // Set initial global filter coefficients
    float sr = static_cast<float>(sampleRate);
    switch (filterType)
    {
        case FilterType::Lowpass:  filter.setLowpass(filterCutoff, filterResonance, sr);  break;
        case FilterType::Highpass: filter.setHighpass(filterCutoff, filterResonance, sr); break;
        case FilterType::Bandpass: filter.setBandpass(filterCutoff, filterResonance, sr); break;
    }

    // Regenerate wavetables for new sample rate
    regenerateWavetables();
}

// Static phase for continuous tone test
static float debugPhase = 0.0f;

void ElementsSynth::processBlock(float* buffer, int numSamples)
{
    // Clear buffer to start fresh
    std::memset(buffer, 0, numSamples * sizeof(float));

    // No lights → total silence, no processing
    if (!hasActiveLights)
    {
        // Clear oscilloscope buffer when silent
        std::fill(oscilloscopeBuffer.begin(), oscilloscopeBuffer.end(), 0.0f);
        samplesSinceLastRegen += numSamples;  // Keep counting even when silent
        return;
    }

    // THROTTLING: Check if we have a pending wavetable regeneration
    samplesSinceLastRegen += numSamples;
    if (regenPending && samplesSinceLastRegen >= regenThrottleSamples)
    {
        // Enough time has passed, regenerate now
        spectrum = pendingSpectrum;
        regenerateWavetables();
        regenPending = false;
        samplesSinceLastRegen = 0;
    }

    // Smooth filter parameters toward targets (per-block, but with gentler coefficient)
    // Using smaller step to reduce zipper noise
    float filterSmoothCoeff = 0.02f;  // Gentler smoothing
    filterCutoff += (filterCutoffTarget - filterCutoff) * filterSmoothCoeff;
    filterResonance += (filterResonanceTarget - filterResonance) * filterSmoothCoeff;

    // Smooth spectral amplitude (controls volume based on angle)
    // Compute start/end for per-sample interpolation within the block
    float spectralAmpStart = spectralAmplitude;
    spectralAmplitude += (spectralAmplitudeTarget - spectralAmplitude) * 0.05f;
    float spectralAmpEnd = spectralAmplitude;

    // Smooth pitch offset from light intensity
    pitchOffsetSemitones += (pitchOffsetTarget - pitchOffsetSemitones) * 0.05f;

    // Smooth deform amount for wavefolding (0.15 = ~0.35s settle time at 86 blocks/sec)
    float prevDeformAmount = deformAmount;
    deformAmount += (deformAmountTarget - deformAmount) * 0.15f;

    // Snap to zero when close enough (avoids lingering near-zero tail)
    if (deformAmountTarget <= 0.0f && deformAmount < 0.001f)
        deformAmount = 0.0f;

    // Spectrum tracks smoothed deformAmount (not target) to avoid volume jumps
    // Also animates noise time offset for timbral movement when deforming
    if (currentGeometry == Geometry::Sphere)
    {
        bool settled = (prevDeformAmount > 0.001f && deformAmount <= 0.001f);

        // Animate noise offset for timbral drift when actively deforming
        bool animatedUpdate = false;
        if (deformAmount > 0.001f)
        {
            deformNoiseTimeOffset += 0.005f;  // ~0.43 units/sec at 86 blocks/sec
            deformAnimBlockCounter++;
            if (deformAnimBlockCounter >= 11)  // ~8Hz spectrum updates
            {
                deformAnimBlockCounter = 0;
                animatedUpdate = true;
            }
        }
        else
        {
            deformNoiseTimeOffset = 0.0f;
            deformAnimBlockCounter = 0;
        }

        if (settled || animatedUpdate || std::abs(deformAmount - lastSpectrumDeformAmount) > 0.001f)
        {
            lastSpectrumDeformAmount = deformAmount;
            updateSpectrum();
        }
    }

    // Filter envelope: compute start and end values for per-sample interpolation
    float filterEnvStart = filterEnvValue;
    filterEnvValue = generateFilterEnvelopeSample(numSamples);
    float filterEnvEnd = filterEnvValue;

    // Count active voices BEFORE processing
    int currentActiveVoiceCount = 0;
    for (const auto& voice : voices)
    {
        if (voice.active && !voice.stealing)
            ++currentActiveVoiceCount;
    }
    prevActiveVoiceCount = currentActiveVoiceCount;

    // Process crossfade
    float xfadeStart = 0.0f, xfadeEnd = 0.0f;
    if (crossfade.active)
    {
        xfadeStart = crossfade.progress;
        xfadeEnd = std::min(1.0f, xfadeStart + crossfade.increment * numSamples);
    }

    // Pre-compute filter state
    bool filterEnvActive = std::abs(filterEnvAmount) > 0.001f;
    float filterMixStep = (filterEnabledTarget - filterEnabledMix) * 0.001f;

    // Compute start and end modulated cutoff for interpolation across block
    float modCutoffStart = filterCutoff;
    float modCutoffEnd = filterCutoff;
    if (filterEnvActive)
    {
        modCutoffStart = filterCutoff * std::pow(2.0f, filterEnvAmount * filterEnvStart * 7.0f);
        modCutoffStart = clamp(modCutoffStart, 20.0f, 20000.0f);
        modCutoffEnd = filterCutoff * std::pow(2.0f, filterEnvAmount * filterEnvEnd * 7.0f);
        modCutoffEnd = clamp(modCutoffEnd, 20.0f, 20000.0f);
    }

    // Set initial global filter coefficients
    {
        float sr = static_cast<float>(sampleRate);
        switch (filterType)
        {
            case FilterType::Lowpass:  filter.setLowpass(modCutoffStart, filterResonance, sr);  break;
            case FilterType::Highpass: filter.setHighpass(modCutoffStart, filterResonance, sr); break;
            case FilterType::Bandpass: filter.setBandpass(modCutoffStart, filterResonance, sr); break;
        }
    }

    // Count active voices for mixing
    int activeVoiceCount = 0;

    // Process each voice
    for (auto& voice : voices)
    {
        if (!voice.active)
            continue;

        activeVoiceCount++;

        // Get appropriate wavetable for this voice's frequency
        const auto& wavetable = currentWavetables.getForFrequency(voice.frequency);
        const auto* oldWavetable = crossfade.active
            ? &crossfade.oldTables.getForFrequency(voice.frequency)
            : nullptr;

        // Apply pitch modulation from light intensity (+-2 semitones)
        float pitchMod = std::pow(2.0f, pitchOffsetSemitones / 12.0f);
        float phaseIncrement = (voice.frequency * pitchMod) / static_cast<float>(sampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            // Handle voice stealing fade-out
            if (voice.stealing)
            {
                float sample = readWavetable(voice.phase, wavetable);

                float fadeGain = static_cast<float>(voice.stealFadeRemaining) / Voice::FADE_SAMPLES;
                float interpSpectralAmpSteal = spectralAmpStart + (spectralAmpEnd - spectralAmpStart) * (static_cast<float>(i) / numSamples);
                sample *= fadeGain * voice.amplitude * voice.velocity * interpSpectralAmpSteal;

                buffer[i] += sample;

                voice.phase += phaseIncrement;
                if (voice.phase >= 1.0f)
                    voice.phase -= 1.0f;

                voice.stealFadeRemaining--;
                if (voice.stealFadeRemaining <= 0)
                {
                    voice.active = false;
                    voice.stealing = false;
                    break;
                }
                continue;
            }

            // Generate envelope
            float env = generateEnvelopeSample(voice);
            if (env < 0.0f)
            {
                voice.active = false;
                break;
            }

            // Read wavetable with linear interpolation
            float sample = readWavetable(voice.phase, wavetable);

            // Apply crossfade if active
            if (oldWavetable != nullptr)
            {
                float oldSample = readWavetable(voice.phase, *oldWavetable);
                float t = lerp(xfadeStart, xfadeEnd, static_cast<float>(i) / numSamples);
                sample = lerp(oldSample, sample, t);
            }

            // Calculate envelope level with velocity
            float envWithVelocity = env * voice.velocity;

            // Retrigger crossfade
            if (voice.retriggering && voice.retriggerFadeRemaining > 0)
            {
                float fadeProgress = 1.0f - static_cast<float>(voice.retriggerFadeRemaining) / Voice::FADE_SAMPLES;
                envWithVelocity = voice.retriggerStartLevel * (1.0f - fadeProgress) + envWithVelocity * fadeProgress;
                voice.retriggerFadeRemaining--;
                if (voice.retriggerFadeRemaining <= 0)
                    voice.retriggering = false;
            }

            // Apply amplitude and spectral amplitude (interpolated per-sample to avoid zipper noise)
            float interpSpectralAmp = spectralAmpStart + (spectralAmpEnd - spectralAmpStart) * (static_cast<float>(i) / numSamples);
            sample *= envWithVelocity * voice.amplitude * interpSpectralAmp;

            // Wavefolding: sinusoidal fold driven by deform amount (Sphere only)
            // drive=1 → sin(x)≈x (transparent), drive=15 → heavy folding + harmonics
            if (currentGeometry == Geometry::Sphere && deformAmount > 0.001f)
            {
                float drive = 1.0f + deformAmount * 14.0f;  // 1 to 15
                sample = std::sin(drive * sample);
            }

            // Anti-click fade-in for NEW voices
            if (voice.fadeInRemaining > 0)
            {
                float fadeGain = 1.0f - static_cast<float>(voice.fadeInRemaining) / Voice::FADE_SAMPLES;
                sample *= fadeGain;
                voice.fadeInRemaining--;
            }

            // Add to buffer
            buffer[i] += sample;

            // Advance phase
            voice.phase += phaseIncrement;
            if (voice.phase >= 1.0f)
                voice.phase -= 1.0f;

            voice.age++;
        }
    }

    // Update crossfade progress
    if (crossfade.active)
    {
        crossfade.progress = xfadeEnd;
        if (crossfade.progress >= 1.0f)
            crossfade.active = false;
    }

    // Update filter enabled mix (for next block)
    filterEnabledMix += filterMixStep * numSamples;
    filterEnabledMix = clamp(filterEnabledMix, 0.0f, 1.0f);

    // Voice count compensation: 1 voice = 1.0, 2 = 0.71, 4 = 0.50, 8 = 0.35
    // Smoothed to prevent clicks when voice count changes between blocks
    float voiceScaleTarget = 1.0f / std::sqrt(static_cast<float>(std::max(activeVoiceCount, 1)));
    float voiceScaleStart = voiceScaleSmoothed;
    // Per-sample smoothing coefficient: ~5ms ramp at 44.1kHz
    float voiceScaleSmoothCoeff = 1.0f - std::exp(-1.0f / (0.005f * static_cast<float>(sampleRate)));

    // Master processing: Global filter + Volume + Soft clipper
    for (int i = 0; i < numSamples; ++i)
    {
        // Smooth voice scale per-sample toward target
        voiceScaleSmoothed += (voiceScaleTarget - voiceScaleSmoothed) * voiceScaleSmoothCoeff;

        float sample = buffer[i];

        // Update global filter coefficients every 32 samples for smooth envelope interpolation
        if (i > 0 && (i % 32 == 0) && (filterEnvActive || modCutoffStart != modCutoffEnd))
        {
            float t = static_cast<float>(i) / numSamples;
            float interpCutoff = modCutoffStart + (modCutoffEnd - modCutoffStart) * t;
            float sr = static_cast<float>(sampleRate);
            switch (filterType)
            {
                case FilterType::Lowpass:  filter.setLowpass(interpCutoff, filterResonance, sr);  break;
                case FilterType::Highpass: filter.setHighpass(interpCutoff, filterResonance, sr); break;
                case FilterType::Bandpass: filter.setBandpass(interpCutoff, filterResonance, sr); break;
            }
        }

        // Global filter with smooth enable/disable
        float localFilterMix = filterEnabledMix + filterMixStep * i;
        localFilterMix = clamp(localFilterMix, 0.0f, 1.0f);

        if (localFilterMix > 0.001f)
        {
            float filtered = filter.process(sample);
            if (localFilterMix < 0.999f)
                sample = sample * (1.0f - localFilterMix) + filtered * localFilterMix;
            else
                sample = filtered;
        }

        sample *= volume * voiceScaleSmoothed;

        // Soft clipper (tanh) instead of hard clamp
        sample = std::tanh(sample);

        buffer[i] = sample;

        // Fill oscilloscope buffer
        oscilloscopeBuffer[static_cast<size_t>(oscilloscopeWritePos)] = sample;
        oscilloscopeWritePos = (oscilloscopeWritePos + 1) % 512;
    }

    // Clear oscilloscope buffer if no active voices (for clean display)
    if (activeVoiceCount == 0)
    {
        std::fill(oscilloscopeBuffer.begin(), oscilloscopeBuffer.end(), 0.0f);
    }
}

void ElementsSynth::releaseResources()
{
    // Stop all voices
    for (auto& voice : voices)
    {
        voice.reset();
    }
}

// ==============================================================================
// NOTE CONTROL
// ==============================================================================

void ElementsSynth::noteOn(int noteNumber, float velocity)
{
    // CLICK FIX 1: Check if same note is already playing - RETRIGGER with crossfade
    int existingVoice = findVoiceForNote(noteNumber);
    if (existingVoice >= 0)
    {
        Voice& voice = voices[existingVoice];

        // Calculate the CURRENT envelope level before resetting
        const ADSREnvelope& env = getActiveEnvelope();
        float currentEnvLevel = 0.0f;
        int attackSamples = static_cast<int>(env.attack * sampleRate);
        int decaySamples = static_cast<int>(env.decay * sampleRate);
        int releaseSamples = static_cast<int>(env.release * sampleRate);
        if (attackSamples < 1) attackSamples = 1;
        if (decaySamples < 1) decaySamples = 1;
        if (releaseSamples < 1) releaseSamples = 1;

        if (voice.releasing)
        {
            // In release phase
            if (voice.releaseAge < releaseSamples)
                currentEnvLevel = voice.releaseStartLevel * (1.0f - static_cast<float>(voice.releaseAge) / releaseSamples);
        }
        else if (voice.age <= attackSamples)
        {
            // In attack phase
            currentEnvLevel = static_cast<float>(voice.age) / attackSamples;
        }
        else if (voice.age < attackSamples + decaySamples)
        {
            // In decay phase
            float decayProgress = static_cast<float>(voice.age - attackSamples) / decaySamples;
            currentEnvLevel = 1.0f - (1.0f - env.sustain) * decayProgress;
        }
        else
        {
            // In sustain phase
            currentEnvLevel = env.sustain;
        }

        // Set up retrigger crossfade FROM current level TO new envelope
        voice.retriggering = true;
        voice.retriggerFadeRemaining = Voice::FADE_SAMPLES;
        voice.retriggerStartLevel = currentEnvLevel * voice.velocity;  // Include old velocity

        // Reset envelope to attack phase
        voice.velocity = 0.2f + velocity * 0.8f;
        voice.age = 0;
        voice.releasing = false;
        voice.releaseAge = 0;
        voice.releaseStartLevel = 1.0f;
        // DON'T use fadeInRemaining - we use retrigger crossfade instead
        voice.fadeInRemaining = 0;
        // DON'T reset phase - continue where we left off for smooth waveform

        // Retrigger filter envelope
        filterEnvAge = 0;
        filterEnvReleasing = false;
        filterEnvReleaseAge = 0;
        return;
    }

    // Find a free voice for new note
    int voiceIndex = findFreeVoice();

    if (voiceIndex < 0)
    {
        // No free voices - try soft steal (mark oldest for fade-out)
        stealOldestVoice();  // This marks a voice for stealing (doesn't return index anymore)

        // Try again - maybe a voice just finished or we need to wait
        voiceIndex = findFreeVoice();

        if (voiceIndex < 0)
        {
            // All voices still busy (stealing in progress) - drop this note
            // This is better than causing a click. The next noteOn will likely succeed.
            DBG("noteOn: No free voice available, dropping note " << noteNumber);
            return;
        }
    }

    Voice& voice = voices[voiceIndex];
    voice.reset();
    voice.frequency = midiNoteToFrequency(noteNumber);
    voice.velocity = 0.2f + velocity * 0.8f;  // Velocity sensitivity
    voice.amplitude = 1.0f;  // Could be based on spectrum mean
    voice.noteId = noteNumber;
    voice.active = true;
    voice.fadeInRemaining = Voice::FADE_SAMPLES;  // Anti-click fade-in
    voice.phase = 0.0f;  // Start at phase 0

    // Add to voice order for stealing
    // Remove if already present
    for (int i = 0; i < voiceOrderCount; ++i)
    {
        if (voiceOrder[i] == voiceIndex)
        {
            // Shift remaining elements
            for (int j = i; j < voiceOrderCount - 1; ++j)
                voiceOrder[j] = voiceOrder[j + 1];
            --voiceOrderCount;
            break;
        }
    }
    // Add at end (newest)
    if (voiceOrderCount < MAX_POLYPHONY)
    {
        voiceOrder[voiceOrderCount++] = voiceIndex;
    }

    // Retrigger filter envelope (monophonic — always follows last note)
    filterEnvAge = 0;
    filterEnvReleasing = false;
    filterEnvReleaseAge = 0;
}

void ElementsSynth::noteOff(int noteNumber)
{
    int voiceIndex = findVoiceForNote(noteNumber);
    if (voiceIndex >= 0)
    {
        Voice& voice = voices[voiceIndex];

        // Calculate current envelope level to start release from there (prevents clicks)
        const ADSREnvelope& env = getActiveEnvelope();
        int attackSamples = static_cast<int>(env.attack * sampleRate);
        int decaySamples = static_cast<int>(env.decay * sampleRate);
        if (attackSamples < 1) attackSamples = 1;
        if (decaySamples < 1) decaySamples = 1;

        float currentLevel;
        if (voice.age <= attackSamples)
        {
            // Still in attack
            currentLevel = static_cast<float>(voice.age) / attackSamples;
        }
        else if (voice.age < attackSamples + decaySamples)
        {
            // In decay
            float decayProgress = static_cast<float>(voice.age - attackSamples) / decaySamples;
            currentLevel = 1.0f - (1.0f - env.sustain) * decayProgress;
        }
        else
        {
            // In sustain
            currentLevel = env.sustain;
        }

        voice.releaseStartLevel = currentLevel;
        voice.releasing = true;
        voice.releaseAge = 0;
    }

    // Check if any voices are still active (not releasing) — if none, trigger filter env release
    bool anyActive = false;
    for (auto& v : voices)
        if (v.active && !v.releasing) { anyActive = true; break; }
    if (!anyActive && !filterEnvReleasing)
    {
        filterEnvReleasing = true;
        filterEnvReleaseAge = 0;
        filterEnvReleaseStartLevel = filterEnvValue;
    }
}

void ElementsSynth::allNotesOff()
{
    for (auto& voice : voices)
    {
        if (voice.active)
        {
            voice.releasing = true;
            voice.releaseAge = 0;
        }
    }
}

// ==============================================================================
// MATERIAL & LIGHT CONTROL
// ==============================================================================

void ElementsSynth::setThickness(float t)
{
    thickness = clamp(t, 0.1f, 3.0f);
    updateSpectrum();
    updatePhysicalEnvelope();
}

void ElementsSynth::setDeformAmount(float amount)
{
    deformAmountTarget = clamp(amount, 0.0f, 1.0f);
    // Spectrum update is driven by the smoothed value in processBlock
}

void ElementsSynth::setDeformFrequency(float freq)
{
    float clamped = clamp(freq, 0.5f, 10.0f);
    if (std::abs(clamped - deformFrequency) > 0.01f)
    {
        deformFrequency = clamped;
        // Force spectrum recalc if actively deforming
        if (currentGeometry == Geometry::Sphere && deformAmount > 0.001f)
            updateSpectrum();
    }
}

void ElementsSynth::setMaterial(int materialIndex)
{
    if (materialIndex >= 0 && materialIndex < NUM_MATERIALS)
    {
        currentMaterialIndex = materialIndex;
        updateSpectrum();
        updatePhysicalEnvelope();
    }
}

void ElementsSynth::setGeometry(Geometry geom)
{
    currentGeometry = geom;
    updateSpectrum();
}

void ElementsSynth::setLightEnabled(int lightIndex, bool enabled)
{
    if (lightIndex >= 0 && lightIndex < 3)
    {
        lights[lightIndex].enabled = enabled;
        updateSpectrum();
        updatePhysicalEnvelope();
    }
}

void ElementsSynth::setLightSource(int lightIndex, int sourceIndex)
{
    if (lightIndex >= 0 && lightIndex < 3 && sourceIndex >= 0 && sourceIndex < NUM_LIGHT_SOURCES)
    {
        lights[lightIndex].sourceIndex = sourceIndex;
        updateSpectrum();
        updatePhysicalEnvelope();
    }
}

void ElementsSynth::setLightIntensity(int lightIndex, float intensity)
{
    if (lightIndex >= 0 && lightIndex < 3)
    {
        lights[lightIndex].intensity = clamp(intensity, 0.0f, 1.0f);
        updateSpectrum();
        updatePhysicalEnvelope();
    }
}

void ElementsSynth::setEnvelopeMode(int mode)
{
    envelopeMode = (mode == 1) ? 1 : 0;
    if (envelopeMode == 1)
        updatePhysicalEnvelope();
}

const ADSREnvelope& ElementsSynth::getActiveEnvelope() const
{
    return (envelopeMode == 1) ? physicalEnvelope : envelope;
}

void ElementsSynth::updatePhysicalEnvelope()
{
    // Sum intensity of active lights (not averaged — more lights = more effect)
    float totalIntensity = 0.0f;
    int activeCount = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (lights[i].enabled)
        {
            totalIntensity += lights[i].intensity;
            ++activeCount;
        }
    }

    // Average intensity (0-1 range) for envelope parameters
    float avgIntensity = (activeCount > 0) ? (totalIntensity / activeCount) : 0.5f;

    // Get current material properties
    const auto& materials = getMaterials();
    const Material& material = materials[currentMaterialIndex];
    float ior = material.refractiveIndex;
    float avgTransmission = calculateAverageTransmission(material);

    // Attack: higher intensity = faster attack (more photon energy)
    physicalEnvelope.attack = lerp(0.5f, 0.005f, avgIntensity);
    physicalEnvelope.attack = clamp(physicalEnvelope.attack, 0.001f, 2.0f);

    // Decay: thick + absorptive = long decay
    physicalEnvelope.decay = thickness * (1.0f - avgTransmission) * 1.5f;
    physicalEnvelope.decay = clamp(physicalEnvelope.decay, 0.01f, 2.0f);

    // Sustain: high IOR = more internal reflections = higher sustain
    physicalEnvelope.sustain = clamp(ior / 2.42f, 0.1f, 0.95f);

    // Release: high IOR + thick = long release (trapped light)
    physicalEnvelope.release = ior * 0.2f * thickness;
    physicalEnvelope.release = clamp(physicalEnvelope.release, 0.01f, 2.0f);

    // Pitch offset: SUM-based deviation from equilibrium (0.5 per light)
    // More lights at high intensity = more pitch shift
    // Equilibrium = activeCount * 0.5, max deviation = activeCount * 0.5
    // Range: +-2 semitones at max deviation
    float equilibrium = activeCount * 0.5f;
    float deviation = totalIntensity - equilibrium;
    float maxDeviation = std::max(activeCount * 0.5f, 0.001f);
    pitchOffsetTarget = clamp((deviation / maxDeviation) * 2.0f, -2.0f, 2.0f);
}

void ElementsSynth::setObjectRotation(float x, float y, float z)
{
    // DEPRECATED: Build rotation matrix from Euler angles (still has gimbal lock issues)
    // Use setObjectRotationMatrix() instead for smooth, continuous rotation
    float rx = x * 3.14159265359f / 180.0f;
    float ry = y * 3.14159265359f / 180.0f;
    float rz = z * 3.14159265359f / 180.0f;

    float cx = std::cos(rx), sx = std::sin(rx);
    float cy = std::cos(ry), sy = std::sin(ry);
    float cz = std::cos(rz), sz = std::sin(rz);

    // Build rotation matrix (row-major): M = Rz * Ry * Rx
    objectRotationMatrix.data[0] = cy * cz;
    objectRotationMatrix.data[1] = -cy * sz;
    objectRotationMatrix.data[2] = sy;
    objectRotationMatrix.data[3] = sx * sy * cz + cx * sz;
    objectRotationMatrix.data[4] = -sx * sy * sz + cx * cz;
    objectRotationMatrix.data[5] = -sx * cy;
    objectRotationMatrix.data[6] = -cx * sy * cz + sx * sz;
    objectRotationMatrix.data[7] = cx * sy * sz + sx * cz;
    objectRotationMatrix.data[8] = cx * cy;

    updateSpectrum();
}

void ElementsSynth::setObjectRotationMatrix(const float* matrix4x4)
{
    // Set from OpenGL column-major 4x4 matrix (extracts upper-left 3x3)
    objectRotationMatrix.setFromColumnMajor4x4(matrix4x4);
    updateSpectrum();
}

void ElementsSynth::setObjectRotationMatrix(const RotationMatrix& matrix)
{
    objectRotationMatrix = matrix;
    updateSpectrum();
}

// ==============================================================================
// FILTER CONTROL
// ==============================================================================

void ElementsSynth::setFilterType(FilterType type)
{
    if (type != filterType)
    {
        filterType = type;
        filter.reset();
    }
}

void ElementsSynth::setFilterCutoff(float cutoffHz)
{
    filterCutoffTarget = clamp(cutoffHz, 20.0f, 20000.0f);
}

void ElementsSynth::setFilterResonance(float Q)
{
    filterResonanceTarget = clamp(Q, 0.5f, 10.0f);
}

void ElementsSynth::setFilterEnabled(bool enabled)
{
    filterEnabled = enabled;
    filterEnabledTarget = enabled ? 1.0f : 0.0f;
    // Don't reset filter immediately - let the crossfade handle it smoothly
}

// ==============================================================================
// ENVELOPE CONTROL
// ==============================================================================

void ElementsSynth::setAttack(float seconds)
{
    envelope.attack = clamp(seconds, 0.001f, 5.0f);
}

void ElementsSynth::setDecay(float seconds)
{
    envelope.decay = clamp(seconds, 0.001f, 5.0f);
}

void ElementsSynth::setSustain(float level)
{
    envelope.sustain = clamp(level, 0.0f, 1.0f);
}

void ElementsSynth::setRelease(float seconds)
{
    envelope.release = clamp(seconds, 0.001f, 5.0f);
}

// ==============================================================================
// FILTER ENVELOPE CONTROL
// ==============================================================================

void ElementsSynth::setFilterAttack(float seconds)
{
    filterEnvelope.attack = clamp(seconds, 0.001f, 5.0f);
}

void ElementsSynth::setFilterDecay(float seconds)
{
    filterEnvelope.decay = clamp(seconds, 0.001f, 5.0f);
}

void ElementsSynth::setFilterSustain(float level)
{
    filterEnvelope.sustain = clamp(level, 0.0f, 1.0f);
}

void ElementsSynth::setFilterRelease(float seconds)
{
    filterEnvelope.release = clamp(seconds, 0.001f, 5.0f);
}

void ElementsSynth::setFilterEnvAmount(float amount)
{
    filterEnvAmount = clamp(amount, 0.0f, 1.0f);
}

// ==============================================================================
// INTERNAL METHODS
// ==============================================================================

void ElementsSynth::updateSpectrum()
{
    // Get material
    const auto& materials = getMaterials();
    const Material& material = materials[currentMaterialIndex];

    // Count active lights
    int activeLightCount = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (lights[i].enabled)
            ++activeLightCount;
    }

    // PROBLEM 3: No lights → silence
    hasActiveLights = (activeLightCount > 0);
    if (!hasActiveLights)
    {
        std::fill(spectrum.begin(), spectrum.end(), 0.0f);
        // Don't regenerate wavetables — processBlock will output silence
        return;
    }

    // SUM contributions from all active lights
    // Each light adds its own spectral contribution based on:
    // - Its spectral distribution (Sunset=warm, Daylight=neutral, LED=cool)
    // - The angle between the object and that specific light
    // This creates richer timbres with multiple lights
    const auto& lightSources = getLightSources();
    std::array<float, NUM_WAVELENGTHS> combinedSpectrum{};
    std::fill(combinedSpectrum.begin(), combinedSpectrum.end(), 0.0f);

    float totalIntensity = 0.0f;
    for (int i = 0; i < 3; ++i)
    {
        if (!lights[i].enabled)
            continue;

        const LightSource& light = lightSources[lights[i].sourceIndex];
        const LightPosition& lightPos = getLightPosition(lights[i].positionIndex);

        std::array<float, NUM_WAVELENGTHS> lightSpectrum{};
        calculateSpectrumMultiFace(material, light, lightPos.position,
                                   objectRotationMatrix, currentGeometry, lightSpectrum,
                                   deformAmount, deformFrequency, deformNoiseTimeOffset);

        float effectiveIntensity = lights[i].intensity;
        for (int w = 0; w < NUM_WAVELENGTHS; ++w)
            combinedSpectrum[w] += lightSpectrum[w] * effectiveIntensity;

        totalIntensity += effectiveIntensity;
    }

    // PROBLEM 1 FIX: Normalize by total light intensity so volume stays
    // consistent regardless of how many lights are active.
    // More lights = richer timbre, NOT louder volume.
    if (totalIntensity > 0.0f)
    {
        float lightNorm = 1.0f / totalIntensity;
        for (int w = 0; w < NUM_WAVELENGTHS; ++w)
            combinedSpectrum[w] *= lightNorm;
    }

    // Beer-Lambert thickness: T_effective = T^thickness
    // thickness < 1.0 = thinner walls = more transmission = brighter sound
    // thickness > 1.0 = thicker walls = more absorption = darker/bassier sound
    if (std::abs(thickness - 1.0f) > 0.001f)
    {
        for (int w = 0; w < NUM_WAVELENGTHS; ++w)
            combinedSpectrum[w] = std::pow(std::max(combinedSpectrum[w], 0.001f), thickness);
    }

    // Per-material gain compensation.
    // Normalizes perceived volume across materials with different total transmission.
    // Gains calculated from avg transmission ratio to Diamond (baseline ~0.96).
    static const float materialGain[NUM_MATERIALS] = {
        1.0f,   // Diamond  — baseline (flat ~0.96 transmission)
        1.6f,   // Water    — red absorption, needs slight brightness boost
        1.7f,   // Amber    — strong blue absorption
        2.5f,   // Ruby     — near-zero below 600nm, low total energy
        2.0f,   // Gold     — sharper interband step, less total energy
        2.8f,   // Emerald  — narrow green peak, most energy concentrated
        2.2f,   // Amethyst — bimodal, green/yellow absorbed
        4.0f,   // Sapphire — steep cutoff, only blue/high harmonics pass
        2.8f,   // Copper   — deep red only, very low total energy
        4.5f    // Obsidian — near-opaque, minimal transmission
    };
    float matGain = materialGain[currentMaterialIndex];
    if (matGain != 1.0f)
    {
        for (int w = 0; w < NUM_WAVELENGTHS; ++w)
            combinedSpectrum[w] *= matGain;
    }

    // Normalize the combined spectrum to prevent clipping
    float maxVal = 0.0f;
    for (int w = 0; w < NUM_WAVELENGTHS; ++w)
    {
        if (combinedSpectrum[w] > maxVal)
            maxVal = combinedSpectrum[w];
    }

    float spectrumTotal = 0.0f;
    float scaleFactor = (maxVal > 1.0f) ? (1.0f / maxVal) : 1.0f;
    for (int w = 0; w < NUM_WAVELENGTHS; ++w)
    {
        spectrum[w] = combinedSpectrum[w] * scaleFactor;
        spectrumTotal += spectrum[w];
    }

    // BUG 3 FIX: If total spectrum is near zero (total internal reflection / no light),
    // treat as "no light" → silence. This is physically correct.
    constexpr float SPECTRUM_SILENCE_THRESHOLD = 0.5f;  // Sum of 50 wavelengths, ~0.01 average
    if (spectrumTotal < SPECTRUM_SILENCE_THRESHOLD)
    {
        hasActiveLights = false;
        // Don't regenerate wavetables — processBlock will output silence
        DBG("Spectrum total: " << spectrumTotal << " < threshold → SILENCE");
        return;
    }

    DBG("Spectrum total: " << spectrumTotal << " -> generating wavetable");

    // Calculate spectral amplitude (overall spectrum strength)
    // This affects the VOLUME of the sound based on angle/rotation
    // Range: 0.1 (very quiet at 90° angle) to 1.0 (full volume at 0° angle)
    float avgSpectrum = spectrumTotal / NUM_WAVELENGTHS;
    spectralAmplitudeTarget = clamp(avgSpectrum * 1.5f, 0.1f, 1.0f);

    DBG("Spectral amplitude target: " << spectralAmplitudeTarget);

    // THROTTLING: Don't regenerate wavetables too frequently to prevent clicks
    // Store the pending spectrum and mark for regeneration
    pendingSpectrum = spectrum;
    regenPending = true;
}

void ElementsSynth::regenerateWavetables()
{
    // Store old wavetables for crossfade if voices are playing
    bool hasActiveVoices = false;
    for (const auto& voice : voices)
    {
        if (voice.active)
        {
            hasActiveVoices = true;
            break;
        }
    }

    if (hasActiveVoices)
    {
        // CLICK FIX: If a crossfade is already in progress, don't restart it.
        // Just update the target wavetables. The crossfade will naturally morph
        // to the new target without an abrupt restart.
        if (!crossfade.active)
        {
            // Start new crossfade only if one isn't already running
            crossfade.oldTables = currentWavetables;
            crossfade.progress = 0.0f;
            crossfade.increment = 1.0f / (crossfadeDuration * static_cast<float>(sampleRate));
            crossfade.active = true;
        }
        // If crossfade is active, we just update currentWavetables below,
        // and the ongoing crossfade will blend toward the new values
    }

    // Generate new wavetables (target for crossfade, or immediate if no voices)
    wavetableGen.generateBandLimitedSet(spectrum, static_cast<float>(sampleRate), currentWavetables);
}

float ElementsSynth::generateEnvelopeSample(Voice& voice)
{
    const ADSREnvelope& env = getActiveEnvelope();
    int attackSamples = static_cast<int>(env.attack * sampleRate);
    int decaySamples = static_cast<int>(env.decay * sampleRate);
    int releaseSamples = static_cast<int>(env.release * sampleRate);

    if (attackSamples < 1) attackSamples = 1;
    if (decaySamples < 1) decaySamples = 1;
    if (releaseSamples < 1) releaseSamples = 1;

    if (voice.releasing)
    {
        // Release phase - start from the level when release was triggered (not sustain!)
        if (voice.releaseAge > releaseSamples)
            return -1.0f;  // Voice is dead (previous sample was 0)

        // Ensure final sample is exactly 0 before killing voice
        if (voice.releaseAge == releaseSamples)
        {
            voice.releaseAge++;
            return 0.0f;
        }

        float envLevel = voice.releaseStartLevel * (1.0f - static_cast<float>(voice.releaseAge) / releaseSamples);
        voice.releaseAge++;
        return envLevel;
    }
    else
    {
        // Attack/Decay/Sustain
        if (voice.age <= attackSamples)
        {
            // Attack: reaches exactly 1.0 at age == attackSamples
            return static_cast<float>(voice.age) / attackSamples;
        }
        else if (voice.age < attackSamples + decaySamples)
        {
            // Decay
            float decayProgress = static_cast<float>(voice.age - attackSamples) / decaySamples;
            return 1.0f - (1.0f - env.sustain) * decayProgress;
        }
        else
        {
            // Sustain
            return env.sustain;
        }
    }
}

float ElementsSynth::generateFilterEnvelopeSample(int numSamples)
{
    int attackSamples = static_cast<int>(filterEnvelope.attack * sampleRate);
    int decaySamples = static_cast<int>(filterEnvelope.decay * sampleRate);
    int releaseSamples = static_cast<int>(filterEnvelope.release * sampleRate);

    if (attackSamples < 1) attackSamples = 1;
    if (decaySamples < 1) decaySamples = 1;
    if (releaseSamples < 1) releaseSamples = 1;

    if (filterEnvReleasing)
    {
        if (filterEnvReleaseAge >= releaseSamples)
            return 0.0f;

        float env = filterEnvReleaseStartLevel * (1.0f - static_cast<float>(filterEnvReleaseAge) / releaseSamples);
        filterEnvReleaseAge += numSamples;
        return env;
    }
    else
    {
        if (filterEnvAge < attackSamples)
        {
            float env = static_cast<float>(filterEnvAge) / attackSamples;
            filterEnvAge += numSamples;
            return env;
        }
        else if (filterEnvAge < attackSamples + decaySamples)
        {
            float decayProgress = static_cast<float>(filterEnvAge - attackSamples) / decaySamples;
            float env = 1.0f - (1.0f - filterEnvelope.sustain) * decayProgress;
            filterEnvAge += numSamples;
            return env;
        }
        else
        {
            return filterEnvelope.sustain;
        }
    }
}

float ElementsSynth::readWavetable(float phase, const std::array<float, WAVETABLE_SIZE>& wavetable)
{
    // Linear interpolation
    float indexFloat = phase * WAVETABLE_SIZE;
    int index = static_cast<int>(indexFloat);
    float frac = indexFloat - index;

    int nextIndex = (index + 1) % WAVETABLE_SIZE;
    index = index % WAVETABLE_SIZE;

    return wavetable[index] * (1.0f - frac) + wavetable[nextIndex] * frac;
}

int ElementsSynth::findFreeVoice()
{
    for (int i = 0; i < MAX_POLYPHONY; ++i)
    {
        // Skip voices that are active OR currently being stolen (fade-out in progress)
        if (!voices[i].active && !voices[i].stealing)
            return i;
    }
    return -1;
}

int ElementsSynth::findVoiceForNote(int noteNumber)
{
    for (int i = 0; i < MAX_POLYPHONY; ++i)
    {
        if (voices[i].active && voices[i].noteId == noteNumber)
            return i;
    }
    return -1;
}

void ElementsSynth::stealOldestVoice()
{
    // CLICK FIX 2: Mark oldest voice for soft fade-out instead of hard steal
    // This allows the voice to fade out gracefully before being reused.
    // The caller should call findFreeVoice() again after this.

    // Find oldest non-releasing, non-stealing voice
    for (int i = 0; i < voiceOrderCount; ++i)
    {
        int voiceIndex = voiceOrder[i];
        if (voiceIndex >= 0 && voiceIndex < MAX_POLYPHONY &&
            voices[voiceIndex].active && !voices[voiceIndex].releasing &&
            !voices[voiceIndex].stealing)
        {
            // Mark for soft kill - voice will fade out over FADE_SAMPLES
            voices[voiceIndex].stealing = true;
            voices[voiceIndex].stealFadeRemaining = Voice::FADE_SAMPLES;

            // Remove from voice order (it's dying)
            for (int j = i; j < voiceOrderCount - 1; ++j)
                voiceOrder[j] = voiceOrder[j + 1];
            --voiceOrderCount;

            DBG("stealOldestVoice: Marked voice " << voiceIndex << " for fade-out");
            return;  // Don't return the index - let it fade out naturally
        }
    }

    // If all voices are already releasing or stealing, mark the oldest releasing voice
    for (int i = 0; i < voiceOrderCount; ++i)
    {
        int voiceIndex = voiceOrder[i];
        if (voiceIndex >= 0 && voiceIndex < MAX_POLYPHONY &&
            voices[voiceIndex].active && voices[voiceIndex].releasing &&
            !voices[voiceIndex].stealing)
        {
            // Force this releasing voice to steal-fade (faster than release)
            voices[voiceIndex].stealing = true;
            voices[voiceIndex].stealFadeRemaining = Voice::FADE_SAMPLES;

            for (int j = i; j < voiceOrderCount - 1; ++j)
                voiceOrder[j] = voiceOrder[j + 1];
            --voiceOrderCount;

            DBG("stealOldestVoice: Marked releasing voice " << voiceIndex << " for fade-out");
            return;
        }
    }

    // All voices are already stealing - nothing more we can do
    DBG("stealOldestVoice: All voices already stealing, cannot free more");
}

float ElementsSynth::midiNoteToFrequency(int noteNumber)
{
    // A4 (MIDI note 69) = 440 Hz
    return 440.0f * std::pow(2.0f, (noteNumber - 69) / 12.0f);
}
