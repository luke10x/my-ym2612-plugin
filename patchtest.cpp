#include <SDL.h>
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
        for(int op=0;op<4;op++)
        {
            write(0,0x30+op*4+ch,(p.op[op].DT<<4)|p.op[op].MUL);
            write(0,0x40+op*4+ch,p.op[op].TL);
            write(0,0x50+op*4+ch,(p.op[op].RS<<6)|p.op[op].AR);
            write(0,0x60+op*4+ch,(p.op[op].AM<<7)|p.op[op].DR);
            write(0,0x70+op*4+ch,p.op[op].SR);
            write(0,0x80+op*4+ch,(p.op[op].SL<<4)|p.op[op].RR);
            write(0,0x90+op*4+ch,p.op[op].SSG);
        }

        write(0, 0xB0+ch, (p.FB<<3)|p.ALG);
        write(0, 0xB4+ch, 0xC0 | (p.AMS<<4) | p.FMS);  // <-- 0xC0 enables L+R output!
    }

    void enable_lfo(uint8_t freq)
    {
        write(0,0x22,0x08|freq);
    }

    void set_pitch(int ch,uint16_t fnum,uint8_t block)
    {
        write(0,0xA0+ch,fnum&0xFF);
        write(0,0xA4+ch,((fnum>>8)&7)|(block<<3));
    }

    void key_on(int ch)
    {
        write(0,0x28,0xF0|ch);
    }

    void key_off(int ch)
    {
        write(0,0x28,ch);
    }

    void generate(int16_t *stream,int samples)
    {
        for(int i=0;i<samples;i++)
        {
            ymfm::ym2612::output_data out;

            chip.generate(&out,1);

            stream[i*2+0] = out.data[0];
            stream[i*2+1] = out.data[1];
        }
    }
};

SimpleYM synth;

struct Note
{
    uint16_t fnum;
    uint8_t block;
};

Note note_table[12] =
{
    {617,4},{653,4},{692,4},{733,4},
    {777,4},{824,4},{873,4},{925,4},
    {980,4},{1038,4},{1100,4},{1165,4}
};

void audio_callback(void* userdata, Uint8* stream, int len)
{
    int samples = len / 4;
    synth.generate((int16_t*)stream,samples);
}


int key_to_note(SDL_Keycode key)   // <-- was "vv"
{
    switch(key)
    {
        case SDLK_z:return 0;
        case SDLK_s:return 1;
        case SDLK_x:return 2;
        case SDLK_d:return 3;
        case SDLK_c:return 4;
        case SDLK_v:return 5;
        case SDLK_g:return 6;
        case SDLK_b:return 7;
        case SDLK_h:return 8;
        case SDLK_n:return 9;
        case SDLK_j:return 10;
        case SDLK_m:return 11;
    }
    return -1;
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);  // <-- add VIDEO

    // Create a window — required for keyboard events to be delivered
    SDL_Window* window = SDL_CreateWindow(
        "YM2612 Patch Tester",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        400, 200,
        SDL_WINDOW_SHOWN
    );

    SDL_AudioSpec spec{};
    spec.freq    = 44100;
    spec.format  = AUDIO_S16;
    spec.channels = 2;
    spec.samples = 512;
    spec.callback = audio_callback;

    SDL_OpenAudio(&spec, nullptr);

    synth.load_patch(PATCH_ELECTRIC_BASS, 0);
    synth.enable_lfo(5);

    SDL_PauseAudio(0);

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

                int n = key_to_note(e.key.keysym.sym);

                if(n >= 0)
                {
                    auto note = note_table[n];
                    // note.block -= 1;
                    synth.set_pitch(0, note.fnum, note.block);
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
    SDL_DestroyWindow(window);   // <-- clean up
    SDL_Quit();
}
