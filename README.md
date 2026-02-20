# ARM2612

**YM2612 (Sega Mega Drive) FM Synthesizer Plugin**

A high-fidelity VST3/AU/Standalone software synthesizer plugin that accurately emulates the Yamaha YM2612 FM chip used in the Sega Mega Drive/Genesis. Built with JUCE and powered by Aaron Giles' ymfm emulation core.

**Authors:** Arminas ir Lukas

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

### Pre-built Binaries
Download the latest release for your platform from the [Releases](https://github.com/yourusername/ARM2612/releases) page.

**macOS:**
- **AU**: Copy `ARM2612.component` to `~/Library/Audio/Plug-Ins/Components/`
- **VST3**: Copy `ARM2612.vst3` to `~/Library/Audio/Plug-Ins/VST3/`

**Windows:**
- **VST3**: Copy `ARM2612.vst3` to `C:\Program Files\Common Files\VST3\`

**Linux:**
- **VST3**: Copy `ARM2612.vst3` to `~/.vst3/`

### Building from Source

**Requirements:**
- CMake 3.22+
- C++17 compiler (Xcode 14+, Visual Studio 2019+, GCC 9+)
- Git

**macOS:**
```bash
git clone https://github.com/yourusername/ARM2612.git
cd ARM2612
./build.sh
```

**Windows:**
```cmd
git clone https://github.com/yourusername/ARM2612.git
cd ARM2612
build.bat
```

**Linux:**
```bash
git clone https://github.com/yourusername/ARM2612.git
cd ARM2612
./build.sh
```

Built plugins will be in `build/ARM2612_artefacts/Release/` (macOS/Linux) or `build_win\ARM2612_artefacts\Release\` (Windows).

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

## Technical Details

### YM2612 Chip Specifications
- **Synthesis:** 6-operator FM (4 operators per channel)
- **Polyphony:** 6 voices (plugin implements all 6)
- **Sample Rate:** 53,267 Hz (chip native), resampled to host rate
- **LFO:** Global modulation with per-operator sensitivity
- **SSG-EG:** Advanced envelope modes from SSG chip integration

### Plugin Specifications
- **Format:** VST3, AU, Standalone
- **Polyphony:** 6 voices (hardware-accurate)
- **MIDI:** Full support, velocity sensitivity
- **Latency:** Minimal (dependent on buffer size)
- **CPU:** Low (ymfm is highly optimized)

---

## Credits

**Authors:** Arminas ir Lukas

**Built With:**
- [JUCE](https://juce.com/) - Audio plugin framework
- [ymfm](https://github.com/aaronsgiles/ymfm) - YM2612 emulation core by Aaron Giles
- [Furnace](https://github.com/tildearrow/furnace) - .fui format inspiration

**Special Thanks:**
- Aaron Giles for the incredible ymfm emulation
- tildearrow for Furnace and format documentation
- Sega/Yamaha for the legendary YM2612 chip

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
