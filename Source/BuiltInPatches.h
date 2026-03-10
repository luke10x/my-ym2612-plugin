#pragma once

// =============================================================================
// YM2612 Patch Structure (simplified ymfm-compatible format)
// =============================================================================
struct YM2612Operator
{
    int DT;   // Detune
    int MUL;  // Multiplier
    int TL;   // Total Level
    int RS;   // Rate Scaling
    int AR;   // Attack Rate
    int AM;   // AM Enable
    int DR;   // Decay Rate
    int SR;   // Sustain Rate
    int SL;   // Sustain Level
    int RR;   // Release Rate
    int SSG;  // SSG-EG
};

struct YM2612Patch
{
    int ALG;  // Algorithm
    int FB;   // Feedback
    int AMS;  // AM Sensitivity
    int FMS;  // FM Sensitivity
    
    YM2612Operator op[4];
};

// =============================================================================
// Built-in Patches
// =============================================================================

static constexpr YM2612Patch PATCH_SLAP_BASS =
{
    .ALG = 4,
    .FB  = 5,
    .AMS = 2,
    .FMS = 3,
    
    .op =
    {
        { .DT = 3, .MUL = 1, .TL = 34, .RS = 0, .AR = 31, .AM = 0, .DR = 10, .SR = 6, .SL = 4, .RR = 7, .SSG = 0 },
        { .DT = 0, .MUL = 2, .TL = 18, .RS = 1, .AR = 25, .AM = 0, .DR = 12, .SR = 5, .SL = 5, .RR = 6, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL =  0, .RS = 0, .AR = 31, .AM = 0, .DR =  6, .SR = 3, .SL = 6, .RR = 5, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL =  0, .RS = 0, .AR = 31, .AM = 0, .DR =  7, .SR = 2, .SL = 5, .RR = 5, .SSG = 0 }
    }
};
static constexpr int PATCH_SLAP_BASS_BLOCK = 0;      // Octave offset
static constexpr int PATCH_SLAP_BASS_LFO_ENABLE = 0; // LFO on/off
static constexpr int PATCH_SLAP_BASS_LFO_FREQ = 0;   // LFO frequency (0-7)

static constexpr YM2612Patch PATCH_SYNTH_BASS =
{
    .ALG = 5,
    .FB  = 7,
    .AMS = 0,
    .FMS = 4,
    
    .op =
    {
        { .DT = 0, .MUL = 1, .TL = 20, .RS = 0, .AR = 31, .AM = 0, .DR = 15, .SR = 7, .SL = 3, .RR = 8, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 15, .RS = 0, .AR = 28, .AM = 0, .DR = 12, .SR = 6, .SL = 4, .RR = 7, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL =  0, .RS = 0, .AR = 31, .AM = 0, .DR =  0, .SR = 0, .SL = 0, .RR = 0, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL =  0, .RS = 0, .AR = 31, .AM = 0, .DR =  0, .SR = 0, .SL = 0, .RR = 0, .SSG = 0 }
    }
};
static constexpr int PATCH_SYNTH_BASS_BLOCK = 0;
static constexpr int PATCH_SYNTH_BASS_LFO_ENABLE = 1;
static constexpr int PATCH_SYNTH_BASS_LFO_FREQ = 3;

static constexpr YM2612Patch PATCH_ELECTRIC_BASS =
{
    .ALG = 4,
    .FB  = 6,
    .AMS = 1,
    .FMS = 2,
    
    .op =
    {
        { .DT = 2, .MUL = 1, .TL = 28, .RS = 0, .AR = 31, .AM = 0, .DR = 12, .SR = 5, .SL = 4, .RR = 6, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 22, .RS = 0, .AR = 26, .AM = 0, .DR = 10, .SR = 4, .SL = 5, .RR = 5, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL =  0, .RS = 0, .AR = 31, .AM = 0, .DR =  5, .SR = 3, .SL = 6, .RR = 4, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL =  0, .RS = 0, .AR = 31, .AM = 0, .DR =  0, .SR = 0, .SL = 0, .RR = 0, .SSG = 0 }
    }
};
static constexpr int PATCH_ELECTRIC_BASS_BLOCK = 0;
static constexpr int PATCH_ELECTRIC_BASS_LFO_ENABLE = 0;
static constexpr int PATCH_ELECTRIC_BASS_LFO_FREQ = 0;

static constexpr YM2612Patch PATCH_ACOUSTIC_BASS =
{
    .ALG = 2,
    .FB  = 3,
    .AMS = 0,
    .FMS = 1,
    
    .op =
    {
        { .DT = 1, .MUL = 1, .TL = 24, .RS = 0, .AR = 30, .AM = 0, .DR =  8, .SR = 4, .SL = 3, .RR = 5, .SSG = 0 },
        { .DT = 0, .MUL = 2, .TL = 16, .RS = 1, .AR = 24, .AM = 0, .DR = 10, .SR = 5, .SL = 4, .RR = 6, .SSG = 0 },
        { .DT = 0, .MUL = 1, .TL = 12, .RS = 0, .AR = 28, .AM = 0, .DR =  6, .SR = 3, .SL = 5, .RR = 4, .SSG = 0 },
        { .DT = 0, .MUL = 0, .TL =  0, .RS = 0, .AR = 31, .AM = 0, .DR =  0, .SR = 0, .SL = 0, .RR = 0, .SSG = 0 }
    }
};
static constexpr int PATCH_ACOUSTIC_BASS_BLOCK = 0;
static constexpr int PATCH_ACOUSTIC_BASS_LFO_ENABLE = 0;
static constexpr int PATCH_ACOUSTIC_BASS_LFO_FREQ = 0;

constexpr YM2612Patch PATCH_GUITAR =
{
    .ALG = 3,
    .FB  = 7,
    .AMS = 0,
    .FMS = 0,

    .op =
    {
        { .DT = 3, .MUL = 15, .TL = 61, .RS = 0, .AR = 11, .AM = 0, .DR = 0, .SR = 0, .SL = 10, .RR = 0, .SSG = 0 },
        { .DT = 3, .MUL = 1, .TL = 4, .RS = 0, .AR = 21, .AM = 0, .DR = 18, .SR = 0, .SL = 2, .RR = 4, .SSG = 0 },
        { .DT = -2, .MUL = 7, .TL = 19, .RS = 0, .AR = 31, .AM = 0, .DR = 31, .SR = 0, .SL = 15, .RR = 9, .SSG = 1 },
        { .DT = 0, .MUL = 2, .TL = 6, .RS = 0, .AR = 21, .AM = 0, .DR = 5, .SR = 0, .SL = 1, .RR = 5, .SSG = 0 }
    }
};
constexpr int PATCH_GUITAR_BLOCK = 0;      // Octave offset
constexpr int PATCH_GUITAR_LFO_ENABLE = 0; // LFO on/off
constexpr int PATCH_GUITAR_LFO_FREQ = 5;   // LFO frequency (0-7)

// Array of all patches with metadata
struct PatchEntry
{
    const char* name;
    const YM2612Patch* patch;
    int block;      // BLOCK (octave offset)
    int lfoEnable;  // LFO enable (0/1)
    int lfoFreq;    // LFO frequency (0-7)
};

static constexpr PatchEntry kBuiltInPatches[] = 
{
    { "Slap Bass",     &PATCH_SLAP_BASS,     PATCH_SLAP_BASS_BLOCK,     PATCH_SLAP_BASS_LFO_ENABLE,     PATCH_SLAP_BASS_LFO_FREQ },
    { "Synth Bass",    &PATCH_SYNTH_BASS,    PATCH_SYNTH_BASS_BLOCK,    PATCH_SYNTH_BASS_LFO_ENABLE,    PATCH_SYNTH_BASS_LFO_FREQ },
    { "Electric Bass", &PATCH_ELECTRIC_BASS, PATCH_ELECTRIC_BASS_BLOCK, PATCH_ELECTRIC_BASS_LFO_ENABLE, PATCH_ELECTRIC_BASS_LFO_FREQ },
    { "Acoustic Bass", &PATCH_ACOUSTIC_BASS, PATCH_ACOUSTIC_BASS_BLOCK, PATCH_ACOUSTIC_BASS_LFO_ENABLE, PATCH_ACOUSTIC_BASS_LFO_FREQ },
    { "Guitar",        &PATCH_GUITAR,        PATCH_GUITAR_BLOCK,        PATCH_GUITAR_LFO_ENABLE,        PATCH_GUITAR_LFO_FREQ },
};

static constexpr int kNumBuiltInPatches = sizeof(kBuiltInPatches) / sizeof(kBuiltInPatches[0]);
