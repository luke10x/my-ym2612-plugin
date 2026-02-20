#include "PluginEditor.h"

using namespace YmColors;

static const char* OP_NAME[4] = { "OP 1", "OP 2", "OP 3", "OP 4" };
static const char* OP_ROLE[4] = { "Modulator", "Carrier", "Modulator", "Carrier" };
static const bool  OP_CARRIER[4] = { false, true, false, true };

static const juce::String* PARAM_IDS[8] = {
    OP_TL_ID, OP_AR_ID, OP_DR_ID, OP_SL_ID, OP_SR_ID, OP_RR_ID, OP_MUL_ID, OP_DT_ID
};
static const int PARAM_MIN[8] = {   0,  0,  0,  0,  0,  0,  0, -3 };
static const int PARAM_MAX[8] = { 127, 31, 31, 15, 31, 15, 15,  3 };

static constexpr int NUM_SLIDERS_BEFORE_ENV = 1;   // Just Level
static constexpr int NUM_SLIDERS_AFTER_ENV = 5;    // AR, DR, SL, SR, RR
static constexpr int NUM_SLIDERS_EXTRA = 2;        // MUL, DT

// ─────────────────────────────────────────────────────────────────────────────
SquareWaveSynthAudioProcessorEditor::SquareWaveSynthAudioProcessorEditor(
    SquareWaveSynthAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      midiKeyboard(p.getMidiKeyboardState(),
                   juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setupGlobalControls();

    for (int op = 0; op < 4; op++)
        styleColumn(ops[op], op);

    midiKeyboard.setAvailableRange(36, 96);
    midiKeyboard.setScrollButtonsVisible(false);
    midiKeyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colours::white);
    midiKeyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xFF1A1A2E));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, accent.withAlpha(0.75f));
    midiKeyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, accent.withAlpha(0.3f));
    addAndMakeVisible(midiKeyboard);

    // Tab order: within each op column
    for (int op = 0; op < 4; op++) {
        int base = op * (NUM_SLIDERS + 2);  // Reduced by 1 since SSG selector handles its own focus
        for (int s = 0; s < NUM_SLIDERS; s++)
            ops[op].rows[s].slider.setExplicitFocusOrder(base + s + 1);
        ops[op].rsRow.slider.setExplicitFocusOrder(base + NUM_SLIDERS + 1);
        ops[op].amRow.toggle.setExplicitFocusOrder(base + NUM_SLIDERS + 2);
        // SSG-EG selector manages its own focus
    }

    // New layout: Level above EG, then EG, then SSG below EG, then envelope params, then extras
    const int opAreaH = kHeaderH + kSliderH + kEnvH + kComboH + 
                        NUM_SLIDERS_AFTER_ENV * kSliderH + NUM_SLIDERS_EXTRA * kSliderH + 
                        kSliderH + kToggleH + kPad * 2;
    const int totalH  = kTitleH + kMargin + kGlobalH + kMargin + opAreaH + kMargin + kKeyboardH;
    setSize(720, totalH);
    setResizable(true, true);
    setResizeLimits(600, totalH, 1600, totalH + 100);
    
    addAndMakeVisible(oscilloscope);

    startTimerHz(30);
}

SquareWaveSynthAudioProcessorEditor::~SquareWaveSynthAudioProcessorEditor()
{
    stopTimer();
}

// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessorEditor::setupGlobalControls()
{
    // Instrument name label (editable)
    instrumentNameLabel.setText(audioProcessor.getInstrumentName(), juce::dontSendNotification);
    instrumentNameLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    instrumentNameLabel.setColour(juce::Label::textColourId, text);
    instrumentNameLabel.setColour(juce::Label::backgroundColourId, panel.darker(0.3f));
    instrumentNameLabel.setColour(juce::Label::outlineColourId, border);
    instrumentNameLabel.setJustificationType(juce::Justification::centred);
    instrumentNameLabel.setEditable(true);
    instrumentNameLabel.onTextChange = [this]() {
        audioProcessor.setInstrumentName(instrumentNameLabel.getText());
    };
    addAndMakeVisible(instrumentNameLabel);

    // Algorithm selector (visual)
    algorithmSelector.setSelectedAlgorithm(
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(GLOBAL_ALGORITHM)->load()));
    algorithmSelector.onChange = [this](int algo) {
        if (auto* param = audioProcessor.apvts.getParameter(GLOBAL_ALGORITHM))
            param->setValueNotifyingHost(param->convertTo0to1(float(algo)));
    };
    addAndMakeVisible(algorithmSelector);

    // Feedback
    feedbackSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 28, 18);
    feedbackSlider.setRange(0, 7, 1);
    globalFb.control = &feedbackSlider;
    globalFb.slAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, GLOBAL_FEEDBACK, feedbackSlider);
    globalFb.label.setText("Feedback", juce::dontSendNotification);
    globalFb.label.setJustificationType(juce::Justification::centredRight);
    globalFb.label.setFont(juce::Font(11.0f));
    globalFb.label.setColour(juce::Label::textColourId, dim);
    addAndMakeVisible(globalFb.label);
    addAndMakeVisible(feedbackSlider);

    // LFO Freq (includes Off option now)
    lfoFreqBox.addItemList(getLfoFreqNames(), 1);
    globalLfoFreq.control = &lfoFreqBox;
    globalLfoFreq.cbAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.apvts, GLOBAL_LFO_FREQ, lfoFreqBox);
    globalLfoFreq.label.setText("LFO", juce::dontSendNotification);
    globalLfoFreq.label.setJustificationType(juce::Justification::centredLeft);
    globalLfoFreq.label.setFont(juce::Font(10.0f));
    globalLfoFreq.label.setColour(juce::Label::textColourId, dim);
    addAndMakeVisible(globalLfoFreq.label);
    addAndMakeVisible(lfoFreqBox);

    // AMS
    amsSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    amsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 28, 18);
    amsSlider.setRange(0, 3, 1);
    globalAms.control = &amsSlider;
    globalAms.slAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, GLOBAL_AMS, amsSlider);
    globalAms.label.setText("AMS", juce::dontSendNotification);
    globalAms.label.setJustificationType(juce::Justification::centredRight);
    globalAms.label.setFont(juce::Font(11.0f));
    globalAms.label.setColour(juce::Label::textColourId, dim);
    addAndMakeVisible(globalAms.label);
    addAndMakeVisible(amsSlider);

    // FMS
    fmsSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    fmsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 28, 18);
    fmsSlider.setRange(0, 7, 1);
    globalFms.control = &fmsSlider;
    globalFms.slAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, GLOBAL_FMS, fmsSlider);
    globalFms.label.setText("FMS", juce::dontSendNotification);
    globalFms.label.setJustificationType(juce::Justification::centredRight);
    globalFms.label.setFont(juce::Font(11.0f));
    globalFms.label.setColour(juce::Label::textColourId, dim);
    addAndMakeVisible(globalFms.label);
    addAndMakeVisible(fmsSlider);

    // Octave
    octaveSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    octaveSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 28, 18);
    octaveSlider.setRange(-2, 2, 1);
    globalOct.control = &octaveSlider;
    globalOct.slAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, GLOBAL_OCTAVE, octaveSlider);
    globalOct.label.setText("Octave", juce::dontSendNotification);
    globalOct.label.setJustificationType(juce::Justification::centredRight);
    globalOct.label.setFont(juce::Font(11.0f));
    globalOct.label.setColour(juce::Label::textColourId, dim);
    addAndMakeVisible(globalOct.label);
    addAndMakeVisible(octaveSlider);
    
    // Import/Export buttons
    importBtn.setButtonText("Import .fui");
    importBtn.onClick = [this]() {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Import Furnace Instrument", juce::File(), "*.fui");
        auto flags = juce::FileBrowserComponent::openMode | 
                     juce::FileBrowserComponent::canSelectFiles;
        chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file.existsAsFile()) {
                if (audioProcessor.importFurnaceInstrument(file)) {
                    // Update the instrument name label
                    juce::String newName = audioProcessor.getInstrumentName();
                    DBG("=== UI UPDATE ===");
                    DBG("Name retrieved from processor: '" << newName << "'");
                    instrumentNameLabel.setText(newName, juce::dontSendNotification);
                    DBG("Label text after update: '" << instrumentNameLabel.getText() << "'");
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::InfoIcon, "Import Successful",
                        "Loaded instrument from " + file.getFileName());
                } else {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon, "Import Failed",
                        "Could not load Furnace instrument file.");
                }
            }
        });
    };
    addAndMakeVisible(importBtn);
    
    exportBtn.setButtonText("Export .fui");
    exportBtn.onClick = [this]() {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Export Furnace Instrument", juce::File(), "*.fui");
        auto flags = juce::FileBrowserComponent::saveMode | 
                     juce::FileBrowserComponent::canSelectFiles |
                     juce::FileBrowserComponent::warnAboutOverwriting;
        chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File()) {
                if (!file.hasFileExtension(".fui"))
                    file = file.withFileExtension(".fui");
                // Use current instrument name
                juce::String name = audioProcessor.getInstrumentName();
                if (audioProcessor.exportFurnaceInstrument(file, name))
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::InfoIcon, "Export Successful",
                        "Saved: " + file.getFileName());
                else
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon, "Export Failed",
                        "Could not save Furnace instrument file.");
            }
        });
    };
    addAndMakeVisible(exportBtn);
}

void SquareWaveSynthAudioProcessorEditor::styleColumn(OpColumn& col, int opIdx)
{
    bool carrier = OP_CARRIER[opIdx];
    juce::Colour colAccent = carrier ? YmColors::accent : YmColors::mod;

    col.nameLabel.setText(OP_NAME[opIdx], juce::dontSendNotification);
    col.nameLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    col.nameLabel.setColour(juce::Label::textColourId, colAccent);
    col.nameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(col.nameLabel);

    col.roleLabel.setText(OP_ROLE[opIdx], juce::dontSendNotification);
    col.roleLabel.setFont(juce::Font(10.0f));
    col.roleLabel.setColour(juce::Label::textColourId, dim);
    col.roleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(col.roleLabel);

    col.envDisplay.setParams(
        dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(OP_AR_ID[opIdx])),
        dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(OP_DR_ID[opIdx])),
        dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(OP_SL_ID[opIdx])),
        dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(OP_SR_ID[opIdx])),
        dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(OP_RR_ID[opIdx])),
        carrier);
    addAndMakeVisible(col.envDisplay);

    for (int s = 0; s < NUM_SLIDERS; s++) {
        setupSlider(col.rows[s], PARAM_IDS[s][opIdx], PARAM_MIN[s], PARAM_MAX[s], colAccent);
    }

    // Rate Scale
    setupSlider(col.rsRow, OP_RS_ID[opIdx], 0, 3, colAccent);

    // AM Enable
    setupToggle(col.amRow, OP_AM_ID[opIdx], colAccent);
    
    // SSG-EG Mode visual selector (includes Off option as index 0)
    col.ssgModeSelector.setSelectedMode(
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(OP_SSG_MODE_ID[opIdx])->load()));
    col.ssgModeSelector.onChange = [this, opIdx](int mode) {
        if (auto* param = audioProcessor.apvts.getParameter(OP_SSG_MODE_ID[opIdx]))
            param->setValueNotifyingHost(param->convertTo0to1(float(mode)));
    };
    addAndMakeVisible(col.ssgModeSelector);
}

void SquareWaveSynthAudioProcessorEditor::setupSlider(SliderRow& row, const juce::String& paramId,
                                                       int minVal, int maxVal, juce::Colour colour)
{
    auto& sl = row.slider;
    sl.setSliderStyle(juce::Slider::LinearHorizontal);
    sl.setTextBoxStyle(juce::Slider::TextBoxRight, false, 34, 18);
    sl.setRange(static_cast<double>(minVal), static_cast<double>(maxVal), 1.0);
    sl.setColour(juce::Slider::trackColourId, colour.withAlpha(0.55f));
    sl.setColour(juce::Slider::thumbColourId, colour);
    sl.setColour(juce::Slider::backgroundColourId, panel.brighter(0.08f));
    sl.setColour(juce::Slider::textBoxTextColourId, text);
    sl.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    sl.setColour(juce::Slider::textBoxBackgroundColourId, panel);
    addAndMakeVisible(sl);

    row.label.setFont(juce::Font(10.5f));
    row.label.setColour(juce::Label::textColourId, dim);
    row.label.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(row.label);

    row.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, paramId, sl);
}

void SquareWaveSynthAudioProcessorEditor::setupToggle(ToggleRow& row, const juce::String& paramId,
                                                       juce::Colour colour)
{
    row.toggle.setButtonText("");
    row.toggle.setColour(juce::ToggleButton::tickColourId, colour);
    row.toggle.setColour(juce::ToggleButton::tickDisabledColourId, dim);
    addAndMakeVisible(row.toggle);

    row.label.setFont(juce::Font(10.5f));
    row.label.setColour(juce::Label::textColourId, dim);
    row.label.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(row.label);

    row.att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, paramId, row.toggle);
}

// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);

    // Title bar
    auto titleR = getLocalBounds().removeFromTop(kTitleH).toFloat();
    g.setColour(panel); g.fillRect(titleR);
    g.setColour(accent);
    g.fillRect(0.0f, titleR.getBottom() - 2.0f, static_cast<float>(getWidth()), 2.0f);
    g.setFont(juce::Font(19.0f, juce::Font::bold));
    g.drawText("YM2612 Synth", titleR.withTrimmedBottom(14.0f), juce::Justification::centred, false);
    g.setColour(dim); g.setFont(juce::Font(10.0f));
    g.drawText("FM Synthesis  ·  6 Voices", titleR.withTrimmedTop(26.0f), juce::Justification::centred, false);

    // Global panel background
    const int globalY = kTitleH + kMargin;
    g.setColour(panel);
    g.fillRoundedRectangle(kMargin * 0.5f, static_cast<float>(globalY),
                           getWidth() - kMargin, static_cast<float>(kGlobalH), 6.0f);

    // Operator panel background
    const int opAreaY = kTitleH + kMargin + kGlobalH + kMargin;
    const int opAreaH = kHeaderH + kSliderH + kEnvH + kComboH + 
                        NUM_SLIDERS_AFTER_ENV * kSliderH + NUM_SLIDERS_EXTRA * kSliderH + 
                        kSliderH + kToggleH + kPad * 2;
    g.setColour(panel);
    g.fillRoundedRectangle(kMargin * 0.5f, static_cast<float>(opAreaY),
                           getWidth() - kMargin, static_cast<float>(opAreaH), 6.0f);

    // Column dividers
    int colW = (getWidth() - kMargin) / 4;  // Local variable (not const to avoid conflict)
    g.setColour(YmColors::border);
    for (int i = 1; i < 4; i++) {
        float x = kMargin * 0.5f + i * colW;
        g.drawLine(x, static_cast<float>(opAreaY + 6),
                   x, static_cast<float>(opAreaY + opAreaH - 6), 1.0f);
    }

    // Alternating row backgrounds for sliders after envelope
    // Skip Level (before env) and SSG (right after env)
    // Shade AR, SL, RR, DT, RS (indices 1, 3, 5, 7, and RS row)
    const int firstEnvSliderY = opAreaY + kHeaderH + kSliderH + kEnvH + kComboH;
    for (int s = 0; s < NUM_SLIDERS_AFTER_ENV + NUM_SLIDERS_EXTRA + 1; s++) {  // +1 for RS
        if (s % 2 == 0) {  // Shade even rows: 0=AR, 2=SL, 4=RR, 6=DT
            int rowY = firstEnvSliderY + s * kSliderH;
            g.setColour(juce::Colour(0x08FFFFFF));
            g.fillRect(static_cast<int>(kMargin * 0.5f), rowY, getWidth() - kMargin, kSliderH);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void SquareWaveSynthAudioProcessorEditor::resized()
{
    const int scopeH = 60;
    
    // Oscilloscope at very top
    oscilloscope.setBounds(kMargin, kMargin, getWidth() - kMargin * 2, scopeH);
    
    // Instrument name label below scope
    const int nameY = kMargin + scopeH + 4;
    instrumentNameLabel.setBounds(getWidth() / 4, nameY, getWidth() / 2, 20);

    // Global panel below name
    const int globalY = nameY + 24;
    const int colW = (getWidth() - kMargin) / 4;
    const int pad = 8;
    
    // Column 1: Algorithm (square)
    int col1X = static_cast<int>(kMargin * 0.5f) + pad;
    int algoSize = colW - pad * 2;  // Square
    algorithmSelector.setBounds(col1X, globalY + 8, algoSize, algoSize);
    
    // Column 2: Feedback + Octave
    int col2X = static_cast<int>(kMargin * 0.5f) + colW + pad;
    
    globalFb.label.setBounds(col2X, globalY + 8, 60, 20);
    globalFb.label.setFont(juce::Font(11.0f));
    feedbackSlider.setBounds(col2X + 4, globalY + 28, colW - pad * 2 - 4, 22);
    
    globalOct.label.setBounds(col2X, globalY + 58, 60, 20);
    globalOct.label.setFont(juce::Font(11.0f));
    octaveSlider.setBounds(col2X + 4, globalY + 78, colW - pad * 2 - 4, 22);
    
    // Column 3: LFO + AMS + FMS
    int col3X = static_cast<int>(kMargin * 0.5f) + colW * 2 + pad;
    
    globalLfoFreq.label.setBounds(col3X, globalY + 8, colW - pad * 2, 14);
    globalLfoFreq.label.setFont(juce::Font(11.0f));
    lfoFreqBox.setBounds(col3X, globalY + 24, colW - pad * 2, 24);
    
    globalAms.label.setBounds(col3X, globalY + 56, 40, 20);
    globalAms.label.setFont(juce::Font(11.0f));
    amsSlider.setBounds(col3X + 42, globalY + 56, colW - pad * 2 - 42, 22);
    
    globalFms.label.setBounds(col3X, globalY + 84, 40, 20);
    globalFms.label.setFont(juce::Font(11.0f));
    fmsSlider.setBounds(col3X + 42, globalY + 84, colW - pad * 2 - 42, 22);
    
    // Column 4: Import + Export
    int col4X = static_cast<int>(kMargin * 0.5f) + colW * 3 + pad;
    
    importBtn.setBounds(col4X, globalY + 40, colW - pad * 2, 28);
    exportBtn.setBounds(col4X, globalY + 74, colW - pad * 2, 28);

    // Operator columns below global panel
    const int opAreaY = kTitleH + kMargin + kGlobalH + kMargin;
    // colW already defined above, reuse it
    
    // Operator columns - new layout:
    // Header (name + role)
    // Level slider
    // Envelope display
    // SSG-EG dropdown
    // AR, DR, SL, SR, RR sliders
    // MUL, DT sliders
    // Rate Scale slider
    // AM Enable toggle
    for (int op = 0; op < 4; op++) {
        const int cx = static_cast<int>(kMargin * 0.5f) + op * colW;
        int y = opAreaY + kPad;

        // Header
        ops[op].nameLabel.setBounds(cx + 2, y, colW - 4, 20);
        ops[op].roleLabel.setBounds(cx + 2, y + 20, colW - 4, 14); 
        y += kHeaderH;

        const int labelW = 52, sliderX = cx + labelW + 2, sliderW = colW - labelW - 6;

        // Level slider (before envelope)
        int rowMidY = y + kSliderH / 2;
        ops[op].rows[0].label.setText("Level", juce::dontSendNotification);
        ops[op].rows[0].label.setBounds(cx + 2, rowMidY - 9, labelW, 18);
        ops[op].rows[0].slider.setBounds(sliderX, rowMidY - 12, sliderW, 24);
        y += kSliderH;

        // Envelope display
        ops[op].envDisplay.setBounds(cx + 4, y, colW - 8, kEnvH); 
        y += kEnvH;

        // SSG-EG visual selector (same height as envelope for consistency)
        ops[op].ssgModeSelector.setBounds(cx + 2, y, colW - 6, kEnvH);
        y += kEnvH;

        // Envelope parameter sliders: AR, DR, SL, SR, RR (indices 1-5)
        for (int s = 1; s <= 5; s++) {
            rowMidY = y + kSliderH / 2;
            ops[op].rows[s].label.setText(SLIDER_LABELS[s], juce::dontSendNotification);
            ops[op].rows[s].label.setBounds(cx + 2, rowMidY - 9, labelW, 18);
            ops[op].rows[s].slider.setBounds(sliderX, rowMidY - 12, sliderW, 24);
            y += kSliderH;
        }

        // Extra sliders: MUL, DT (indices 6-7)
        for (int s = 6; s < 8; s++) {
            rowMidY = y + kSliderH / 2;
            ops[op].rows[s].label.setText(SLIDER_LABELS[s], juce::dontSendNotification);
            ops[op].rows[s].label.setBounds(cx + 2, rowMidY - 9, labelW, 18);
            ops[op].rows[s].slider.setBounds(sliderX, rowMidY - 12, sliderW, 24);
            y += kSliderH;
        }

        // Rate Scale
        rowMidY = y + kSliderH / 2;
        ops[op].rsRow.label.setText("RateScale", juce::dontSendNotification);
        ops[op].rsRow.label.setBounds(cx + 2, rowMidY - 9, labelW, 18);
        ops[op].rsRow.slider.setBounds(sliderX, rowMidY - 12, sliderW, 24);
        y += kSliderH;

        // AM Enable
        ops[op].amRow.label.setText("AM Enable", juce::dontSendNotification);
        ops[op].amRow.label.setBounds(cx + 2, y, labelW, kToggleH);
        ops[op].amRow.toggle.setBounds(sliderX, y + 4, 24, 24);
    }

    // MIDI keyboard - centered with background padding
    const int opAreaH = kHeaderH + kSliderH + kEnvH + kEnvH +  // SSG now same height as envelope
                        NUM_SLIDERS_AFTER_ENV * kSliderH + NUM_SLIDERS_EXTRA * kSliderH + 
                        kSliderH + kToggleH + kPad * 2;
    const int kbY = kTitleH + kMargin + kGlobalH + kMargin + opAreaH + kMargin;
    const int kbWidth = getWidth() - kMargin * 4;  // Inset from edges
    const int kbX = (getWidth() - kbWidth) / 2;
    midiKeyboard.setBounds(kbX, kbY, kbWidth, kKeyboardH);
}
