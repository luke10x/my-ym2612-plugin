# ARM2612

**YM2612 (Sega Mega Drive) FM Synthesizer Plugin**

A high-fidelity VST3/AU/Standalone software synthesizer plugin that accurately emulates the Yamaha YM2612 FM chip used in the Sega Mega Drive/Genesis. Built with JUCE and powered by Aaron Giles' ymfm emulation core.

## 🎬 Demo

[![ARM2612 Demo](https://img.youtube.com/vi/UymoMBYGNIw/0.jpg)](https://www.youtube.com/shorts/UymoMBYGNIw)

*Click to watch a quick demonstration of ARM2612 in action*

---

## Features

### Authentic YM2612 Emulation
- **4 FM operators** with 8 algorithm configurations
- **Hardware-accurate** sound using ymfm core
- **Full parameter control** over all YM2612 registers
- **SSG-EG envelope modes** (8 modes + off)
- **LFO modulation** with frequency/amplitude sensitivity
- **Feedback** control for algorithm complexity

### Modern Plugin Interface
- **Visual algorithm selector** - see operator routing at a glance
- **Visual SSG-EG selector** - understand envelope shapes instantly
- **Real-time oscilloscope** with phase-lock for waveform analysis
- **Per-operator envelope displays** - visualize ADSR in real-time
- **MIDI keyboard** with velocity sensitivity
- **Dark, professional UI** optimized for music production

### Furnace Integration
- **Import/Export .fui files** - seamlessly work with Furnace tracker
- **Full parameter compatibility** - all YM2612 features supported
- **Preset management** - name and organize your patches

### Cross-Platform
- **macOS** (AU, VST3, Standalone)
- **Windows** (VST3, Standalone)
- **Linux** (VST3, Standalone)

---

## Installation

### Pre-built Binaries (Windows Only)
Download the latest Windows release from the [Releases](https://github.com/yourusername/ARM2612/releases) page.

**Windows:**
- **VST3**: Unzip and copy `ARM2612-vX.X.X.vst3` to `C:\Program Files\Common Files\VST3\`
- **Standalone**: Unzip and run `ARM2612-vX.X.X.exe` directly

**Note:** macOS builds (AU/VST3) require local building due to code-signing requirements (see below).

### Building from Source

**Requirements:**
- CMake 3.22+
- C++17 compiler (Xcode 14+, Visual Studio 2019+, GCC 9+)
- Git

**macOS (for AU/VST3/Standalone):**
```bash
git clone https://github.com/yourusername/ARM2612.git
cd ARM2612
chmod +x build.sh
./build.sh
```

After building, you'll find:
- **AU**: `build/ARM2612_artefacts/Release/AU/ARM2612.component`
  - Install: `cp -R build/ARM2612_artefacts/Release/AU/ARM2612.component ~/Library/Audio/Plug-Ins/Components/`
  - Compatible with: Logic Pro, GarageBand, MainStage, Ableton Live, etc.
- **VST3**: `build/ARM2612_artefacts/Release/VST3/ARM2612.vst3`
  - Install: `cp -R build/ARM2612_artefacts/Release/VST3/ARM2612.vst3 ~/Library/Audio/Plug-Ins/VST3/`
  - Compatible with: Reaper, Ableton Live, FL Studio 20+, Bitwig, etc.
- **Standalone**: `build/ARM2612_artefacts/Release/Standalone/ARM2612.app`
  - Install: `cp -R build/ARM2612_artefacts/Release/Standalone/ARM2612.app /Applications/`
  - Runs independently without a DAW

**macOS Code Signing (Optional but Recommended):**
To avoid Gatekeeper warnings, you can self-sign the plugins:
```bash
# Sign AU
codesign --force --deep --sign - build/ARM2612_artefacts/Release/AU/ARM2612.component

# Sign VST3
codesign --force --deep --sign - build/ARM2612_artefacts/Release/VST3/ARM2612.vst3

# Sign Standalone
codesign --force --deep --sign - build/ARM2612_artefacts/Release/Standalone/ARM2612.app
```

After signing, copy to the appropriate locations as shown above.

**Requirements:**
- CMake 3.22+
- C++17 compiler (Xcode 14+, Visual Studio 2019+, GCC 9+)
- Git

**Windows (for VST3/Standalone):**
```cmd
git clone https://github.com/yourusername/ARM2612.git
cd ARM2612
build.bat
```

After building, you'll find:
- **VST3**: `build_win\ARM2612_artefacts\Release\VST3\ARM2612.vst3`
  - Install: Copy to `C:\Program Files\Common Files\VST3\`
- **Standalone**: `build_win\ARM2612_artefacts\Release\Standalone\ARM2612.exe`
  - Run directly or copy to a convenient location

**Linux (for VST3/Standalone):**
```bash
git clone https://github.com/yourusername/ARM2612.git
cd ARM2612
chmod +x build.sh
./build.sh
```

After building, you'll find:
- **VST3**: `build/ARM2612_artefacts/Release/VST3/ARM2612.vst3`
  - Install: `cp -R build/ARM2612_artefacts/Release/VST3/ARM2612.vst3 ~/.vst3/`
- **Standalone**: `build/ARM2612_artefacts/Release/Standalone/ARM2612`
  - Run directly or copy to `/usr/local/bin/`

---

## Usage

### Interface Layout

**Top Bar:**
- Instrument name (click to edit)

**Global Controls (4 columns):**
1. **Algorithm** - Visual selector showing operator routing (8 algorithms)
2. **Feedback & Octave** - Algorithm feedback + global pitch shift
3. **LFO Controls** - Frequency selection, AMS, FMS modulation depth
4. **File Operations** - Import/Export .fui, Phase Lock toggle

**Oscilloscope:**
- Real-time waveform display in columns 2-3
- Enable "Phase Lock" to stabilize waveform for analysis
- 2.5x zoom shows 2-3 cycles clearly

**Operator Columns (OP1-OP4):**
Each operator has:
- **Role indicator** (Modulator/Carrier)
- **Level slider** (0-127, TL parameter)
- **Envelope display** (real-time visualization)
- **SSG-EG selector** (visual envelope shape picker)
- **ADSR parameters** (Attack, Decay, Sustain Level, Sustain Rate, Release)
- **Multiplier & Detune** (harmonic control)
- **Rate Scale** (envelope velocity scaling)
- **AM Enable** (LFO amplitude modulation)

### Algorithm Configurations

- **Algorithm 0**: Serial (1→2→3→4) - Classic FM stack
- **Algorithm 1**: Dual modulators (1,3→2→4) - Rich modulation
- **Algorithm 2**: Parallel modulation (1→4, 2→3→4)
- **Algorithm 3**: Mixed (1→2→4, 3→4)
- **Algorithm 4**: Dual stacks (1→2, 3→4) - Two-voice patches
- **Algorithm 5**: Fan-out (1→2,3,4) - Complex harmonics
- **Algorithm 6**: Stack + carriers (1→2, 3, 4) - Layered sounds
- **Algorithm 7**: Fully parallel (1, 2, 3, 4) - Additive-style

### SSG-EG Modes

- **None** - Standard envelope
- **Down Down Down** - Sawtooth down repeating
- **Down.** - Decay once, hold at zero
- **Down Up Down Up** - Triangle wave
- **Down UP** - Decay then hold at max
- **Up Up Up** - Sawtooth up repeating
- **Up.** - Attack once, hold at max
- **Up Down Up Down** - Inverted triangle
- **Up DOWN** - Attack then hold at zero

### Tips for Classic Mega Drive Sounds

**Bass:**
- Algorithm 4 (dual stacks)
- High feedback (5-7)
- Fast attack, medium decay
- Low multipliers (1-2)

**Lead:**
- Algorithm 1 or 5
- Moderate feedback (3-4)
- Medium attack for smoothness
- Higher multipliers (3-7) for brightness

**Percussion:**
- Algorithm 7 (all parallel)
- Very fast attack/release
- High detune for metallic quality
- SSG-EG modes for snappy envelopes

---

## Furnace Integration

ARM2612 is fully compatible with [Furnace](https://github.com/tildearrow/furnace) tracker's YM2612 instrument format (.fui files).

**Workflow:**
1. Design patches in Furnace's Genesis/Mega Drive system
2. Export instrument as .fui file
3. Click "Import .fui" in ARM2612
4. Tweak in real-time with MIDI input
5. Export back to Furnace with "Export .fui"

All YM2612 parameters are preserved including SSG-EG modes, operator enable flags, and LFO settings.

---

## Alternative YM2612 Plugins

If you're looking for other YM2612 emulation options:

- **[ADLplug](https://github.com/jpcima/ADLplug)** - Multi-chip FM synthesizer supporting YM2612, OPL3, and more
- **[Genny](https://github.com/superjoebob/genny)** - Another open-source YM2612 VST plugin
- **[RYM2612](https://www.inphonik.com/products/rym2612-iconic-fm-synthesizer/)** - Commercial plugin by Inphonik with enhanced features

Each has different strengths - try them all to find what works best for your workflow!

---

## Learning More About YM2612

Want to dive deeper into how this chip works?

**Technical Resources:**
- **[Emulating the YM2612](https://jsgroth.dev/blog/posts/emulating-ym2612-part-7/)** by jsgroth - Comprehensive series on YM2612 emulation techniques
- **[Atsushi Eno's Research](https://atsushieno.github.io/)** - Deep technical documentation on FM synthesis and the YM2612

**The ymfm Library:**
- ARM2612 uses [ymfm](https://github.com/aaronsgiles/ymfm) by Aaron Giles for cycle-accurate emulation
- One of the most accurate YM2612 cores available

---

## Technical Details

### YM2612 Chip Specifications
- **Synthesis:** 6-operator FM (4 operators per channel)
- **Polyphony:** Hardware-accurate voice allocation
- **Sample Rate:** 53,267 Hz (chip native), resampled to host rate
- **LFO:** Global modulation with per-operator sensitivity
- **SSG-EG:** Advanced envelope modes from SSG chip integration

### Plugin Specifications
- **Format:** VST3, AU, Standalone
- **Polyphony:** Hardware-accurate
- **MIDI:** Full support, velocity sensitivity
- **Latency:** Minimal (dependent on buffer size)
- **CPU:** Low (ymfm is highly optimized)

---

## Credits

**Built With:**
- [JUCE](https://juce.com/) - Audio plugin framework
- [ymfm](https://github.com/aaronsgiles/ymfm) - YM2612 emulation core by Aaron Giles
- [Furnace](https://github.com/tildearrow/furnace) - .fui format inspiration

**Special Thanks:**
- **Aaron Giles** - For the incredible ymfm emulation core
- **tildearrow** - For Furnace and format documentation
- **jsgroth** - For detailed YM2612 emulation research and documentation
- **Atsushi Eno** - For comprehensive FM synthesis and YM2612 technical resources
- **Sega/Yamaha** - For the legendary YM2612 chip that powered the Mega Drive/Genesis

---

## License

[Add your license here - MIT, GPL, etc.]

---

## Links

- **GitHub**: https://github.com/yourusername/ARM2612
- **Issues**: https://github.com/yourusername/ARM2612/issues
- **Furnace**: https://github.com/tildearrow/furnace
- **ymfm**: https://github.com/aaronsgiles/ymfm

---

**Sega Mega Drive / Genesis Forever!** 🎮🎵
