#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>
#include <cstring>

#include "ymfm_opn.h"
#include "SynthSound.h"

// ─────────────────────────────────────────────────────────────────────────────
// PluginYmfmInterface  –  stub timer / IRQ callbacks (not needed for a synth)
// ─────────────────────────────────────────────────────────────────────────────
class PluginYmfmInterface : public ymfm::ymfm_interface
{
public:
    void    ymfm_set_timer(uint32_t, int32_t)                 override {}
    void    ymfm_sync_mode_write(uint8_t)                     override {}
    void    ymfm_sync_check_interrupts()                      override {}
    void    ymfm_set_busy_end(uint32_t)                       override {}
    uint8_t ymfm_external_read(ymfm::access_class, uint32_t)  override { return 0; }
    void    ymfm_external_write(ymfm::access_class, uint32_t, uint8_t) override {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Ym2612Voice
//
// One JUCE SynthesiserVoice = one ymfm::ym2612 chip instance (channel 0).
//
// All 8 per-operator parameters are stored as plain-struct copies so the
// processor can push them from any thread with a single struct assignment.
// A dirty flag triggers a full register re-write on the next audio block.
// ─────────────────────────────────────────────────────────────────────────────
class Ym2612Voice : public juce::SynthesiserVoice
{
public:
    static constexpr uint32_t YM_CLOCK = 7'670'453;   // NTSC Mega Drive

    // ── Per-operator parameter block ─────────────────────────────────────────
    // All values are the *raw chip register values* (not UI-friendly inversions).
    struct OpParams {
        int tl  = 0;    // total level  0-127  (0=loud, 127=silent)
        int ar  = 31;   // attack rate  0-31
        int dr  = 5;    // decay rate   0-31
        int sr  = 0;    // sustain rate 0-31
        int sl  = 1;    // sustain level 0-15
        int rr  = 10;   // release rate  0-15
        int mul = 1;    // frequency multiply 0-15  (0 = ×0.5)
        int dt  = 0;    // detune 0-7  (4-7 = negative on chip)
    };

    Ym2612Voice()
        : m_chip(m_interface)
    {
        // Algo 4 defaults: carriers (OP2, OP4) loud; modulators (OP1, OP3) half-open
        m_params[0].tl = 63;   // OP1 modulator
        m_params[1].tl = 0;    // OP2 carrier
        m_params[2].tl = 63;   // OP3 modulator
        m_params[3].tl = 0;    // OP4 carrier
    }

    // Push a full parameter block for one operator (called from audio thread)
    void setOpParams(int op, const OpParams& p)
    {
        jassert(op >= 0 && op < 4);
        m_params[op] = p;
        m_dirty.store(true);
    }

    // ── SynthesiserVoice ─────────────────────────────────────────────────────
    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SynthSound*>(s) != nullptr;
    }

    void startNote(int midiNote, float velocity,
                   juce::SynthesiserSound*, int) override
    {
        initResamplingState();
        m_chip.reset();
        programPatch();
        setFrequency(juce::MidiMessage::getMidiNoteInHertz(midiNote));
        keyOn();
        m_velGain   = velocity;
        m_active    = true;
        m_releasing = false;
    }

    void stopNote(float, bool allowTailOff) override
    {
        keyOff();
        if (allowTailOff) {
            m_releasing    = true;
            m_releaseTimer = static_cast<int>(getSampleRate() * 0.4);
        } else {
            clearCurrentNote();
            m_active = false;
        }
    }

    void renderNextBlock(juce::AudioBuffer<float>& output,
                         int startSample, int numSamples) override
    {
        if (!m_active) return;

        if (m_dirty.exchange(false))
            writeAllRegisters();

        const int   nch   = output.getNumChannels();
        const float scale = m_velGain / (2.0f * 32768.0f);

        for (int i = 0; i < numSamples; i++) {
            while (m_resamplePos >= 1.0) {
                m_prevL = m_currL;  m_prevR = m_currR;
                ymfm::ym2612::output_data out;
                m_chip.generate(&out);
                m_currL = static_cast<float>(out.data[0]);
                m_currR = static_cast<float>(out.data[1]);
                m_resamplePos -= 1.0;
            }
            float t  = static_cast<float>(m_resamplePos);
            float sl = m_prevL + t * (m_currL - m_prevL);
            float sr = m_prevR + t * (m_currR - m_prevR);

            if (nch > 0) output.addSample(0, startSample + i, sl * scale);
            if (nch > 1) output.addSample(1, startSample + i, sr * scale);
            m_resamplePos += m_resampleStep;
        }

        if (m_releasing) {
            m_releaseTimer -= numSamples;
            if (m_releaseTimer <= 0) {
                clearCurrentNote();
                m_active = m_releasing = false;
            }
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

private:
    PluginYmfmInterface m_interface;
    ymfm::ym2612        m_chip;

    bool  m_active       = false;
    bool  m_releasing    = false;
    int   m_releaseTimer = 0;
    float m_velGain      = 1.0f;

    // Parameter storage – written by processor, read by audio thread
    OpParams           m_params[4];
    std::atomic<bool>  m_dirty { false };

    // Resampler
    double m_resampleStep = 1.0;
    double m_resamplePos  = 1.0;
    float  m_prevL = 0, m_currL = 0;
    float  m_prevR = 0, m_currR = 0;

    void initResamplingState()
    {
        uint32_t chipRate = m_chip.sample_rate(YM_CLOCK);
        m_resampleStep    = static_cast<double>(chipRate) / getSampleRate();
        m_resamplePos     = 1.0;
        m_prevL = m_currL = m_prevR = m_currR = 0.0f;
    }

    // ── Register write helpers ────────────────────────────────────────────────
    void wr(uint8_t reg, uint8_t val)
    {
        m_chip.write_address(reg);
        m_chip.write_data(val);
    }

    // ── Full register programming ─────────────────────────────────────────────
    // YM2612 slot register offsets within ch0 (part 0):
    //   OP1=+0, OP3=+4, OP2=+8, OP4=+12  (hardware numbering)
    // Our m_params[] is indexed as the user sees: [0]=OP1, [1]=OP2, [2]=OP3, [3]=OP4
    // so we map:  param[0]→reg+0,  param[1]→reg+8,  param[2]→reg+4,  param[3]→reg+12
    static constexpr uint8_t kSlotOff[4] = { 0, 8, 4, 12 };

    void writeAllRegisters()
    {
        // Algorithm 4 ((OP1→OP2)+(OP3→OP4)), Feedback 5 on OP1
        wr(0xB0, (5 << 3) | 4);
        wr(0xB4, 0xC0);   // L+R, no LFO AM/PM

        for (int p = 0; p < 4; p++) {
            uint8_t o         = kSlotOff[p];
            const OpParams& q = m_params[p];

            uint8_t dtmul = static_cast<uint8_t>(((q.dt  & 7) << 4) | (q.mul & 0x0F));
            uint8_t ksar  = static_cast<uint8_t>(q.ar  & 0x1F);          // KS=0
            uint8_t dr    = static_cast<uint8_t>(q.dr  & 0x1F);
            uint8_t sr    = static_cast<uint8_t>(q.sr  & 0x1F);
            uint8_t slrr  = static_cast<uint8_t>(((q.sl & 0x0F) << 4) | (q.rr & 0x0F));
            uint8_t tl    = static_cast<uint8_t>(q.tl  & 0x7F);

            wr(0x30 + o, dtmul);
            wr(0x40 + o, tl);
            wr(0x50 + o, ksar);
            wr(0x60 + o, dr);
            wr(0x70 + o, sr);
            wr(0x80 + o, slrr);
        }
    }

    void programPatch()
    {
        writeAllRegisters();
    }

    // ── Frequency ─────────────────────────────────────────────────────────────
    void setFrequency(double hz)
    {
        const double fref = static_cast<double>(YM_CLOCK) / 144.0;
        int    block = 4;
        double fn    = hz * static_cast<double>(1 << (20 - block)) / fref;
        while (fn > 0x7FF && block < 7) { block++; fn /= 2.0; }
        while (fn < 0x200 && block > 0) { block--; fn *= 2.0; }
        auto fnum = static_cast<uint16_t>(juce::jlimit(0, 0x7FF, static_cast<int>(fn)));
        wr(0xA4, static_cast<uint8_t>(((block & 7) << 3) | ((fnum >> 8) & 0x07)));
        wr(0xA0, static_cast<uint8_t>(fnum & 0xFF));
    }

    // ── Key on/off ────────────────────────────────────────────────────────────
    void keyOn()  { wr(0x28, 0xF0); }
    void keyOff() { wr(0x28, 0x00); }
};
