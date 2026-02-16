#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>

extern "C" {
#include "mame/mame_ym2612fm.h"
}

#include "SynthSound.h"

/**
 * Ym2612Voice
 * -----------
 * One JUCE SynthesiserVoice = one YM2612 chip instance, driving channel 0.
 *
 * Register write convention for the new emulator core:
 *   ym2612_write(chip, 0, reg)  -> address latch, part 0 (channels 0-2)
 *   ym2612_write(chip, 1, val)  -> data write,    part 0
 *   ym2612_write(chip, 2, reg)  -> address latch, part 1 (channels 3-5)
 *   ym2612_write(chip, 3, val)  -> data write,    part 1
 *
 * Key-on register 0x28 (always part 0):
 *   bits [7:4] = OP mask (0xF0 = all 4 on)
 *   bits [2:0] = channel (0-2 for part0, 4-6 for part1)
 *
 * Parameters:
 *   opLevel[0..3] – TL for each operator (0=loud, 127=silent)
 *   Exposed as "level" in the editor (0=silent, 127=loud), inverted here.
 */
class Ym2612Voice : public juce::SynthesiserVoice
{
public:
    static constexpr int YM_CLOCK  = 7670453;  // NTSC Mega Drive
    static constexpr int FM_CH_IDX = 0;        // use chip channel 0 (part 0)

    Ym2612Voice()
    {
        // In algo 4: slots 0,1 (OP1,OP3) are modulators; 2,3 (OP2,OP4) are carriers.
        // TL param: 0=silent (TL=127), 127=loud (TL=0).
        // Carriers default loud, modulators half-open for FM colour.
        opLevel[0].store(64);   // OP1 modulator
        opLevel[1].store(64);   // OP3 modulator
        opLevel[2].store(127);  // OP2 carrier – loud
        opLevel[3].store(127);  // OP4 carrier – loud
    }

    ~Ym2612Voice() override
    {
        if (chip) { ym2612_shutdown(chip); chip = nullptr; }
    }

    /* Called by processor every block for each voice */
    void setOpLevel(int op, int tl127)
    {
        jassert(op >= 0 && op < 4);
        opLevel[op].store(juce::jlimit(0, 127, tl127));
        dirtyParams.store(true);
    }

    bool canPlaySound(juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SynthSound*>(s) != nullptr;
    }

    void startNote(int midiNote, float velocity,
                   juce::SynthesiserSound*, int) override
    {
        ensureChipReady();
        ym2612_reset_chip(chip);
        programPatch();
        setFrequency(juce::MidiMessage::getMidiNoteInHertz(midiNote));
        keyOn();
        velGain  = velocity * 0.5f;
        active   = true;
        releasing = false;
    }

    void stopNote(float, bool allowTailOff) override
    {
        keyOff();
        if (allowTailOff) {
            releasing    = true;
            releaseTimer = static_cast<int>(getSampleRate() * 0.35);
        } else {
            clearCurrentNote();
            active = false;
        }
    }

    void renderNextBlock(juce::AudioBuffer<float>& output,
                         int startSample, int numSamples) override
    {
        if (!active || !chip) return;

        if (dirtyParams.exchange(false))
            writeTL();

        if ((int)bufL.size() < numSamples) {
            bufL.assign(numSamples, 0);
            bufR.assign(numSamples, 0);
        }
        std::fill(bufL.begin(), bufL.begin() + numSamples, 0);
        std::fill(bufR.begin(), bufR.begin() + numSamples, 0);

        ym2612_update_one(chip, (UINT32)numSamples, bufL.data(), bufR.data());

        const float scale = velGain / 32768.0f;
        const int   nch   = output.getNumChannels();
        for (int i = 0; i < numSamples; i++) {
            float l = juce::jlimit(-1.0f, 1.0f, (float)bufL[i] * scale);
            float r = juce::jlimit(-1.0f, 1.0f, (float)bufR[i] * scale);
            if (nch > 0) output.addSample(0, startSample + i, l);
            if (nch > 1) output.addSample(1, startSample + i, r);
        }

        if (releasing) {
            releaseTimer -= numSamples;
            if (releaseTimer <= 0) {
                clearCurrentNote();
                active = releasing = false;
            }
        }
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

private:
    void* chip = nullptr;
    bool  active = false, releasing = false;
    int   releaseTimer = 0;
    float velGain = 1.0f;

    std::atomic<int>  opLevel[4];
    std::atomic<bool> dirtyParams { true };

    std::vector<INT32> bufL, bufR;

    void ensureChipReady()
    {
        if (chip) return;
        chip = ym2612_init(nullptr, YM_CLOCK,
                           (int)getSampleRate(), nullptr, nullptr);
        jassert(chip != nullptr);
    }

    /* Write address then data to part 0 (channels 0-2) */
    void wr(UINT8 reg, UINT8 val)
    {
        ym2612_write(chip, 0, reg);   /* address latch part 0 */
        ym2612_write(chip, 1, val);   /* data */
    }

    /* ── Patch ────────────────────────────────────────────────────────────
     * Algorithm 4: (OP1->OP2) + (OP3->OP4) = OP2 + OP4 summed to output.
     * A warm analogue-style FM lead similar to the Mega Drive's common patch.
     *
     * YM2612 slot register offsets within a channel (ch=0 so base=0):
     *   OP1 = +0, OP3 = +4, OP2 = +8, OP4 = +12  (hardware numbering)
     *
     * DT/MUL format: bits[6:4]=DT(0-7), bits[3:0]=MUL(0-15)
     * AR/KS  format: bits[7:6]=KS,      bits[4:0]=AR
     * DR     format: bits[4:0]=DR
     * SR     format: bits[4:0]=SR
     * SL/RR  format: bits[7:4]=SL,      bits[3:0]=RR
     */
    void programPatch()
    {
        /* Algorithm 4: (OP1->OP2) + (OP3->OP4), summed at output.
         * OP2 and OP4 are carriers; OP1 and OP3 are modulators.
         * Feedback 5 on OP1 adds a metallic edge.
         *
         * Register offsets within channel 0 (part 0):
         *   OP1=+0, OP3=+4, OP2=+8, OP4=+12  (YM2612 hardware order)
         *
         * Byte formats:
         *   0x30  DT[6:4] | MUL[3:0]
         *   0x50  KS[7:6] | AR[4:0]
         *   0x60  AM[7]   | DR[4:0]
         *   0x70           SR[4:0]
         *   0x80  SL[7:4] | RR[3:0]
         *   0x40  TL[6:0]  (0=loud, 127=silent)
         */
        wr(0xB0, (5 << 3) | 4);   /* FB=5, Algo=4 */
        wr(0xB4, 0xC0);            /* L+R output, no LFO AM/PM */

        /*                   DT|MUL   KS|AR   DR      SR    SL|RR  */
        /* OP1 (modulator) */ wr(0x30, 0x71); wr(0x50, 0x1F); wr(0x60, 0x05); wr(0x70, 0x00); wr(0x80, 0x18);
        /* OP3 (modulator) */ wr(0x34, 0x0D); wr(0x54, 0x1F); wr(0x64, 0x05); wr(0x74, 0x00); wr(0x84, 0x18);
        /* OP2 (carrier)   */ wr(0x38, 0x01); wr(0x58, 0x1F); wr(0x68, 0x05); wr(0x78, 0x00); wr(0x88, 0x18);
        /* OP4 (carrier)   */ wr(0x3C, 0x01); wr(0x5C, 0x1F); wr(0x6C, 0x05); wr(0x7C, 0x00); wr(0x8C, 0x18);

        /* Write TL from params; until the processor pushes real values,
         * default the carriers (OP2/OP4) to TL=0 (loudest) so you hear
         * something immediately, and modulators to TL=63 (half-open).       */
        writeTL();
    }

    /* Write TL for all 4 operators from the atomic opLevel params */
    void writeTL()
    {
        const UINT8 offsets[4] = { 0, 4, 8, 12 };
        for (int op = 0; op < 4; op++)
            wr(0x40 + offsets[op], (UINT8)opLevel[op].load());
    }

    /* ── Frequency ─────────────────────────────────────────────────────── */
    void setFrequency(double hz)
    {
        /* F-number = freq * 2^(20-block) / (fref)
         * fref = YM_CLOCK / 144                                             */
        const double fref = (double)YM_CLOCK / 144.0;
        int block = 4;
        double fn = hz * (double)(1 << (20 - block)) / fref;
        /* Clamp into [0x200, 0x7FF] by adjusting block */
        while (fn > 0x7FF && block < 7) { block++; fn /= 2.0; }
        while (fn < 0x200 && block > 0) { block--; fn *= 2.0; }

        UINT16 fnum = (UINT16)juce::jlimit(0, 0x7FF, (int)fn);

        /* Write F-number high byte (latch) then low byte (triggers update) */
        wr(0xA4, (UINT8)(((block & 7) << 3) | ((fnum >> 8) & 0x07)));
        wr(0xA0, (UINT8)(fnum & 0xFF));
    }

    /* ── Key on / off ─────────────────────────────────────────────────── */
    void keyOn()
    {
        /* reg 0x28: bits[7:4]=OP mask (0xF=all on), bits[2:0]=channel      */
        wr(0x28, (UINT8)(0xF0 | FM_CH_IDX));
    }

    void keyOff()
    {
        if (chip) wr(0x28, (UINT8)(0x00 | FM_CH_IDX));
    }
};
