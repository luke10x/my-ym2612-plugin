#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Ym2612Voice.h"
#include "SynthSound.h"

static constexpr int NUM_VOICES = 6;

// ── Per-operator parameter IDs ────────────────────────────────────────────────
// 4 operators × 12 parameters = 48 per-operator params
static const juce::String OP_TL_ID[4]  = { "op1_TL",  "op2_TL",  "op3_TL",  "op4_TL"  };
static const juce::String OP_AR_ID[4]  = { "op1_AR",  "op2_AR",  "op3_AR",  "op4_AR"  };
static const juce::String OP_DR_ID[4]  = { "op1_DR",  "op2_DR",  "op3_DR",  "op4_DR"  };
static const juce::String OP_SR_ID[4]  = { "op1_SR",  "op2_SR",  "op3_SR",  "op4_SR"  };
static const juce::String OP_SL_ID[4]  = { "op1_SL",  "op2_SL",  "op3_SL",  "op4_SL"  };
static const juce::String OP_RR_ID[4]  = { "op1_RR",  "op2_RR",  "op3_RR",  "op4_RR"  };
static const juce::String OP_MUL_ID[4] = { "op1_MUL", "op2_MUL", "op3_MUL", "op4_MUL" };
static const juce::String OP_DT_ID[4]  = { "op1_DT",  "op2_DT",  "op3_DT",  "op4_DT"  };
static const juce::String OP_RS_ID[4]  = { "op1_RS",  "op2_RS",  "op3_RS",  "op4_RS"  };
static const juce::String OP_AM_ID[4]  = { "op1_AM",  "op2_AM",  "op3_AM",  "op4_AM"  };
static const juce::String OP_SSG_EN_ID[4] = { "op1_SSGEN", "op2_SSGEN", "op3_SSGEN", "op4_SSGEN" };
static const juce::String OP_SSG_MODE_ID[4] = { "op1_SSGMODE", "op2_SSGMODE", "op3_SSGMODE", "op4_SSGMODE" };

// ── Global parameters ─────────────────────────────────────────────────────────
static const juce::String GLOBAL_ALGORITHM  = "algorithm";     // 0-7
static const juce::String GLOBAL_FEEDBACK   = "feedback";      // 0-7
static const juce::String GLOBAL_LFO_ENABLE = "lfoEnable";     // bool
static const juce::String GLOBAL_LFO_FREQ   = "lfoFreq";       // 0-7 (chip values)
static const juce::String GLOBAL_AMS        = "ams";           // AM LFO sens 0-3
static const juce::String GLOBAL_FMS        = "fms";           // FM LFO sens 0-7
static const juce::String GLOBAL_OCTAVE     = "octave";        // -2 to +2

// LFO frequency table (Hz) — chip values 0-7
static const char* LFO_FREQ_NAMES[8] = {
    "3.98 Hz", "5.56 Hz", "6.02 Hz", "6.37 Hz",
    "6.88 Hz", "9.63 Hz", "48.1 Hz", "72.2 Hz"
};

// Algorithm names for dropdown
static const char* ALGORITHM_NAMES[8] = {
    "1 → 2 → 3 → 4",
    "(1+2) → 3 → 4",
    "(1+(2→3)) → 4",
    "((1→2)+3) → 4",
    "(1→2) + (3→4)",
    "(1→2) + (1→3) + (1→4)",
    "(1→2) + 3 + 4",
    "1 + 2 + 3 + 4"
};

// SSG-EG mode names (Furnace-style notation)
static const char* SSG_MODE_NAMES[8] = {
    "Down Down Down",      // 0: \\\
    "Down.",               // 1: \__
    "Down Up Down Up",     // 2: \/\/
    "Down UP",             // 3: \¯¯
    "Up Up Up",            // 4: ///
    "Up.",                 // 5: /¯¯
    "Up Down Up Down",     // 6: /\/\
    "Up DOWN"              // 7: /___
};

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