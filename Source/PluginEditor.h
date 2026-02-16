#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

/**
 * YM2612 Synth Editor
 *
 * Layout:
 *  ┌────────────────────────────────────────────────────┐
 *  │  Title bar: "YM2612 Synth"                         │
 *  ├────────────────────────────────────────────────────┤
 *  │  [OP1 Level]  [OP2 Level]  [OP3 Level]  [OP4 Level]│
 *  │  (Modulator)  (Modulator)  (Modulator)  (Carrier)  │
 *  ├────────────────────────────────────────────────────┤
 *  │  MIDI Keyboard                                     │
 *  └────────────────────────────────────────────────────┘
 *
 * OP4 is the carrier (output operator in algo 4); its level is the
 * master volume. OP1-3 are modulators: raising them adds more FM
 * character / harmonic richness.
 */
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

    // ── Operator controls (4 ops) ─────────────────────────────────────────
    struct OpControl {
        juce::Slider slider;
        juce::Label  label;
        juce::Label  roleLabel;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    OpControl ops[4];

    // ── MIDI keyboard ─────────────────────────────────────────────────────
    juce::MidiKeyboardComponent midiKeyboard;

    void timerCallback() override { midiKeyboard.repaint(); }

    // ── Style ────────────────────────────────────────────────────────────
    static constexpr int kTitleH    = 44;
    static constexpr int kControlH  = 120;
    static constexpr int kKeyboardH = 90;
    static constexpr int kMargin    = 14;

    juce::Colour bg     { 0xFF0F0F1A };
    juce::Colour panel  { 0xFF1A1A2E };
    juce::Colour accent { 0xFF00D4AA };  // teal – Genesis vibes
    juce::Colour text   { 0xFFE0F0FF };
    juce::Colour dim    { 0xFF607080 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SquareWaveSynthAudioProcessorEditor)
};
