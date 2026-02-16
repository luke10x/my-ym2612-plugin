#include "PluginProcessor.h"
#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter layout
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
SquareWaveSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // UI "level": 0 = silent, 127 = loudest.
    // YM2612 TL is the inverse: 0 = max output, 127 = silent.
    // The processor converts:  TL = 127 - level  before pushing to the chip.
    //
    // Defaults for Algorithm 4 ((OP1→OP2) + (OP3→OP4)):
    //   OP1, OP3 = modulators → level 64  (TL = 63, half-open, adds FM colour)
    //   OP2, OP4 = carriers   → level 127 (TL =  0, maximum output)
    static const char* kLabels[4] = { "OP1 Level", "OP2 Level", "OP3 Level", "OP4 Level" };
    static const int   kDefaults[4] = { 64, 127, 64, 127 };  // OP1,OP2,OP3,OP4

    for (int i = 0; i < 4; i++) {
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { OP_LEVEL_ID[i], 1 },
            kLabels[i],
            0, 127,
            kDefaults[i]
        ));
    }
    return { params.begin(), params.end() };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────
SquareWaveSynthAudioProcessor::SquareWaveSynthAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    synth.addSound(new SynthSound());
    for (int i = 0; i < NUM_VOICES; ++i) {
        auto* v  = new Ym2612Voice();
        voices[i] = v;
        synth.addVoice(v);
    }
}

SquareWaveSynthAudioProcessor::~SquareWaveSynthAudioProcessor() {}

// ─────────────────────────────────────────────────────────────────────────────
//  Playback
// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessor::prepareToPlay(double sampleRate, int /*block*/)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    midiKeyboardState.reset();
    pushParamsToVoices();
}

void SquareWaveSynthAudioProcessor::releaseResources()
{
    midiKeyboardState.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SquareWaveSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::disabled()) return false;
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter push
// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessor::pushParamsToVoices()
{
    for (int v = 0; v < NUM_VOICES; ++v) {
        for (int op = 0; op < 4; op++) {
            int level = static_cast<int>(
                apvts.getRawParameterValue(OP_LEVEL_ID[op])->load());
            // UI level: 0=silent, 127=loud  →  YM2612 TL = 127 - level
            voices[v]->setOpTL(op, 127 - level);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Process block
// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    midiKeyboardState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);
    pushParamsToVoices();
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Editor
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorEditor* SquareWaveSynthAudioProcessor::createEditor()
{
    return new SquareWaveSynthAudioProcessorEditor(*this);
}

// ─────────────────────────────────────────────────────────────────────────────
//  State
// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void SquareWaveSynthAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SquareWaveSynthAudioProcessor();
}
