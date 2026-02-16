#include "PluginProcessor.h"
#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter layout  (4 operators × 8 params = 32 automatable parameters)
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
SquareWaveSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Algo 4: OP1,OP3 = modulators  /  OP2,OP4 = carriers
    const int defaultTL[4]  = { 63, 0, 63, 0   };
    const int defaultAR[4]  = { 31, 31, 31, 31  };
    const int defaultDR[4]  = {  5,  5,  5,  5  };
    const int defaultSR[4]  = {  0,  0,  0,  0  };
    const int defaultSL[4]  = {  1,  1,  1,  1  };
    const int defaultRR[4]  = { 10, 10, 10, 10  };
    const int defaultMUL[4] = {  1,  1,  1,  1  };
    const int defaultDT[4]  = {  0,  0,  0,  0  };

    for (int op = 0; op < 4; op++) {
        juce::String pre = "OP" + juce::String(op + 1) + " ";

        // TL: UI 0=silent, 127=loud; internally inverted to chip TL
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_TL_ID[op], 1}, pre + "Level", 0, 127,
            127 - defaultTL[op]));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_AR_ID[op], 1}, pre + "Attack",  0, 31, defaultAR[op]));
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_DR_ID[op], 1}, pre + "Decay",   0, 31, defaultDR[op]));
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_SR_ID[op], 1}, pre + "Sus.Rate",0, 31, defaultSR[op]));
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_SL_ID[op], 1}, pre + "Sus.Lvl", 0, 15, defaultSL[op]));
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_RR_ID[op], 1}, pre + "Release", 0, 15, defaultRR[op]));
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_MUL_ID[op], 1}, pre + "Multi",  0, 15, defaultMUL[op]));

        // Detune: -3 to +3
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_DT_ID[op], 1},  pre + "Detune", -3, 3, defaultDT[op]));
    }

    return { params.begin(), params.end() };
}

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

void SquareWaveSynthAudioProcessor::prepareToPlay(double sampleRate, int)
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
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()) return false;
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  Detune mapping: UI -3..+3 → chip 0..7
//    UI:  -3  -2  -1   0  +1  +2  +3
//  Chip:   7   6   5   0   1   2   3
// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessor::pushParamsToVoices()
{
    for (int op = 0; op < 4; op++) {
        Ym2612Voice::OpParams p;

        int uiLevel = static_cast<int>(apvts.getRawParameterValue(OP_TL_ID[op])->load());
        p.tl  = 127 - uiLevel;

        p.ar  = static_cast<int>(apvts.getRawParameterValue(OP_AR_ID[op])->load());
        p.dr  = static_cast<int>(apvts.getRawParameterValue(OP_DR_ID[op])->load());
        p.sr  = static_cast<int>(apvts.getRawParameterValue(OP_SR_ID[op])->load());
        p.sl  = static_cast<int>(apvts.getRawParameterValue(OP_SL_ID[op])->load());
        p.rr  = static_cast<int>(apvts.getRawParameterValue(OP_RR_ID[op])->load());
        p.mul = static_cast<int>(apvts.getRawParameterValue(OP_MUL_ID[op])->load());

        int uiDT = static_cast<int>(apvts.getRawParameterValue(OP_DT_ID[op])->load());
        p.dt = (uiDT < 0) ? (4 - uiDT) : uiDT;

        for (int v = 0; v < NUM_VOICES; ++v)
            voices[v]->setOpParams(op, p);
    }
}

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

juce::AudioProcessorEditor* SquareWaveSynthAudioProcessor::createEditor()
{
    return new SquareWaveSynthAudioProcessorEditor(*this);
}

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
