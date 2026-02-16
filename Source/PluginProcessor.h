#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Ym2612Voice.h"
#include "SynthSound.h"

static constexpr int NUM_VOICES = 6;

// ── Per-operator parameter IDs ────────────────────────────────────────────────
// 4 operators × 8 parameters = 32 DAW-automatable parameters
// TL  : total level        0-127  (0=loud, 127=silent)
// AR  : attack rate        0-31
// DR  : decay rate         0-31
// SR  : sustain rate       0-31
// SL  : sustain level      0-15
// RR  : release rate       0-15
// MUL : frequency multiply 0-15
// DT  : detune             0-7   (4-7 = negative detune on the chip)
static const juce::String OP_TL_ID[4]  = { "op1_TL",  "op2_TL",  "op3_TL",  "op4_TL"  };
static const juce::String OP_AR_ID[4]  = { "op1_AR",  "op2_AR",  "op3_AR",  "op4_AR"  };
static const juce::String OP_DR_ID[4]  = { "op1_DR",  "op2_DR",  "op3_DR",  "op4_DR"  };
static const juce::String OP_SR_ID[4]  = { "op1_SR",  "op2_SR",  "op3_SR",  "op4_SR"  };
static const juce::String OP_SL_ID[4]  = { "op1_SL",  "op2_SL",  "op3_SL",  "op4_SL"  };
static const juce::String OP_RR_ID[4]  = { "op1_RR",  "op2_RR",  "op3_RR",  "op4_RR"  };
static const juce::String OP_MUL_ID[4] = { "op1_MUL", "op2_MUL", "op3_MUL", "op4_MUL" };
static const juce::String OP_DT_ID[4]  = { "op1_DT",  "op2_DT",  "op3_DT",  "op4_DT"  };

// ─────────────────────────────────────────────────────────────────────────────
class SquareWaveSynthAudioProcessor : public juce::AudioProcessor
{
public:
    SquareWaveSynthAudioProcessor();
    ~SquareWaveSynthAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool   acceptsMidi()  const override { return true;  }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState& getMidiKeyboardState() { return midiKeyboardState; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::Synthesiser synth;
    juce::MidiKeyboardState midiKeyboardState;
    std::array<Ym2612Voice*, NUM_VOICES> voices {};

    void pushParamsToVoices();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SquareWaveSynthAudioProcessor)
};
