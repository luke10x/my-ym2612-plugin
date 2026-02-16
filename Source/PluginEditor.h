#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Shared colour palette (used by editor + envelope display)
// ─────────────────────────────────────────────────────────────────────────────
namespace YmColors {
    static const juce::Colour bg     { 0xFF0D0D1A };
    static const juce::Colour panel  { 0xFF161625 };
    static const juce::Colour border { 0xFF252540 };
    static const juce::Colour accent { 0xFF00D4AA };   // teal carrier
    static const juce::Colour mod    { 0xFF5599FF };   // blue modulator
    static const juce::Colour text   { 0xFFDDEEFF };
    static const juce::Colour dim    { 0xFF556070 };
    static const juce::Colour envLine{ 0xFF00FFB0 };   // bright envelope trace
}

// ─────────────────────────────────────────────────────────────────────────────
// EnvelopeDisplay
//
// Draws a classic 5-segment FM ADSR shape:
//   [0→peak] Attack ramp  (speed = AR 0-31, fast = steep)
//   [peak→SL] Decay fall  (speed = DR)
//   [SL hold] Sustain     (level = SL 0-15, held while key down)
//   [SL→0]  Sus.Rate fade (speed = SR, can be 0 = hold forever)
//   [→0]    Release       (speed = RR, after key off)
//
// All parameters drive the visual shape directly – not accurate chip timing,
// but perceptually proportional so tweaking feels intuitive.
// ─────────────────────────────────────────────────────────────────────────────
class EnvelopeDisplay : public juce::Component
{
public:
    // Pointers to the live parameter values – updated each repaint via timer
    void setParams(juce::RangedAudioParameter* ar,
                   juce::RangedAudioParameter* dr,
                   juce::RangedAudioParameter* sl,
                   juce::RangedAudioParameter* sr,
                   juce::RangedAudioParameter* rr,
                   bool isCarrier)
    {
        pAR = ar; pDR = dr; pSL = sl; pSR = sr; pRR = rr;
        m_isCarrier = isCarrier;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        float w = bounds.getWidth();
        float h = bounds.getHeight();
        float x0 = bounds.getX();
        float y0 = bounds.getY();

        // Background
        g.setColour(YmColors::panel.darker(0.3f));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(YmColors::border);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        if (!pAR) return;

        // Read normalised values
        float ar = pAR->getValue();   // 0-1, 1=fastest
        float dr = pDR->getValue();
        float sl = 1.0f - pSL->getValue();  // SL=0→top, SL=15→bottom; invert for display
        float sr = pSR->getValue();
        float rr = pRR->getValue();

        // Convert rates to horizontal widths (faster = narrower)
        // We divide the total width into 5 segments
        // Minimum width so even rate=0 shows something
        auto rateToW = [](float rate, float maxW) -> float {
            // rate 0→1: 1 = instant (very narrow), 0 = very slow (wide)
            return maxW * (1.0f - rate * 0.85f) + maxW * 0.05f;
        };
        float totalW    = w;
        float wAtk      = rateToW(ar, totalW * 0.18f);
        float wDec      = rateToW(dr, totalW * 0.20f);
        float wSus      = totalW * 0.28f;                    // fixed sustain hold width
        float wSusRate  = sr > 0.01f ? rateToW(sr, totalW * 0.18f) : totalW * 0.18f;
        float wRel      = rateToW(rr, totalW * 0.16f);
        // normalise to fit exactly
        float total = wAtk + wDec + wSus + wSusRate + wRel;
        float scale = totalW / total;
        wAtk *= scale; wDec *= scale; wSus *= scale; wSusRate *= scale; wRel *= scale;

        // Y positions (0 = top = loud, h = bottom = silent)
        float yTop  = y0 + 4.0f;
        float yBot  = y0 + h - 4.0f;
        float ySL   = yTop + (yBot - yTop) * (1.0f - sl);   // sustain level

        // Build path
        juce::Path p;
        float cx = x0;
        p.startNewSubPath(cx, yBot);                          // start at silence
        cx += wAtk;  p.lineTo(cx, yTop);                     // attack to peak
        cx += wDec;  p.lineTo(cx, ySL);                      // decay to SL
        float susEnd = cx + wSus;
        p.lineTo(susEnd, ySL);                                // sustain hold
        cx = susEnd;
        float srEnd = cx + wSusRate;
        if (sr > 0.01f)
            p.lineTo(srEnd, yBot);                            // sustain rate fade
        else
            p.lineTo(srEnd, ySL);                             // hold at SL if SR=0
        cx = srEnd;
        p.lineTo(cx + wRel, yBot);                            // release to silence

        // Fill under curve
        juce::Path fill = p;
        fill.lineTo(cx + wRel, yBot);
        fill.lineTo(x0, yBot);
        fill.closeSubPath();
        juce::Colour lineCol = m_isCarrier ? YmColors::accent : YmColors::mod;
        g.setColour(lineCol.withAlpha(0.18f));
        g.fillPath(fill);

        // Stroke the curve
        g.setColour(lineCol.withAlpha(0.9f));
        g.strokePath(p, juce::PathStrokeType(1.8f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Sustain level dotted line
        g.setColour(lineCol.withAlpha(0.3f));
        float dashLen = 3.0f, gapLen = 3.0f, xd = x0;
        while (xd < x0 + w) {
            g.drawHorizontalLine(static_cast<int>(ySL), xd, xd + dashLen);
            xd += dashLen + gapLen;
        }

        // Label "EG" top-left
        g.setColour(YmColors::dim);
        g.setFont(juce::Font(8.5f));
        g.drawText("EG", bounds.translated(4, 2), juce::Justification::topLeft, false);
    }

private:
    juce::RangedAudioParameter *pAR=nullptr, *pDR=nullptr,
                                *pSL=nullptr, *pSR=nullptr, *pRR=nullptr;
    bool m_isCarrier = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// SquareWaveSynthAudioProcessorEditor
//
// Layout (one column per operator, side by side):
//
//  ┌─ Title ──────────────────────────────────────────────────────────────────┐
//  ├─ [OP1 col]──[OP2 col]──[OP3 col]──[OP4 col] ─────────────────────────── ┤
//  │   name+role  name+role  name+role  name+role                             │
//  │   [EnvDisp]  [EnvDisp]  [EnvDisp]  [EnvDisp]                            │
//  │   Level────  Level────  Level────  Level────                             │
//  │   Attack───  Attack───  Attack───  Attack───                             │
//  │   Decay────  Decay────  Decay────  Decay────                             │
//  │   Sus.Rate─  Sus.Rate─  Sus.Rate─  Sus.Rate─                            │
//  │   Sus.Lvl──  Sus.Lvl──  Sus.Lvl──  Sus.Lvl──                            │
//  │   Release──  Release──  Release──  Release──                             │
//  │   Multi────  Multi────  Multi────  Multi────                             │
//  │   Detune───  Detune───  Detune───  Detune───                             │
//  ├─ MIDI Keyboard ──────────────────────────────────────────────────────────┤
//  └──────────────────────────────────────────────────────────────────────────┘
// ─────────────────────────────────────────────────────────────────────────────
class SquareWaveSynthAudioProcessorEditor
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit SquareWaveSynthAudioProcessorEditor(SquareWaveSynthAudioProcessor&);
    ~SquareWaveSynthAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SquareWaveSynthAudioProcessor& audioProcessor;

    // ── Per-operator column ───────────────────────────────────────────────────
    struct SliderRow {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    static constexpr int NUM_SLIDERS = 8;  // TL, AR, DR, SR, SL, RR, MUL, DT
    static constexpr const char* SLIDER_LABELS[NUM_SLIDERS] = {
        "Level", "Attack", "Decay", "Sus.Rate", "Sus.Lvl", "Release", "Multi", "Detune"
    };

    struct OpColumn {
        juce::Label      nameLabel;
        juce::Label      roleLabel;
        EnvelopeDisplay  envDisplay;
        SliderRow        rows[NUM_SLIDERS];
    };
    OpColumn ops[4];

    // ── MIDI keyboard ─────────────────────────────────────────────────────────
    juce::MidiKeyboardComponent midiKeyboard;

    // ── Timer: repaint envelope displays ─────────────────────────────────────
    void timerCallback() override
    {
        for (auto& op : ops)
            op.envDisplay.repaint();
        midiKeyboard.repaint();
    }

    void setupSlider(SliderRow& row, const juce::String& paramId,
                     int maxVal, juce::Colour colour);
    void styleColumn(OpColumn& col, int opIdx);

    static constexpr int kTitleH    = 46;
    static constexpr int kHeaderH   = 36;   // name + role labels
    static constexpr int kEnvH      = 60;   // envelope display height
    static constexpr int kSliderH   = 44;   // height per slider row
    static constexpr int kKeyboardH = 80;
    static constexpr int kMargin    = 8;
    static constexpr int kPad       = 6;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SquareWaveSynthAudioProcessorEditor)
};
