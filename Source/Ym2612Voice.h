#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>
#include <cstring>

// ymfm: YM2612 (OPN2) from aaronsgiles/ymfm
#include "ymfm_opn.h"

#include "SynthSound.h"

// ─────────────────────────────────────────────────────────────────────────────
// PluginYmfmInterface
//
// ymfm chips require a host interface for timer callbacks and memory reads.
// We don't need timers for a synthesiser plugin (we drive the clock manually),
// so all methods are stubbed out.  The chip still works correctly without them.
// ─────────────────────────────────────────────────────────────────────────────
class PluginYmfmInterface : public ymfm::ymfm_interface
{
public:
    // ymfm calls these when the chip raises/clears its IRQ line – not needed.
    void ymfm_set_timer(uint32_t, int32_t) override {}
    void ymfm_sync_mode_write(uint8_t) override {}
    void ymfm_sync_check_interrupts() override {}
    void ymfm_set_busy_end(uint32_t) override {}
    // For chips that read external memory (ADPCM ROMs etc.) – YM2612 DAC only
    uint8_t ymfm_external_read(ymfm::access_class, uint32_t) override { return 0; }
    void ymfm_external_write(ymfm::access_class, uint32_t, uint8_t) override {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Ym2612Voice
//
// One JUCE SynthesiserVoice = one ymfm::ym2612 chip instance.
//
// ymfm design notes
// -----------------
// • ymfm chips are clock-driven, not sample-rate-driven.
//   ym2612::sample_rate(clock) returns the chip's own output rate (~53267 Hz
//   for the NTSC Mega Drive clock of 7 670 453 Hz).
// • We call chip.generate() once per chip sample, accumulate into a ring
//   buffer, then linear-interpolate to the DAW's sample rate in renderNextBlock.
// • Register writes:
//     Part 0 (channels 0-2):  write_address(reg)  + write_data(val)
//     Part 1 (channels 3-5):  write_address_hi(reg) + write_data_hi(val)
//   Key-on register 0x28 always goes to write_address(0x28)+write_data(val)
//   regardless of channel.
// • Output: ymfm::ym2612::output_data::data[0] = left, data[1] = right,
//   nominally ±(1<<15) per operator sum (scales with number of active ops).
//
// Parameters exposed
// ------------------
//   opLevel[0..3]  – Total Level for OP1-OP4 (0 = loud, 127 = silent)
//   Stored as TL directly; the processor inverts the UI "level" before passing.
// ─────────────────────────────────────────────────────────────────────────────
class Ym2612Voice : public juce::SynthesiserVoice
{
public:
    // Mega Drive NTSC master clock
    static constexpr uint32_t YM_CLOCK = 7'670'453;

    Ym2612Voice()
        : m_chip(m_interface)
    {
        // Sensible defaults: carriers loud, modulators half-open
        m_opTL[0] = 63;   // OP1 modulator
        m_opTL[1] = 63;   // OP3 modulator
        m_opTL[2] = 0;    // OP2 carrier (loud)
        m_opTL[3] = 0;    // OP4 carrier (loud)
    }

    // ── Parameter setter (called from processor, audio thread) ──────────────
    // tl: 0 = max output, 127 = silent  (raw YM2612 TL value)
    void setOpTL(int op, int tl)
    {
        jassert(op >= 0 && op < 4);
        m_opTL[op].store(juce::jlimit(0, 127, tl));
        m_dirtyTL.store(true);
    }

    // ── SynthesiserVoice ─────────────────────────────────────────────────────
    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SynthSound*>(s) != nullptr;
    }

    void startNote(int midiNote, float velocity,
                   juce::SynthesiserSound*, int /*pitchWheel*/) override
    {
        // Rebuild resampling state when the note starts
        initResamplingState();

        // Full chip reset then program the patch
        m_chip.reset();
        programPatch();
        setFrequency(juce::MidiMessage::getMidiNoteInHertz(midiNote));
        keyOn();

        m_velGain   = velocity;
        m_active    = true;
        m_releasing = false;
    }

    void stopNote(float /*velocity*/, bool allowTailOff) override
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

        // Push fresh TL values if the user moved a knob
        if (m_dirtyTL.exchange(false))
            writeTL();

        const int nch = output.getNumChannels();
        // ymfm output is nominally ±(1<<15) per operator contributing;
        // with algo 4 two carriers can sum, so scale by 1/(2 * 32768).
        const float scale = m_velGain / (2.0f * 32768.0f);

        for (int i = 0; i < numSamples; i++) {
            // Advance the resampler: generate chip samples until we have
            // enough to interpolate one DAW sample.
            while (m_resamplePos >= 1.0) {
                m_prevSampleL = m_currSampleL;
                m_prevSampleR = m_currSampleR;

                ymfm::ym2612::output_data out;
                m_chip.generate(&out);

                m_currSampleL = static_cast<float>(out.data[0]);
                m_currSampleR = static_cast<float>(out.data[1]);
                m_resamplePos -= 1.0;
            }

            // Linear interpolation between previous and current chip sample
            float t  = static_cast<float>(m_resamplePos);
            float sl = m_prevSampleL + t * (m_currSampleL - m_prevSampleL);
            float sr = m_prevSampleR + t * (m_currSampleR - m_prevSampleR);

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
    // ── ymfm objects ────────────────────────────────────────────────────────
    PluginYmfmInterface m_interface;
    ymfm::ym2612        m_chip;

    // ── Voice state ──────────────────────────────────────────────────────────
    bool  m_active    = false;
    bool  m_releasing = false;
    int   m_releaseTimer = 0;
    float m_velGain   = 1.0f;

    // ── Parameters ───────────────────────────────────────────────────────────
    std::atomic<int>  m_opTL[4];
    std::atomic<bool> m_dirtyTL { false };

    // ── Resampling state ─────────────────────────────────────────────────────
    // The chip runs at its own rate (~53267 Hz); the DAW wants getSampleRate().
    // m_resampleStep = chipRate / dawRate  (< 1 for 44.1/48 kHz DAW rates)
    double m_resampleStep = 1.0;
    double m_resamplePos  = 1.0;   // start at 1 so first call generates a sample
    float  m_prevSampleL = 0.0f, m_currSampleL = 0.0f;
    float  m_prevSampleR = 0.0f, m_currSampleR = 0.0f;

    void initResamplingState()
    {
        uint32_t chipRate   = m_chip.sample_rate(YM_CLOCK);
        m_resampleStep      = static_cast<double>(chipRate) / getSampleRate();
        m_resamplePos       = 1.0;   // trigger first chip generate immediately
        m_prevSampleL = m_prevSampleR = 0.0f;
        m_currSampleL = m_currSampleR = 0.0f;
    }

    // ── Register write helpers ────────────────────────────────────────────────
    // Part 0 = channels 0-2 (write_address / write_data)
    // Part 1 = channels 3-5 (write_address_hi / write_data_hi)
    void wr0(uint8_t reg, uint8_t val)
    {
        m_chip.write_address(reg);
        m_chip.write_data(val);
    }
    void wr1(uint8_t reg, uint8_t val)
    {
        m_chip.write_address_hi(reg);
        m_chip.write_data_hi(val);
    }

    // ── Patch programming ─────────────────────────────────────────────────────
    // We use channel 0 (part 0) for all voices.
    //
    // YM2612 operator slot layout within one channel:
    //   Register offset  +0 = OP1,  +4 = OP3,  +8 = OP2,  +12 = OP4
    // (This is the hardware numbering; the order in the register map is
    //  OP1, OP3, OP2, OP4 — *not* OP1..OP4 sequentially.)
    //
    // Algorithm 4:  (OP1→OP2) + (OP3→OP4)  — both pairs contribute output.
    //   OP1 and OP3 are modulators; OP2 and OP4 are carriers.
    //   Feedback on OP1 adds harmonic richness.
    //
    // Register format quick-ref:
    //   0x30  DT[6:4] MUL[3:0]          0x50  KS[7:6] AR[4:0]
    //   0x60  AM[7]   DR[4:0]           0x70          SR[4:0]
    //   0x80  SL[7:4] RR[3:0]           0x40          TL[6:0]
    //   0xB0  FB[5:3] ALG[2:0]          0xA4  BLK[5:3] FN[10:8]
    //   0xA0  FN[7:0]                   0x28  keyon
    void programPatch()
    {
        // Algorithm 4, Feedback 5 on OP1
        wr0(0xB0, (5 << 3) | 4);
        // Both L+R output, no LFO AM/PM sensitivity
        wr0(0xB4, 0xC0);

        // ┌────────┬──────────┬─────┬─────┬──────┬───────┐
        // │  Slot  │ DT│MUL   │ AR  │ DR  │ SR   │ SL│RR │
        // ├────────┼──────────┼─────┼─────┼──────┼───────┤
        // │ OP1 +0 │ DT1,MUL1 │ 31  │  5  │  0   │ 1│10  │  modulator
        // │ OP3 +4 │ DT0,MUL1 │ 31  │  5  │  0   │ 1│10  │  modulator
        // │ OP2 +8 │ DT0,MUL1 │ 31  │  5  │  0   │ 1│10  │  carrier
        // │ OP4+12 │ DT0,MUL1 │ 31  │  5  │  0   │ 1│10  │  carrier
        // └────────┴──────────┴─────┴─────┴──────┴───────┘
        //  DT=1→0x10, MUL=1→0x01  combined: 0x11 for OP1, 0x01 for others
        //  AR=31→0x1F,  KS=0→0x00   combined: 0x1F
        //  DR= 5→0x05
        //  SR= 0→0x00
        //  SL=1 (–3 dB),RR=10:  SL<<4 | RR = 0x1A

        struct SlotPatch { uint8_t dtmul, ksar, dr, sr, slrr; };
        static const SlotPatch kPatch[4] = {
            { 0x11, 0x1F, 0x05, 0x00, 0x1A },   // OP1 (+0)
            { 0x01, 0x1F, 0x05, 0x00, 0x1A },   // OP3 (+4)
            { 0x01, 0x1F, 0x05, 0x00, 0x1A },   // OP2 (+8)
            { 0x01, 0x1F, 0x05, 0x00, 0x1A },   // OP4 (+12)
        };
        const uint8_t offsets[4] = { 0, 4, 8, 12 };

        for (int op = 0; op < 4; op++) {
            uint8_t o = offsets[op];
            wr0(0x30 + o, kPatch[op].dtmul);
            wr0(0x50 + o, kPatch[op].ksar);
            wr0(0x60 + o, kPatch[op].dr);
            wr0(0x70 + o, kPatch[op].sr);
            wr0(0x80 + o, kPatch[op].slrr);
        }
        writeTL();
    }

    void writeTL()
    {
        // TL register offsets match the slot offsets: OP1=+0, OP3=+4, OP2=+8, OP4=+12
        // Our m_opTL array is indexed as the user sees them: [0]=OP1, [1]=OP3, [2]=OP2, [3]=OP4
        const uint8_t offsets[4] = { 0, 4, 8, 12 };
        for (int op = 0; op < 4; op++)
            wr0(0x40 + offsets[op], static_cast<uint8_t>(m_opTL[op].load()));
    }

    // ── Frequency ─────────────────────────────────────────────────────────────
    // F-number formula:  Fn = freq_hz * 2^(20 - block) / (fref)
    // where fref = YM_CLOCK / 144  (≈ 53267 Hz)
    void setFrequency(double hz)
    {
        const double fref = static_cast<double>(YM_CLOCK) / 144.0;
        int    block = 4;
        double fn    = hz * static_cast<double>(1 << (20 - block)) / fref;

        // Shift block so fn lands in [0x200, 0x7FF]
        while (fn > 0x7FF && block < 7) { block++; fn /= 2.0; }
        while (fn < 0x200 && block > 0) { block--; fn *= 2.0; }

        auto fnum = static_cast<uint16_t>(juce::jlimit(0, 0x7FF, static_cast<int>(fn)));

        // Write high byte (latch) then low byte (applies update)
        wr0(0xA4, static_cast<uint8_t>(((block & 7) << 3) | ((fnum >> 8) & 0x07)));
        wr0(0xA0, static_cast<uint8_t>(fnum & 0xFF));
    }

    // ── Key on/off ────────────────────────────────────────────────────────────
    // Register 0x28:  bits[7:4] = operator mask (0xF = all four on)
    //                 bits[2:0] = channel index (0-2 part0, 4-6 part1)
    void keyOn()
    {
        wr0(0x28, 0xF0 | 0x00);   // all OPs on, channel 0
    }
    void keyOff()
    {
        wr0(0x28, 0x00 | 0x00);   // all OPs off, channel 0
    }
};
