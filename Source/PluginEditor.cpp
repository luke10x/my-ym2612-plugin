#include "PluginEditor.h"

using namespace YmColors;

// ── Operator metadata ─────────────────────────────────────────────────────────
static const char* OP_NAME[4] = { "OP 1", "OP 2", "OP 3", "OP 4" };
static const char* OP_ROLE[4] = { "Modulator", "Carrier", "Modulator", "Carrier" };
static const bool  OP_CARRIER[4] = { false, true, false, true };

// Parameter IDs indexed [sliderIndex][op]
static const juce::String* PARAM_IDS[8] = {
    OP_TL_ID, OP_AR_ID, OP_DR_ID, OP_SR_ID,
    OP_SL_ID, OP_RR_ID, OP_MUL_ID, OP_DT_ID
};
// Min/max for each slider type
static const int PARAM_MIN[8] = {   0,  0,  0,  0,  0,  0,  0, -3 };
static const int PARAM_MAX[8] = { 127, 31, 31, 31, 15, 15, 15,  3 };

// ─────────────────────────────────────────────────────────────────────────────
SquareWaveSynthAudioProcessorEditor::SquareWaveSynthAudioProcessorEditor(
    SquareWaveSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      midiKeyboard(p.getMidiKeyboardState(),
                   juce::MidiKeyboardComponent::horizontalKeyboard)
{
    for (int op = 0; op < 4; op++)
        styleColumn(ops[op], op);

    // MIDI keyboard
    midiKeyboard.setAvailableRange(36, 96);
    midiKeyboard.setScrollButtonsVisible(false);
    midiKeyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,
                           juce::Colours::white);
    midiKeyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId,
                           juce::Colour(0xFF1A1A2E));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                           accent.withAlpha(0.75f));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                           accent.withAlpha(0.3f));
    addAndMakeVisible(midiKeyboard);

    // Set explicit tab order: within each operator column, go top to bottom
    // OP1 sliders → OP2 sliders → OP3 sliders → OP4 sliders
    for (int op = 0; op < 4; op++) {
        for (int s = 0; s < NUM_SLIDERS; s++) {
            int explicitIndex = op * NUM_SLIDERS + s;
            ops[op].rows[s].slider.setExplicitFocusOrder(explicitIndex + 1);
        }
    }

    const int opAreaH = kHeaderH + kEnvH + NUM_SLIDERS * kSliderH + kPad * 2;
    const int totalH  = kTitleH + kMargin + opAreaH + kMargin + kKeyboardH;
    setSize(680, totalH);
    setResizable(true, true);
    setResizeLimits(560, totalH, 1400, totalH + 80);

    startTimerHz(30);
}

SquareWaveSynthAudioProcessorEditor::~SquareWaveSynthAudioProcessorEditor()
{
    stopTimer();
}

// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessorEditor::styleColumn(OpColumn& col, int opIdx)
{
    bool carrier = OP_CARRIER[opIdx];
    juce::Colour colAccent = carrier ? YmColors::accent : YmColors::mod;

    // Name label
    col.nameLabel.setText(OP_NAME[opIdx], juce::dontSendNotification);
    col.nameLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    col.nameLabel.setColour(juce::Label::textColourId, colAccent);
    col.nameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(col.nameLabel);

    // Role label
    col.roleLabel.setText(OP_ROLE[opIdx], juce::dontSendNotification);
    col.roleLabel.setFont(juce::Font(10.0f));
    col.roleLabel.setColour(juce::Label::textColourId, dim);
    col.roleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(col.roleLabel);

    // Envelope display
    col.envDisplay.setParams(
        dynamic_cast<juce::RangedAudioParameter*>(
            audioProcessor.apvts.getParameter(OP_AR_ID[opIdx])),
        dynamic_cast<juce::RangedAudioParameter*>(
            audioProcessor.apvts.getParameter(OP_DR_ID[opIdx])),
        dynamic_cast<juce::RangedAudioParameter*>(
            audioProcessor.apvts.getParameter(OP_SL_ID[opIdx])),
        dynamic_cast<juce::RangedAudioParameter*>(
            audioProcessor.apvts.getParameter(OP_SR_ID[opIdx])),
        dynamic_cast<juce::RangedAudioParameter*>(
            audioProcessor.apvts.getParameter(OP_RR_ID[opIdx])),
        carrier);
    addAndMakeVisible(col.envDisplay);

    // 8 sliders
    for (int s = 0; s < NUM_SLIDERS; s++) {
        setupSlider(col.rows[s], PARAM_IDS[s][opIdx],
                    PARAM_MIN[s], PARAM_MAX[s], colAccent);
    }
}

void SquareWaveSynthAudioProcessorEditor::setupSlider(
    SliderRow& row, const juce::String& paramId,
    int minVal, int maxVal, juce::Colour colour)
{
    auto& sl = row.slider;
    sl.setSliderStyle(juce::Slider::LinearHorizontal);
    sl.setTextBoxStyle(juce::Slider::TextBoxRight, false, 34, 18);
    sl.setRange(static_cast<double>(minVal), static_cast<double>(maxVal), 1.0);
    sl.setColour(juce::Slider::trackColourId,       colour.withAlpha(0.55f));
    sl.setColour(juce::Slider::thumbColourId,        colour);
    sl.setColour(juce::Slider::backgroundColourId,   panel.brighter(0.08f));
    sl.setColour(juce::Slider::textBoxTextColourId,  text);
    sl.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    sl.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    addAndMakeVisible(sl);

    // Label
    row.label.setFont(juce::Font(10.5f));
    row.label.setColour(juce::Label::textColourId, dim);
    row.label.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(row.label);

    row.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, paramId, sl);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paint
// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);

    // Title bar
    auto titleR = getLocalBounds().removeFromTop(kTitleH).toFloat();
    g.setColour(panel);
    g.fillRect(titleR);
    g.setColour(accent);
    g.fillRect(0.0f, titleR.getBottom() - 2.0f, static_cast<float>(getWidth()), 2.0f);
    g.setColour(accent);
    g.setFont(juce::Font(19.0f, juce::Font::bold));
    g.drawText("YM2612 Synth", titleR.withTrimmedBottom(14.0f),
               juce::Justification::centred, false);
    g.setColour(dim);
    g.setFont(juce::Font(10.0f));
    g.drawText("FM Synthesis  \xc2\xb7  6 Voices  \xc2\xb7  Algorithm 4",
               titleR.withTrimmedTop(26.0f), juce::Justification::centred, false);

    // Operator panel background
    const int opAreaY = kTitleH + kMargin;
    const int opAreaH = kHeaderH + kEnvH + NUM_SLIDERS * kSliderH + kPad * 2;
    g.setColour(panel);
    g.fillRoundedRectangle(kMargin * 0.5f, static_cast<float>(opAreaY),
                           getWidth() - kMargin, static_cast<float>(opAreaH), 6.0f);

    // Vertical column dividers
    const int colW = (getWidth() - kMargin) / 4;
    g.setColour(YmColors::border);
    for (int i = 1; i < 4; i++) {
        float x = kMargin * 0.5f + i * colW;
        g.drawLine(x, static_cast<float>(opAreaY + 6),
                   x, static_cast<float>(opAreaY + opAreaH - 6), 1.0f);
    }

    // Alternating row backgrounds
    for (int s = 0; s < NUM_SLIDERS; s++) {
        if (s % 2 == 0) {
            int rowY = opAreaY + kHeaderH + kEnvH + kPad + s * kSliderH;
            g.setColour(juce::Colour(0x08FFFFFF));
            g.fillRect(static_cast<int>(kMargin * 0.5f), rowY,
                       getWidth() - kMargin, kSliderH);
        }
    }

    // Algo flow hint
    g.setColour(dim.withAlpha(0.5f));
    g.setFont(juce::Font(9.0f));
    int hintY = opAreaY + opAreaH - 13;
    g.drawText("OP1 \xe2\x86\x92 OP2 \xe2\x9e\x95 OP3 \xe2\x86\x92 OP4  [algo 4  \xc2\xb7  FB on OP1]",
               kMargin, hintY, getWidth() - kMargin * 2, 12,
               juce::Justification::centred, false);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resized
// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessorEditor::resized()
{
    const int colW   = (getWidth() - kMargin) / 4;
    const int opAreaY = kTitleH + kMargin;

    for (int op = 0; op < 4; op++) {
        const int cx = static_cast<int>(kMargin * 0.5f) + op * colW;
        int y = opAreaY + kPad;

        // Name + role
        ops[op].nameLabel.setBounds(cx + 2, y, colW - 4, 20);
        ops[op].roleLabel.setBounds(cx + 2, y + 20, colW - 4, 14);
        y += kHeaderH;

        // Envelope display
        ops[op].envDisplay.setBounds(cx + 4, y, colW - 8, kEnvH);
        y += kEnvH;

        // 8 slider rows
        const int labelW = 52;
        const int sliderX = cx + labelW + 2;
        const int sliderW = colW - labelW - 6;

        for (int s = 0; s < NUM_SLIDERS; s++) {
            int rowMidY = y + s * kSliderH + kSliderH / 2;
            ops[op].rows[s].label.setText(SLIDER_LABELS[s],
                                          juce::dontSendNotification);
            ops[op].rows[s].label .setBounds(cx + 2, rowMidY - 9, labelW, 18);
            ops[op].rows[s].slider.setBounds(sliderX, rowMidY - 12, sliderW, 24);
        }
    }

    // MIDI keyboard
    const int opAreaH = kHeaderH + kEnvH + NUM_SLIDERS * kSliderH + kPad * 2;
    const int kbY = kTitleH + kMargin + opAreaH + kMargin;
    midiKeyboard.setBounds(0, kbY, getWidth(), kKeyboardH);
}
