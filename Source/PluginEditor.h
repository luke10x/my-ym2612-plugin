#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
namespace YmColors {
    static const juce::Colour bg     { 0xFF0D0D1A };
    static const juce::Colour panel  { 0xFF161625 };
    static const juce::Colour border { 0xFF252540 };
    static const juce::Colour accent { 0xFF00D4AA };
    static const juce::Colour mod    { 0xFF5599FF };
    static const juce::Colour text   { 0xFFDDEEFF };
    static const juce::Colour dim    { 0xFF556070 };
}

// ─────────────────────────────────────────────────────────────────────────────
// EnvelopeDisplay
//
// User-friendly visual envelope (not chip-accurate, optimized for readability):
//
//   1. Attack:  ramp up (faster AR = steeper)
//   2. Decay:   fall to sustain level (faster DR = steeper)
//   3. Sustain: horizontal line at SL
//      → LENGTH of this line reflects "Sustain Decay" (SR parameter)
//      → LONGER line = note rings longer (low SR value)
//      → SHORTER line = note fades faster (high SR value)
//   4. Release: diagonal fall from end of sustain line to bottom
//      → LONGER diagonal = rings longer after key release (low RR)
//      → SHORTER diagonal = quick cutoff (high RR)
//
// This makes it intuitive: move SR right → sustain line gets shorter (fades faster)
//                          move RR right → release diagonal gets shorter (quick release)
// ─────────────────────────────────────────────────────────────────────────────
class EnvelopeDisplay : public juce::Component
{
public:
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

        g.setColour(YmColors::panel.darker(0.3f));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(YmColors::border);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        if (!pAR) return;

        // Read normalized values (0..1)
        float ar = pAR->getValue();   // 1 = fast attack
        float dr = pDR->getValue();   // 1 = fast decay
        float sl = 1.0f - pSL->getValue();  // flip so high SL = high on display
        float sr = pSR->getValue();   // sustain decay rate: 1 = fast decay (short line)
        float rr = pRR->getValue();   // release rate: 1 = fast release (short diagonal)

        float yTop = y0 + 4.0f;
        float yBot = y0 + h - 4.0f;
        float ySL  = yTop + (yBot - yTop) * (1.0f - sl);

        // Width allocation (simplified for readability):
        // Attack takes proportional space (faster = narrower)
        // Decay takes proportional space
        // Sustain line length INVERSELY proportional to SR (high SR = short line)
        // Release diagonal length INVERSELY proportional to RR (high RR = short)

        auto rateToWidth = [](float rate, float baseWidth) -> float {
            // rate 0→1: slow→fast
            // return: slow = wide, fast = narrow
            return baseWidth * (1.0f - rate * 0.7f);
        };

        float wAtk = rateToWidth(ar, w * 0.15f);
        float wDec = rateToWidth(dr, w * 0.15f);
        // Sustain line: longer when SR is low (slow decay = rings longer)
        float wSus = w * 0.35f * (1.0f - sr * 0.8f);  // low SR = long line
        // Release: longer when RR is low (slow release = rings longer)
        float wRel = w * 0.25f * (1.0f - rr * 0.7f);  // low RR = long diagonal

        // Normalize to fit total width
        float totalUsed = wAtk + wDec + wSus + wRel;
        if (totalUsed > 0.0f) {
            float scale = w / totalUsed;
            wAtk *= scale; wDec *= scale; wSus *= scale; wRel *= scale;
        }

        // Build path
        juce::Path p;
        float cx = x0;
        p.startNewSubPath(cx, yBot);      // start at silence
        cx += wAtk;
        p.lineTo(cx, yTop);               // attack to peak
        cx += wDec;
        p.lineTo(cx, ySL);                // decay to sustain level
        float susEnd = cx + wSus;
        p.lineTo(susEnd, ySL);            // sustain hold (horizontal line)
        cx = susEnd;
        p.lineTo(cx + wRel, yBot);        // release diagonal to silence

        // Fill
        juce::Path fill = p;
        fill.lineTo(cx + wRel, yBot);
        fill.lineTo(x0, yBot);
        fill.closeSubPath();
        juce::Colour lineCol = m_isCarrier ? YmColors::accent : YmColors::mod;
        g.setColour(lineCol.withAlpha(0.18f));
        g.fillPath(fill);

        // Stroke
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

        // Label
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

    struct SliderRow {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    static constexpr int NUM_SLIDERS = 8;
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

    juce::MidiKeyboardComponent midiKeyboard;

    void timerCallback() override
    {
        for (auto& op : ops)
            op.envDisplay.repaint();
        midiKeyboard.repaint();
    }

    void setupSlider(SliderRow& row, const juce::String& paramId,
                     int minVal, int maxVal, juce::Colour colour);
    void styleColumn(OpColumn& col, int opIdx);

    static constexpr int kTitleH    = 46;
    static constexpr int kHeaderH   = 36;
    static constexpr int kEnvH      = 60;
    static constexpr int kSliderH   = 44;
    static constexpr int kKeyboardH = 80;
    static constexpr int kMargin    = 8;
    static constexpr int kPad       = 6;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SquareWaveSynthAudioProcessorEditor)
};
