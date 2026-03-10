#include <SDL.h>
#include <cmath>
#include "ymfm.h"
#include "ymfm_opn.h"
#include "Source/BuiltInPatches.h"

class YMInterface : public ymfm::ymfm_interface
{
public:
void ymfm_set_timer(uint32_t, int32_t) override {}
void ymfm_set_busy_end(uint32_t) override {}
bool ymfm_is_busy() override { return false; }
};

class SimpleYM
{
public:
static constexpr uint32_t YM_CLOCK = 7670453;  // Match plugin exactly

YMInterface intf;
ymfm::ym2612 chip;

SimpleYM() : chip(intf)
{
    chip.reset();
}

void write(uint8_t port,uint8_t reg,uint8_t val)
{
    chip.write(port*2+0,reg);
    chip.write(port*2+1,val);
}

void load_patch(const YM2612Patch &p,int ch)
{
    // YM2612 operator order in registers: OP1, OP3, OP2, OP4
    // But our patch structure is: OP1, OP2, OP3, OP4
    // So we need to map: patch[0]→slot0, patch[1]→slot2, patch[2]→slot1, patch[3]→slot3
    const int slotMap[4] = {0, 2, 1, 3};
    
    for(int patchOp = 0; patchOp < 4; patchOp++)
    {
        int hwSlot = slotMap[patchOp];
        const auto& op = p.op[patchOp];
        
        // Register 0x30-0x3F: DT (bits 6-4) | MUL (bits 3-0)
        // DT in patch is -3 to +3, need to convert to 0-7 for hardware
        uint8_t dt_hw = (op.DT + 3) & 0x07;
        write(0, 0x30 + hwSlot*4 + ch, (dt_hw << 4) | (op.MUL & 0x0F));
        
        // Register 0x40-0x4F: TL (bits 6-0)
        write(0, 0x40 + hwSlot*4 + ch, op.TL & 0x7F);
        
        // Register 0x50-0x5F: RS (bits 7-6) | AR (bits 4-0)
        write(0, 0x50 + hwSlot*4 + ch, ((op.RS & 0x03) << 6) | (op.AR & 0x1F));
        
        // Register 0x60-0x6F: AM (bit 7) | DR (bits 4-0)
        write(0, 0x60 + hwSlot*4 + ch, ((op.AM & 0x01) << 7) | (op.DR & 0x1F));
        
        // Register 0x70-0x7F: SR (bits 4-0)
        write(0, 0x70 + hwSlot*4 + ch, op.SR & 0x1F);
        
        // Register 0x80-0x8F: SL (bits 7-4) | RR (bits 3-0)
        write(0, 0x80 + hwSlot*4 + ch, ((op.SL & 0x0F) << 4) | (op.RR & 0x0F));
        
        // Register 0x90-0x9F: SSG-EG (bit 3 = enable, bits 2-0 = mode)
        // Patch SSG format: 0=off, 1-8=enabled with modes 0-7
        uint8_t ssg_hw = 0;
        if (op.SSG > 0) {
            int mode = op.SSG - 1;  // 1→0, 2→1, ..., 8→7
            ssg_hw = 0x08 | (mode & 0x07);  // Set enable bit + mode
        }
        write(0, 0x90 + hwSlot*4 + ch, ssg_hw);
    }

    // Register 0xB0-0xB2: FB (bits 5-3) | ALG (bits 2-0)
    write(0, 0xB0 + ch, ((p.FB & 0x07) << 3) | (p.ALG & 0x07));
    
    // Register 0xB4-0xB6: L/R enable (bits 7-6) | AMS (bits 5-4) | FMS (bits 2-0)
    // 0xC0 = both L and R enabled
    write(0, 0xB4 + ch, 0xC0 | ((p.AMS & 0x03) << 4) | (p.FMS & 0x07));
}

void enable_lfo(uint8_t enable, uint8_t freq)
{
    // Register 0x22: LFO enable (bit 3) | LFO frequency (bits 2-0)
    if (enable)
        write(0, 0x22, 0x08 | (freq & 0x07));
    else
        write(0, 0x22, 0x00);
}

void setFrequency(int ch, double hz, int octaveOffset = 0)
{
    // Apply octave offset (same as plugin)
    hz *= std::pow(2.0, octaveOffset);
    
    // Plugin formula - EXACTLY as in Ym2612Voice.h
    const double fref = static_cast<double>(YM_CLOCK) / 144.0;
    int block = 4;
    double fn = hz * static_cast<double>(1 << (20 - block)) / fref;
    
    // Adaptive block selection (same as plugin)
    while (fn > 0x7FF && block < 7) { block++; fn /= 2.0; }
    while (fn < 0x200 && block > 0) { block--; fn *= 2.0; }
    
    // Clamp fnum to valid range
    auto fnum = static_cast<uint16_t>(std::min(0x7FF, std::max(0, static_cast<int>(fn))));
    
    // Write to registers (same order as plugin)
    write(0, 0xA4 + ch, ((block & 7) << 3) | ((fnum >> 8) & 0x07));
    write(0, 0xA0 + ch, fnum & 0xFF);
    
    printf("Ch%d: %.2f Hz → fnum=%d block=%d\n", ch, hz, fnum, block);
}

void key_on(int ch)
{
    // Register 0x28: Slot mask (bits 7-4) | Channel (bits 2-0)
    // 0xF0 = all 4 slots on
    write(0, 0x28, 0xF0 | (ch & 0x07));
}

void key_off(int ch)
{
    // Register 0x28: Channel only, no slots
    write(0, 0x28, ch & 0x07);
}

void generate(int16_t *stream, int samples)
{
    for(int i = 0; i < samples; i++)
    {
        ymfm::ym2612::output_data out;

        chip.generate(&out, 1);

        stream[i*2+0] = out.data[0];
        stream[i*2+1] = out.data[1];
    }
}

};

SimpleYM synth;

// Convert MIDI note to frequency (same as JUCE’s getMidiNoteInHertz)
double midiNoteToHz(int midiNote)
{
return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
}

void audio_callback(void* userdata, Uint8* stream, int len)
{
int samples = len / 4;
synth.generate((int16_t*)stream, samples);
}

int key_to_note(SDL_Keycode key)
{
// Return MIDI note numbers (C4 = 60)
switch(key)
{
case SDLK_z: return 60;  // C4
case SDLK_s: return 61;  // C#4
case SDLK_x: return 62;  // D4
case SDLK_d: return 63;  // D#4
case SDLK_c: return 64;  // E4
case SDLK_v: return 65;  // F4
case SDLK_g: return 66;  // F#4
case SDLK_b: return 67;  // G4
case SDLK_h: return 68;  // G#4
case SDLK_n: return 69;  // A4
case SDLK_j: return 70;  // A#4
case SDLK_m: return 71;  // B4
}
return -1;
}

int main(int argc, char** argv)
{
SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);

SDL_Window* window = SDL_CreateWindow(
    "YM2612 Patch Tester",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    400, 200,
    SDL_WINDOW_SHOWN
);

SDL_AudioSpec spec{};
spec.freq     = 44100;
spec.format   = AUDIO_S16;
spec.channels = 2;
spec.samples  = 512;
spec.callback = audio_callback;

SDL_OpenAudio(&spec, nullptr);

// Load Slap Bass patch
printf("=== Loading Patch ===\n");
synth.load_patch(PATCH_GUITAR, 0);

// Enable LFO
synth.enable_lfo(PATCH_GUITAR_LFO_ENABLE, PATCH_GUITAR_LFO_FREQ);

SDL_PauseAudio(0);

printf("\n=== YM2612 Patch Tester ===\n");
printf("Loaded: Guitar\n");
printf("Press Z-M for chromatic scale (C4-B4)\n");
printf("Press ESC to quit\n\n");

bool running = true;

while(running)
{
    SDL_Event e;

    while(SDL_PollEvent(&e))
    {
        if(e.type == SDL_QUIT)
            running = false;

        if(e.type == SDL_KEYDOWN)
        {
            if(e.key.keysym.sym == SDLK_ESCAPE)
                running = false;

            int midiNote = key_to_note(e.key.keysym.sym);

            if(midiNote >= 0)
            {
                // Convert MIDI note to Hz (same as plugin)
                double hz = midiNoteToHz(midiNote);
                
                // Set frequency with octave offset from patch
                synth.setFrequency(0, hz, PATCH_GUITAR_BLOCK);
                synth.key_on(0);
            }
        }

        if(e.type == SDL_KEYUP)
        {
            synth.key_off(0);
        }
    }

    SDL_Delay(1);
}

SDL_CloseAudio();
SDL_DestroyWindow(window);
SDL_Quit();

return 0;

}