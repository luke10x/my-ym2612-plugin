#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

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
// EnvelopeDisplay (unchanged)
// ─────────────────────────────────────────────────────────────────────────────
class EnvelopeDisplay : public juce::Component
{
public:
    void setParams(juce::RangedAudioParameter* ar, juce::RangedAudioParameter* dr,
                   juce::RangedAudioParameter* sl, juce::RangedAudioParameter* sr,
                   juce::RangedAudioParameter* rr, bool isCarrier)
    {
        pAR = ar; pDR = dr; pSL = sl; pSR = sr; pRR = rr;
        m_isCarrier = isCarrier;
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        float w = bounds.getWidth(), h = bounds.getHeight();
        float x0 = bounds.getX(), y0 = bounds.getY();

        g.setColour(YmColors::panel.darker(0.3f));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(YmColors::border);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        if (!pAR) return;

        float ar = pAR->getValue(), dr = pDR->getValue();
        float sl = 1.0f - pSL->getValue(), sr = pSR->getValue(), rr = pRR->getValue();
        float yTop = y0 + 4.0f, yBot = y0 + h - 4.0f;
        float ySL  = yTop + (yBot - yTop) * (1.0f - sl);

        auto rateToWidth = [](float rate, float baseWidth) -> float {
            return baseWidth * (1.0f - rate * 0.7f);
        };

        float wAtk = rateToWidth(ar, w * 0.18f);
        float wDec = rateToWidth(dr, w * 0.18f);
        float wSus = w * 0.40f * (1.0f - sr * 0.8f);
        float wRel = w * 0.30f * (1.0f - rr * 0.75f);

        juce::Path p;
        float cx = x0;
        p.startNewSubPath(cx, yBot); cx += wAtk;
        p.lineTo(cx, yTop); cx += wDec;
        p.lineTo(cx, ySL); cx += wSus;
        p.lineTo(cx, ySL);
        p.lineTo(cx + wRel, yBot);

        juce::Path fill = p;
        fill.lineTo(cx + wRel, yBot); fill.lineTo(x0, yBot); fill.closeSubPath();
        juce::Colour lineCol = m_isCarrier ? YmColors::accent : YmColors::mod;
        g.setColour(lineCol.withAlpha(0.18f)); g.fillPath(fill);
        g.setColour(lineCol.withAlpha(0.9f));
        g.strokePath(p, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(lineCol.withAlpha(0.3f));
        float dashLen = 3.0f, gapLen = 3.0f, xd = x0;
        while (xd < x0 + w) {
            g.drawHorizontalLine(static_cast<int>(ySL), xd, xd + dashLen);
            xd += dashLen + gapLen;
        }

        g.setColour(YmColors::dim); g.setFont(juce::Font(8.5f));
        g.drawText("EG", bounds.translated(4, 2), juce::Justification::topLeft, false);
    }

private:
    juce::RangedAudioParameter *pAR=nullptr, *pDR=nullptr, *pSL=nullptr, *pSR=nullptr, *pRR=nullptr;
    bool m_isCarrier = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// SquareWaveSynthAudioProcessorEditor
//
// Layout:
//   Title bar
//   Global settings panel (algorithm, feedback, LFO, octave, etc.)
//   [OP1 col] [OP2 col] [OP3 col] [OP4 col]
//   MIDI keyboard
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

    // ── Global settings panel ─────────────────────────────────────────────────
    struct LabeledControl {
        juce::Label label;
        juce::Component* control = nullptr;  // owned by unique_ptr below or built-in
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cbAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> slAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> btnAtt;
    };

    juce::ComboBox algorithmBox, lfoFreqBox;
    juce::Slider   feedbackSlider, amsSlider, fmsSlider, octaveSlider;
    juce::TextButton importBtn, exportBtn;

    LabeledControl globalAlgo, globalFb, globalLfoEn, globalLfoFreq,
                   globalAms, globalFms, globalOct;

    // ── Per-operator columns ──────────────────────────────────────────────────
    struct SliderRow {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct ToggleRow {
        juce::ToggleButton toggle;
        juce::Label        label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    static constexpr int NUM_SLIDERS = 8;  // TL, AR, DR, SL, SR, RR, MUL, DT
    static constexpr const char* SLIDER_LABELS[NUM_SLIDERS] = {
        "Level", "Attack", "Decay", "Sus.Lvl", "Sus.Rate", "Release", "Multi", "Detune"
    };

    struct OpColumn {
        juce::Label      nameLabel;
        juce::Label      roleLabel;
        EnvelopeDisplay  envDisplay;
        SliderRow        rows[NUM_SLIDERS];
        SliderRow        rsRow;      // Rate Scale
        ToggleRow        amRow;      // AM Enable
        struct {
            juce::ComboBox box;
            juce::Label    label;
            std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
        } ssgModeRow;  // SSG-EG Mode (includes Off option)
    };
    OpColumn ops[4];

    juce::MidiKeyboardComponent midiKeyboard;

    void timerCallback() override
    {
        for (auto& op : ops) op.envDisplay.repaint();
        midiKeyboard.repaint();
    }

    void setupSlider(SliderRow& row, const juce::String& paramId,
                     int minVal, int maxVal, juce::Colour colour);
    void setupToggle(ToggleRow& row, const juce::String& paramId, juce::Colour colour);
    void styleColumn(OpColumn& col, int opIdx);
    void setupGlobalControls();

    static constexpr int kTitleH    = 46;
    static constexpr int kGlobalH   = 98;   // Increased for label rows above dropdowns
    static constexpr int kHeaderH   = 36;
    static constexpr int kEnvH      = 60;
    static constexpr int kSliderH   = 44;
    static constexpr int kToggleH   = 30;
    static constexpr int kComboH    = 36;   // Increased for label above SSG dropdown
    static constexpr int kKeyboardH = 80;
    static constexpr int kMargin    = 8;
    static constexpr int kPad       = 6;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SquareWaveSynthAudioProcessorEditor)
};
