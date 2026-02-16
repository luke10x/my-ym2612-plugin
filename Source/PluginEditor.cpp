#include "PluginEditor.h"

static const char* OP_ROLE[4] = { "Modulator", "Modulator", "Modulator", "Carrier" };
static const char* OP_LABEL[4] = { "OP 1", "OP 2", "OP 3", "OP 4" };

SquareWaveSynthAudioProcessorEditor::SquareWaveSynthAudioProcessorEditor(
    SquareWaveSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      midiKeyboard(p.getMidiKeyboardState(),
                   juce::MidiKeyboardComponent::horizontalKeyboard)
{
    for (int i = 0; i < 4; i++) {
        auto& op = ops[i];

        // Slider
        op.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        op.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 18);
        op.slider.setRange(0, 127, 1);
        // Carrier gets a bright accent; modulators a dimmer teal
        juce::Colour knobCol = (i == 3) ? accent : accent.withAlpha(0.65f);
        op.slider.setColour(juce::Slider::rotarySliderFillColourId, knobCol);
        op.slider.setColour(juce::Slider::rotarySliderOutlineColourId,
                            knobCol.withAlpha(0.25f));
        op.slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        op.slider.setColour(juce::Slider::textBoxTextColourId, text);
        op.slider.setColour(juce::Slider::textBoxOutlineColourId,
                            juce::Colours::transparentBlack);
        op.slider.setColour(juce::Slider::textBoxBackgroundColourId,
                            panel.brighter(0.05f));
        addAndMakeVisible(op.slider);

        // Name label (OP 1..4)
        op.label.setText(OP_LABEL[i], juce::dontSendNotification);
        op.label.setFont(juce::Font(14.0f, juce::Font::bold));
        op.label.setColour(juce::Label::textColourId, text);
        op.label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(op.label);

        // Role label (Modulator / Carrier)
        op.roleLabel.setText(OP_ROLE[i], juce::dontSendNotification);
        op.roleLabel.setFont(juce::Font(10.5f));
        op.roleLabel.setColour(juce::Label::textColourId,
                               i == 3 ? accent : dim);
        op.roleLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(op.roleLabel);

        // Attachment (parameter ID matches OP_LEVEL_ID from processor)
        op.attachment = std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.apvts, OP_LEVEL_ID[i], op.slider);
    }

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

    const int totalH = kTitleH + kMargin + kControlH + kMargin + kKeyboardH;
    setSize(520, totalH);
    setResizable(true, true);
    setResizeLimits(400, totalH, 1200, totalH + 60);

    startTimerHz(30);
}

SquareWaveSynthAudioProcessorEditor::~SquareWaveSynthAudioProcessorEditor()
{
    stopTimer();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paint
// ─────────────────────────────────────────────────────────────────────────────

void SquareWaveSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(bg);

    // Title bar
    const auto titleR = getLocalBounds().removeFromTop(kTitleH).toFloat();
    g.setColour(panel);
    g.fillRect(titleR);

    // Accent bar at bottom of title
    g.setColour(accent);
    g.fillRect(0.0f, titleR.getBottom() - 2.0f, (float)getWidth(), 2.0f);

    // Title text
    g.setColour(accent);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("YM2612 Synth v2", titleR, juce::Justification::centred, false);

    // Subtitle
    g.setColour(dim);
    g.setFont(juce::Font(10.0f));
    g.drawText("FM Synthesis  \xc2\xb7  6 Voices  \xc2\xb7  4 Operators",
               titleR.withTrimmedTop(22.0f), juce::Justification::centred, false);

    // Operator panel background
    const int panelY  = kTitleH + kMargin / 2;
    const int panelH  = kControlH + kMargin;
    g.setColour(panel);
    g.fillRoundedRectangle(kMargin / 2.0f, (float)panelY,
                           getWidth() - kMargin, (float)panelH, 6.0f);

    // Vertical dividers between ops
    const int opW = (getWidth() - kMargin) / 4;
    g.setColour(accent.withAlpha(0.12f));
    for (int i = 1; i < 4; i++) {
        float x = kMargin / 2.0f + i * opW;
        g.drawLine(x, (float)panelY + 8, x, (float)(panelY + panelH) - 8, 1.0f);
    }

    // "→ carrier" flow arrow area (tiny)
    g.setColour(dim.withAlpha(0.5f));
    g.setFont(juce::Font(9.0f));
    g.drawText("OP1\xe2\x86\x92OP2\xe2\x86\x92OP3\xe2\x86\x92OP4 \xe2\x86\x92 out  [algo 4]",
               kMargin, panelY + panelH - 14,
               getWidth() - kMargin * 2, 14,
               juce::Justification::centred, false);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resized
// ─────────────────────────────────────────────────────────────────────────────

void SquareWaveSynthAudioProcessorEditor::resized()
{
    const int opW    = (getWidth() - kMargin) / 4;
    const int knobSz = 68;
    const int panelY = kTitleH + kMargin / 2;

    for (int i = 0; i < 4; i++) {
        const int cx = kMargin / 2 + i * opW + opW / 2;
        const int topY = panelY + 8;

        ops[i].label    .setBounds(cx - opW / 2, topY, opW, 18);
        ops[i].roleLabel.setBounds(cx - opW / 2, topY + 18, opW, 14);
        ops[i].slider   .setBounds(cx - knobSz / 2, topY + 34, knobSz, knobSz + 20);
    }

    // MIDI keyboard at bottom
    const int kbY = kTitleH + kMargin + kControlH + kMargin;
    midiKeyboard.setBounds(0, kbY, getWidth(), kKeyboardH);
}
