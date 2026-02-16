#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "SynthSound.h"

/**
 * SynthVoice – a single square-wave voice.
 *
 * Features
 * --------
 *  • Bandlimited square wave via a series of harmonic sinusoids (up to Nyquist)
 *    This avoids the nasty aliasing you get from a naive comparator approach.
 *  • Duty cycle: 0–1, where 0.5 = symmetric square wave.
 *  • Simple ADSR envelope baked in.
 *  • Velocity sensitivity.
 */
class SynthVoice : public juce::SynthesiserVoice
{
public:
    // ──────────────────────────────────────────────────────────────────────
    //  Public API called from the processor
    // ──────────────────────────────────────────────────────────────────────

    /** Set duty cycle in [0.01, 0.99]. Thread-safe via atomic. */
    void setDutyCycle (float duty)
    {
        dutyCycle.store (juce::jlimit (0.01f, 0.99f, duty));
    }

    // ──────────────────────────────────────────────────────────────────────
    //  SynthesiserVoice overrides
    // ──────────────────────────────────────────────────────────────────────

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber,
                    float velocity,
                    juce::SynthesiserSound* /*sound*/,
                    int /*currentPitchWheelPosition*/) override
    {
        currentAngle  = 0.0;
        level         = velocity * 0.15;                    // scale to avoid clipping
        frequency     = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        angleDelta    = (juce::MathConstants<double>::twoPi * frequency)
                        / getSampleRate();

        adsr.setSampleRate (getSampleRate());
        adsr.noteOn();
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            adsr.noteOff();
        }
        else
        {
            clearCurrentNote();
            adsr.reset();
            currentAngle = 0.0;
        }
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample,
                          int numSamples) override
    {
        if (! adsr.isActive())
        {
            clearCurrentNote();
            return;
        }

        const float duty = dutyCycle.load();
        const double sr  = getSampleRate();

        // Pre-calculate the highest harmonic we can include below Nyquist.
        // We re-check each block (frequency doesn't change mid-note here, but
        // it makes the code robust to pitch-wheel later).
        const int maxHarmonic = static_cast<int> ((sr * 0.5) / frequency);

        auto* leftChannel  = outputBuffer.getWritePointer (0, startSample);
        auto* rightChannel = outputBuffer.getNumChannels() > 1
                             ? outputBuffer.getWritePointer (1, startSample)
                             : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            // Bandlimited square wave with duty cycle:
            //
            //   x(t) = Σ_{k=1,3,5,...} (2/(kπ)) sin(k·ω₀·t)   [duty=0.5]
            //
            // For arbitrary duty d, the Fourier coefficients are:
            //   a_k = (2/(kπ)) sin(k·π·d)
            // And the waveform is:
            //   x(t) = Σ_k a_k · cos(k·ω₀·t)   (cosine so phase is consistent)

            double sample = 0.0;
            for (int k = 1; k <= maxHarmonic; ++k)
            {
                const double coeff = (2.0 / (juce::MathConstants<double>::pi * k))
                                     * std::sin (k * juce::MathConstants<double>::pi * duty);
                sample += coeff * std::cos (k * currentAngle);
            }

            // Apply ADSR
            const float envGain = adsr.getNextSample();
            const float out     = static_cast<float> (sample * level) * envGain;

            leftChannel[i] += out;
            if (rightChannel != nullptr)
                rightChannel[i] += out;

            currentAngle += angleDelta;
            if (currentAngle >= juce::MathConstants<double>::twoPi)
                currentAngle -= juce::MathConstants<double>::twoPi;
        }

        // If ADSR finished its release, silence this voice
        if (! adsr.isActive())
            clearCurrentNote();
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

private:
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams { 0.01f, 0.1f, 0.8f, 0.3f };

    void prepareToPlay (double /*sampleRate*/, int /*blockSize*/)
    {
        adsr.setParameters (adsrParams);
    }

    double currentAngle = 0.0;
    double angleDelta   = 0.0;
    double level        = 0.0;
    double frequency    = 440.0;

    std::atomic<float> dutyCycle { 0.5f };
};
