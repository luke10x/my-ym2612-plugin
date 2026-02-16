#include "PluginProcessor.h"
#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter layout
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout
SquareWaveSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // TL range: 0 = maximum output, 127 = silent
    // We invert in the UI so it feels like a "level" (higher = louder)
    static const char* labels[4] = { "OP1 Level", "OP2 Level", "OP3 Level", "OP4 Level" };
    for (int i = 0; i < 4; i++) {
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { OP_LEVEL_ID[i], 1 },
            labels[i],
            0, 127,
            i == 3 ? 127 : 64  // OP4 carrier: level=127→TL=0 (max output)
                                    // OP1-3 modulators: level=64→TL=63 (mid FM colour)
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
        auto* voice = new Ym2612Voice();
        voices[i]   = voice;
        synth.addVoice(voice);
    }
}

SquareWaveSynthAudioProcessor::~SquareWaveSynthAudioProcessor() {}

// ─────────────────────────────────────────────────────────────────────────────
//  Playback lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void SquareWaveSynthAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    midiKeyboardState.reset();
    updateVoiceParams();
}

void SquareWaveSynthAudioProcessor::releaseResources()
{
    midiKeyboardState.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bus layout
// ─────────────────────────────────────────────────────────────────────────────

#ifndef JucePlugin_PreferredChannelConfigurations
bool SquareWaveSynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;
    return true;
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter push
// ─────────────────────────────────────────────────────────────────────────────

void SquareWaveSynthAudioProcessor::updateVoiceParams()
{
    // "level" param: 127=loud, 0=silent → TL (YM2612) = 127 - level
    // TL=0 is maximum chip output; TL=127 is silence.
    for (int v = 0; v < NUM_VOICES; ++v) {
        for (int op = 0; op < 4; op++) {
            int level = static_cast<int>(
                apvts.getRawParameterValue(OP_LEVEL_ID[op])->load());
            voices[v]->setOpLevel(op, 127 - level);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Processing
// ─────────────────────────────────────────────────────────────────────────────

void SquareWaveSynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    midiKeyboardState.processNextMidiBuffer(midiMessages, 0,
                                            buffer.getNumSamples(), true);
    updateVoiceParams();
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Editor
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioProcessorEditor* SquareWaveSynthAudioProcessor::createEditor()
{
    return new SquareWaveSynthAudioProcessorEditor(*this);
}

// ─────────────────────────────────────────────────────────────────────────────
//  State persistence
// ─────────────────────────────────────────────────────────────────────────────

void SquareWaveSynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SquareWaveSynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Plugin entry point
// ─────────────────────────────────────────────────────────────────────────────

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SquareWaveSynthAudioProcessor();
}
