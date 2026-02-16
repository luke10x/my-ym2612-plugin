#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

/**
 * SynthSound – tells the synthesiser which MIDI notes/channels this
 * sound should respond to.  We simply respond to everything.
 */
class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel (int /*midiChannel*/)    override { return true; }
};
