#include "PluginProcessor.h"
#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter layout
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
SquareWaveSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Defaults for Algo 4: (1→2)+(3→4), feedback 5, modulators half-open
    const int defaultTL[4]  = { 63, 0, 63, 0   };
    const int defaultAR[4]  = { 31, 31, 31, 31  };
    const int defaultDR[4]  = {  5,  5,  5,  5  };
    const int defaultSR[4]  = {  0,  0,  0,  0  };
    const int defaultSL[4]  = {  1,  1,  1,  1  };
    const int defaultRR[4]  = { 10, 10, 10, 10  };
    const int defaultMUL[4] = {  1,  1,  1,  1  };
    const int defaultDT[4]  = {  0,  0,  0,  0  };
    const int defaultRS[4]  = {  0,  0,  0,  0  };
    const int defaultAM[4]  = {  0,  0,  0,  0  };
    const int defaultSSGEn[4]   = { 0, 0, 0, 0 };
    const int defaultSSGMode[4] = { 0, 0, 0, 0 };

    // ── Per-operator parameters ───────────────────────────────────────────────
    for (int op = 0; op < 4; op++) {
        juce::String pre = "OP" + juce::String(op + 1) + " ";

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_TL_ID[op], 1}, pre + "Level", 0, 127, 127 - defaultTL[op]));
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
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_DT_ID[op], 1},  pre + "Detune", -3, 3, defaultDT[op]));
        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{OP_RS_ID[op], 1},  pre + "RateScale", 0, 3, defaultRS[op]));
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{OP_AM_ID[op], 1},  pre + "AM Enable", defaultAM[op] != 0));
        
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{OP_SSG_EN_ID[op], 1}, pre + "SSG-EG Enable", defaultSSGEn[op] != 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{OP_SSG_MODE_ID[op], 1}, pre + "SSG-EG Mode",
            getSsgModeNames(), defaultSSGMode[op]));
    }

    // ── Global parameters ──────────────────────────────────────────────────────
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{GLOBAL_ALGORITHM, 1}, "Algorithm",
        getAlgorithmNames(), 4));  // default = algo 4

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{GLOBAL_FEEDBACK, 1}, "Feedback", 0, 7, 5));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{GLOBAL_LFO_ENABLE, 1}, "LFO Enable", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{GLOBAL_LFO_FREQ, 1}, "LFO Freq",
        getLfoFreqNames(), 0));  // default 3.98 Hz

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{GLOBAL_AMS, 1}, "AMS (AM Sens)", 0, 3, 0));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{GLOBAL_FMS, 1}, "FMS (FM Sens)", 0, 7, 0));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{GLOBAL_OCTAVE, 1}, "Octave", -2, 2, 0));

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
//  Push parameters to voices
// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessor::pushParamsToVoices()
{
    // Read global params
    Ym2612Voice::GlobalParams gp;
    gp.algorithm = static_cast<int>(apvts.getRawParameterValue(GLOBAL_ALGORITHM)->load());
    gp.feedback  = static_cast<int>(apvts.getRawParameterValue(GLOBAL_FEEDBACK)->load());
    gp.lfoEnable = apvts.getRawParameterValue(GLOBAL_LFO_ENABLE)->load() > 0.5f;
    gp.lfoFreq   = static_cast<int>(apvts.getRawParameterValue(GLOBAL_LFO_FREQ)->load());
    gp.ams       = static_cast<int>(apvts.getRawParameterValue(GLOBAL_AMS)->load());
    gp.fms       = static_cast<int>(apvts.getRawParameterValue(GLOBAL_FMS)->load());
    gp.octave    = static_cast<int>(apvts.getRawParameterValue(GLOBAL_OCTAVE)->load());

    // Read per-operator params
    Ym2612Voice::OpParams ops[4];
    for (int op = 0; op < 4; op++) {
        int uiLevel = static_cast<int>(apvts.getRawParameterValue(OP_TL_ID[op])->load());
        ops[op].tl  = 127 - uiLevel;

        ops[op].ar  = static_cast<int>(apvts.getRawParameterValue(OP_AR_ID[op])->load());
        ops[op].dr  = static_cast<int>(apvts.getRawParameterValue(OP_DR_ID[op])->load());
        ops[op].sr  = static_cast<int>(apvts.getRawParameterValue(OP_SR_ID[op])->load());
        ops[op].sl  = static_cast<int>(apvts.getRawParameterValue(OP_SL_ID[op])->load());
        ops[op].rr  = static_cast<int>(apvts.getRawParameterValue(OP_RR_ID[op])->load());
        ops[op].mul = static_cast<int>(apvts.getRawParameterValue(OP_MUL_ID[op])->load());

        int uiDT = static_cast<int>(apvts.getRawParameterValue(OP_DT_ID[op])->load());
        ops[op].dt = (uiDT < 0) ? (4 - uiDT) : uiDT;

        ops[op].rs  = static_cast<int>(apvts.getRawParameterValue(OP_RS_ID[op])->load());
        ops[op].am  = apvts.getRawParameterValue(OP_AM_ID[op])->load() > 0.5f ? 1 : 0;
        
        ops[op].ssgEnable = apvts.getRawParameterValue(OP_SSG_EN_ID[op])->load() > 0.5f ? 1 : 0;
        ops[op].ssgMode   = static_cast<int>(apvts.getRawParameterValue(OP_SSG_MODE_ID[op])->load());
    }

    // Push to all voices
    for (int v = 0; v < NUM_VOICES; ++v) {
        voices[v]->setGlobalParams(gp);
        for (int op = 0; op < 4; op++)
            voices[v]->setOpParams(op, ops[op]);
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
