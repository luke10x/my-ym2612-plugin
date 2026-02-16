# SquareWaveSynth

A minimal but fully functional **square-wave instrument plugin** built with [JUCE](https://juce.com/).

## Features

| Feature | Detail |
|---|---|
| Waveform | Bandlimited square wave (harmonic summation, no aliasing) |
| Polyphony | 8 voices |
| MIDI input | Full note-on/off + velocity |
| Parameter | **Duty Cycle** (0 – 99%), saved with host session |
| MIDI keyboard | On-screen 5-octave keyboard widget (C2–C7) |
| Formats | VST3 (all platforms) · AU (macOS only) |

---

## Prerequisites

| Tool | Version |
|---|---|
| CMake | ≥ 3.22 |
| C++ compiler | C++17 (Clang, GCC, MSVC) |
| Xcode (macOS AU) | ≥ 14 |
| JUCE | 7.x (fetched automatically or supply path) |

---

## Quick Start

### macOS / Linux

```bash
# Clone this repo
git clone https://github.com/you/SquareWaveSynth
cd SquareWaveSynth

# Build (JUCE is downloaded automatically via FetchContent)
chmod +x build.sh
./build.sh

# Or point to a local JUCE clone (much faster – no download)
./build.sh --juce ~/SDKs/JUCE
```

### Windows

```bat
build.bat
REM or with local JUCE:
build.bat --juce C:\SDKs\JUCE
```

### Manual CMake

```bash
cmake -B build -DJUCE_SOURCE_DIR=/path/to/JUCE -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(nproc)
```

---

## Installing the plugins

### macOS

```bash
# VST3
cp -r build/SquareWaveSynth_artefacts/Release/VST3/SquareWaveSynth.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/

# AU
cp -r build/SquareWaveSynth_artefacts/Release/AU/SquareWaveSynth.component \
      ~/Library/Audio/Plug-Ins/Components/

# Re-scan AU cache
killall -9 AudioComponentRegistrar 2>/dev/null || true
```

### Windows

Copy `SquareWaveSynth.vst3` to `C:\Program Files\Common Files\VST3\`.

---

## Project structure

```
SquareWaveSynth/
├── CMakeLists.txt          # Main build script
├── build.sh                # macOS/Linux build helper
├── build.bat               # Windows build helper
└── Source/
    ├── SynthSound.h        # JUCE SynthesiserSound subclass
    ├── SynthVoice.h        # Bandlimited square-wave voice + ADSR
    ├── PluginProcessor.h/.cpp   # AudioProcessor: synth engine, APVTS
    └── PluginEditor.h/.cpp      # Editor: knob, live waveform preview, MIDI kbd
```

---

## Extending the plugin

Ideas for next steps:

- **More parameters** – add Attack/Release sliders hooked into `SynthVoice`'s ADSR.
- **Pitch wheel** – implement `pitchWheelMoved()` in `SynthVoice`.
- **Filter** – add a `juce::IIRFilter` or `juce::StateVariableTPTFilter` per voice.
- **Unison / detune** – spawn multiple sub-oscillators per voice.
- **LFO** – modulate duty cycle at audio or sub-audio rate.
- **Custom Look and Feel** – subclass `juce::LookAndFeel_V4` for a bespoke UI skin.

---

## License

MIT – do whatever you like. JUCE itself is subject to the [JUCE licence](https://juce.com/juce-6-licence/).
