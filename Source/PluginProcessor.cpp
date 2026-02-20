#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FurnaceFormat.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter layout
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
ARM2612AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Defaults for Algo 4: (1→2)+(3→4), feedback 5
    // TL: 0=loud, 127=silent (Furnace convention)
    // Modulators (OP1, OP3) should be loud (low values)
    const int defaultTL[4]  = { 0, 63, 0, 63 };
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
            juce::ParameterID{OP_TL_ID[op], 1}, pre + "Level", 0, 127, defaultTL[op]));
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
ARM2612AudioProcessor::ARM2612AudioProcessor()
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

ARM2612AudioProcessor::~ARM2612AudioProcessor() {}

void ARM2612AudioProcessor::prepareToPlay(double sampleRate, int)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
    midiKeyboardState.reset();
    pushParamsToVoices();
}

void ARM2612AudioProcessor::releaseResources()
{
    midiKeyboardState.reset();
}

void ARM2612AudioProcessor::setInstrumentName(const juce::String& name)
{
    instrumentName = name;
}

juce::String ARM2612AudioProcessor::getInstrumentName() const
{
    return instrumentName;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ARM2612AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()) return false;
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  Push parameters to voices
// ─────────────────────────────────────────────────────────────────────────────
void ARM2612AudioProcessor::pushParamsToVoices()
{
    // Read global params
    Ym2612Voice::GlobalParams gp;
    gp.algorithm = static_cast<int>(apvts.getRawParameterValue(GLOBAL_ALGORITHM)->load());
    gp.feedback  = static_cast<int>(apvts.getRawParameterValue(GLOBAL_FEEDBACK)->load());
    
    // LFO Freq dropdown: 0=Off, 1-8=chip values 0-7
    int lfoFreqIdx = static_cast<int>(apvts.getRawParameterValue(GLOBAL_LFO_FREQ)->load());
    gp.lfoEnable = (lfoFreqIdx > 0);
    gp.lfoFreq   = (lfoFreqIdx > 0) ? (lfoFreqIdx - 1) : 0;
    
    gp.ams       = static_cast<int>(apvts.getRawParameterValue(GLOBAL_AMS)->load());
    gp.fms       = static_cast<int>(apvts.getRawParameterValue(GLOBAL_FMS)->load());
    gp.octave    = static_cast<int>(apvts.getRawParameterValue(GLOBAL_OCTAVE)->load());

    // Read per-operator params
    Ym2612Voice::OpParams ops[4];
    for (int op = 0; op < 4; op++) {
        // TL: Use directly from parameter (0=loud, 127=silent)
        ops[op].tl  = static_cast<int>(apvts.getRawParameterValue(OP_TL_ID[op])->load());

        ops[op].ar  = static_cast<int>(apvts.getRawParameterValue(OP_AR_ID[op])->load());
        ops[op].dr  = static_cast<int>(apvts.getRawParameterValue(OP_DR_ID[op])->load());
        ops[op].sr  = static_cast<int>(apvts.getRawParameterValue(OP_SR_ID[op])->load());
        ops[op].sl  = static_cast<int>(apvts.getRawParameterValue(OP_SL_ID[op])->load());
        ops[op].rr  = static_cast<int>(apvts.getRawParameterValue(OP_RR_ID[op])->load());
        ops[op].mul = static_cast<int>(apvts.getRawParameterValue(OP_MUL_ID[op])->load());

        int uiDT = static_cast<int>(apvts.getRawParameterValue(OP_DT_ID[op])->load());
        // UI(-3..+3) → chip(0-7): chipValue = displayValue + 3
        ops[op].dt = (uiDT + 3) & 7;

        ops[op].rs  = static_cast<int>(apvts.getRawParameterValue(OP_RS_ID[op])->load());
        ops[op].am  = apvts.getRawParameterValue(OP_AM_ID[op])->load() > 0.5f ? 1 : 0;
        
        // SSG Mode dropdown: 0=Off/disabled, 1-8=chip modes 0-7
        int ssgIdx = static_cast<int>(apvts.getRawParameterValue(OP_SSG_MODE_ID[op])->load());
        ops[op].ssgEnable = (ssgIdx > 0) ? 1 : 0;
        ops[op].ssgMode   = (ssgIdx > 0) ? (ssgIdx - 1) : 0;
    }

    // Push to all voices
    for (int v = 0; v < NUM_VOICES; ++v) {
        voices[v]->setGlobalParams(gp);
        for (int op = 0; op < 4; op++)
            voices[v]->setOpParams(op, ops[op]);
    }
}

void ARM2612AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    midiKeyboardState.processNextMidiBuffer(midi, 0, buffer.getNumSamples(), true);
    pushParamsToVoices();
    synth.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
    
    // Push samples to FIFO for oscilloscope
    if (buffer.getNumChannels() > 0)
    {
        const int numSamples = buffer.getNumSamples();
        const int numToWrite = juce::jmin(numSamples, audioFifo.getFreeSpace());
        
        if (numToWrite > 0)
        {
            int start1, size1, start2, size2;
            audioFifo.prepareToWrite(numToWrite, start1, size1, start2, size2);
            
            // Mix to mono and write to FIFO
            for (int i = 0; i < size1; ++i)
            {
                float sample = buffer.getSample(0, i);
                if (buffer.getNumChannels() > 1)
                    sample = (sample + buffer.getSample(1, i)) * 0.5f;
                audioFifoBuffer[start1 + i] = sample;
            }
            
            for (int i = 0; i < size2; ++i)
            {
                float sample = buffer.getSample(0, size1 + i);
                if (buffer.getNumChannels() > 1)
                    sample = (sample + buffer.getSample(1, size1 + i)) * 0.5f;
                audioFifoBuffer[start2 + i] = sample;
            }
            
            audioFifo.finishedWrite(size1 + size2);
        }
    }
}

juce::AudioProcessorEditor* ARM2612AudioProcessor::createEditor()
{
    return new ARM2612AudioProcessorEditor(*this);
}

void ARM2612AudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    // Add instrument name to state
    state.setProperty("instrumentName", instrumentName, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void ARM2612AudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType())) {
        auto state = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(state);
        // Restore instrument name
        instrumentName = state.getProperty("instrumentName", "YM2612 Instrument").toString();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ARM2612AudioProcessor();
}

// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
//  Furnace .fui Import/Export
//
//  Furnace stores DT in chip format (0-7: 0-3 pos, 4-7 neg).
//  We display DT as UI format (-3 to +3).
//  Conversion:  chip→UI:  dt<4 → dt,  dt>=4 → -(dt-4)  (except 4→0 clash: 4→0neg)
//               Actually Furnace: 0=no detune, 1-3=pos, 4=no(neg), 5-7=neg
//               Our push: uiDT<0 → chip=4-uiDT,  uiDT>=0 → chip=uiDT
//
//  Furnace ssgEnv: value 0-7 with bit3 set means "enabled" (i.e. values 8-15).
//  But Furnace source also uses 8 to mean "disabled" in some contexts.
//  The actual encoding in the FM feature block is:
//    bit3 (0x08) = SSG-EG enable
//    bits2:0     = mode 0-7
//  So: disabled = 0x00,  enabled mode N = 0x08 | N
// ─────────────────────────────────────────────────────────────────────────────

bool ARM2612AudioProcessor::importFurnaceInstrument(const juce::File& file)
{
    FurnaceFormat::Instrument ins;
    if (!FurnaceFormat::readFui(file, ins))
        return false;

    // Debug logging
    DBG("=== IMPORT DEBUG ===");
    DBG("File name: " << file.getFileName());
    DBG("File name without extension: " << file.getFileNameWithoutExtension());
    DBG("Instrument name from file: '" << ins.name << "'");
    DBG("Instrument name is empty: " << (ins.name.isEmpty() ? "YES" : "NO"));
    
    // Set instrument name - prefer name from file, fallback to filename
    juce::String nameToUse = ins.name.isEmpty() ? file.getFileNameWithoutExtension() : ins.name;
    DBG("Name being set: '" << nameToUse << "'");
    setInstrumentName(nameToUse);

    auto set = [&](const juce::String& id, float rawValue) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(rawValue));
    };

    // Global
    set(GLOBAL_ALGORITHM, float(ins.alg & 7));
    set(GLOBAL_FEEDBACK,  float(ins.fb  & 7));
    set(GLOBAL_AMS,       float(ins.ams & 3));
    set(GLOBAL_FMS,       float(ins.fms & 7));
    
    // LFO: Furnace has separate enable flag (bit in opCount byte, not saved in our format)
    // For now, assume LFO is disabled on import. User can enable via dropdown.
    set(GLOBAL_LFO_FREQ, 0.0f);  // index 0 = Off

    // YM2612 operator slot mapping:
    // Furnace stores in slot order: [0]=OP1, [1]=OP3, [2]=OP2, [3]=OP4
    // Our UI displays in order: OP1, OP2, OP3, OP4
    // So we need to remap: Furnace[0]→UI[0], Furnace[2]→UI[1], Furnace[1]→UI[2], Furnace[3]→UI[3]
    const int slotMap[4] = { 0, 2, 1, 3 };  // Map UI op index to Furnace slot index

    // Per-operator
    for (int uiOp = 0; uiOp < 4; uiOp++) {
        int furnaceSlot = slotMap[uiOp];
        const auto& fop = ins.op[furnaceSlot];

        // tl: Keep same as Furnace (0=loud, 127=silent) - no inversion
        set(OP_TL_ID[uiOp],  float(fop.tl));
        set(OP_AR_ID[uiOp],  float(fop.ar));
        set(OP_DR_ID[uiOp],  float(fop.dr));
        set(OP_SR_ID[uiOp],  float(fop.d2r));   // d2r = sustain rate
        set(OP_SL_ID[uiOp],  float(fop.sl));
        set(OP_RR_ID[uiOp],  float(fop.rr));
        set(OP_MUL_ID[uiOp], float(fop.mult));
        set(OP_RS_ID[uiOp],  float(fop.rs));

        // dt chip(0-7) → UI(-3..+3)
        // Furnace displays detune as: displayValue = chipValue - 3
        // So: chip 0→display -3, chip 3→display 0, chip 5→display +2, chip 7→display +4 (capped at +3)
        int chipDT = int(fop.dt) & 7;
        int uiDT = chipDT - 3;
        if (uiDT > 3) uiDT = 3;   // Clamp to valid range
        if (uiDT < -3) uiDT = -3;
        set(OP_DT_ID[uiOp], float(uiDT));

        set(OP_AM_ID[uiOp], fop.am != 0 ? 1.0f : 0.0f);
        
        // SSG-EG: Furnace ssgEnv bit3=enable, bits2:0=mode
        // Map to dropdown: 0=Off, 1-8=modes 0-7
        bool ssgEn   = (fop.ssgEnv & 0x08) != 0;
        int ssgMode  = fop.ssgEnv & 0x07;
        int dropdownIdx = ssgEn ? (ssgMode + 1) : 0;
        set(OP_SSG_MODE_ID[uiOp], float(dropdownIdx));
    }

    pushParamsToVoices();
    return true;
}

bool ARM2612AudioProcessor::exportFurnaceInstrument(
    const juce::File& file, const juce::String& patchName)
{
    auto gi = [&](const juce::String& id) {
        return static_cast<int>(apvts.getRawParameterValue(id)->load());
    };

    FurnaceFormat::Instrument ins;
    // Use stored instrument name, fallback to provided name, fallback to filename
    ins.name = instrumentName.isEmpty() ? 
               (patchName.isEmpty() ? file.getFileNameWithoutExtension() : patchName) :
               instrumentName;
    
    // Debug logging
    DBG("=== EXPORT DEBUG ===");
    DBG("File name: " << file.getFileName());
    DBG("File name without extension: " << file.getFileNameWithoutExtension());
    DBG("instrumentName member: '" << instrumentName << "'");
    DBG("patchName parameter: '" << patchName << "'");
    DBG("Name being written to file: '" << ins.name << "'");
    
    ins.alg        = uint8_t(gi(GLOBAL_ALGORITHM) & 7);
    ins.fb         = uint8_t(gi(GLOBAL_FEEDBACK)  & 7);
    ins.fms        = uint8_t(gi(GLOBAL_FMS)       & 7);
    ins.ams        = uint8_t(gi(GLOBAL_AMS)       & 3);
    ins.fms2       = 0;
    ins.ams2       = 0;
    ins.ops        = 4;
    ins.opllPreset = 0;
    ins.block      = 0;

    // YM2612 operator slot mapping (reverse of import):
    // Our UI order: OP1, OP2, OP3, OP4 (indices 0, 1, 2, 3)
    // Furnace slot order: OP1, OP3, OP2, OP4 (slots 0, 1, 2, 3)
    // So we need to write: UI[0]→Furnace[0], UI[1]→Furnace[2], UI[2]→Furnace[1], UI[3]→Furnace[3]
    const int slotMap[4] = { 0, 2, 1, 3 };  // Map UI op index to Furnace slot index

    for (int uiOp = 0; uiOp < 4; uiOp++) {
        int furnaceSlot = slotMap[uiOp];
        FurnaceFormat::Op& fop = ins.op[furnaceSlot];
        fop.enable = true;

        // tl: Write directly as stored in parameter (0=loud, 127=silent)
        fop.tl   = uint8_t(gi(OP_TL_ID[uiOp]));

        fop.ar   = uint8_t(gi(OP_AR_ID[uiOp]));
        fop.dr   = uint8_t(gi(OP_DR_ID[uiOp]));
        fop.d2r  = uint8_t(gi(OP_SR_ID[uiOp]));   // sustain rate
        fop.sl   = uint8_t(gi(OP_SL_ID[uiOp]));
        fop.rr   = uint8_t(gi(OP_RR_ID[uiOp]));
        fop.mult = uint8_t(gi(OP_MUL_ID[uiOp]));
        fop.rs   = uint8_t(gi(OP_RS_ID[uiOp]));
        fop.am   = uint8_t(gi(OP_AM_ID[uiOp]));

        // dt: UI(-3..+3) → chip(0-7)
        // Furnace: chipValue = displayValue + 3
        int uiDT = gi(OP_DT_ID[uiOp]);
        fop.dt = uint8_t((uiDT + 3) & 7);  // Clamp to 0-7

        // SSG-EG: dropdown index 0=Off, 1-8=modes 0-7 → Furnace ssgEnv
        int dropdownIdx = gi(OP_SSG_MODE_ID[uiOp]);
        if (dropdownIdx == 0) {
            fop.ssgEnv = 0x00;  // disabled
        } else {
            int mode = dropdownIdx - 1;  // 1→0, 2→1, ..., 8→7
            fop.ssgEnv = uint8_t(0x08 | (mode & 0x07));  // enabled with mode
        }

        fop.kvs = 2;
        fop.dam = fop.dvb = fop.egt = fop.ksl = 0;
        fop.sus = fop.vib = fop.ws  = fop.ksr = fop.dt2 = 0;
    }

    return FurnaceFormat::writeFui(file, ins);
}
