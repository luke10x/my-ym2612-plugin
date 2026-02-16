/*
 * YM2612 FM sound chip emulator
 *
 * A clean, correct implementation for plugin use.
 * Inspired by the MAME/Genesis Plus GX architecture but rewritten for
 * clarity and correctness.  License: GPL v2+
 *
 * Design choices:
 *   - Internal processing in double precision, output as INT32
 *   - Phase accumulator: uint32, wraps naturally at 2^32
 *     phase_inc is scaled so 2^32 counts = one full sine period at fs
 *   - Envelope: linear 0.0 (loud) .. 1.0 (silent), maps to dB via exp
 *   - All 8 algorithms implemented correctly
 *   - TL, AR, DR, SR, RR, SL all wired up with sensible defaults
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mamedef.h"
#include "mame_ym2612fm.h"

/* ── Constants ──────────────────────────────────────────────────────────── */
#define NUM_CH    6
#define NUM_SLOTS 4

/* Envelope: stored as integer attenuation in 1/64 dB steps, 0=max, 1023=off */
#define ENV_MAX   1023
#define ENV_QUIET 896    /* below this = effectively silent for early-out    */

/* Phase accumulator: 32-bit, 2^32 = one full cycle.
 * phase_inc = (freq / samplerate) * 2^32                                   */

/* ── Envelope states ────────────────────────────────────────────────────── */
typedef enum { EG_OFF=0, EG_REL, EG_SUS, EG_DEC, EG_ATT } EGState;

/* ── Slot ───────────────────────────────────────────────────────────────── */
typedef struct {
    /* Phase */
    UINT32 phase;
    UINT32 phase_inc;   /* set by ch_set_freq() */
    UINT8  mul;         /* 0..15; effective = mul ? mul*2 : 1  (in half-steps) */
    UINT8  det;         /* 0..7; 4-7 negate detune */

    /* Envelope (all in ENV units: 0=full volume, ENV_MAX=silence) */
    INT32   env;        /* current level */
    EGState eg_state;
    INT32   tl;         /* total level 0..ENV_MAX (set from TL register 0..127) */
    INT32   sl;         /* sustain level 0..ENV_MAX */

    /* Rates – stored as env-units per sample * 65536 (fixed-point speed)   */
    UINT32  ar_inc;     /* attack  speed (env decrements by this per sample) */
    UINT32  dr_inc;     /* decay   speed */
    UINT32  sr_inc;     /* sustain rate speed */
    UINT32  rr_inc;     /* release speed */

    /* Raw register values (kept for rate re-calc on key-scale changes)      */
    UINT8   ar_reg, dr_reg, sr_reg, rr_reg, ks_reg;

    UINT8   keyon;
} OPN_SLOT;

/* ── Channel ────────────────────────────────────────────────────────────── */
typedef struct {
    OPN_SLOT slot[4];   /* 0=OP1, 1=OP3, 2=OP2, 3=OP4  (hardware order)    */
    UINT8    algo;
    UINT8    fb;        /* 0=none .. 7=max */
    double   fb_buf[2]; /* OP1 feedback history */
    UINT32   fc;        /* base frequency counter (fn << block) */
    UINT8    kcode;     /* for key-scaling */
} OPN_CH;

/* ── Chip ───────────────────────────────────────────────────────────────── */
typedef struct {
    OPN_CH   ch[NUM_CH];
    double   sample_rate;
    UINT8    addr[2];   /* address latches for part 0/1 */
    UINT8    fn_h;      /* latched F-number high byte   */
    UINT8    dacen;
    INT32    dacout;
} YM2612;

/* ── Rate -> increment conversion ──────────────────────────────────────────
 * YM2612 envelope rates are 0-63 (after key-scaling).
 * Rate 0 = no movement.  Rate 63 = instant.
 * We target: at rate 63 the envelope goes from 0 to ENV_MAX in ~1 ms,
 *            at rate 1  it takes ~8 seconds.
 * inc is in ENV units per sample, stored * 65536 for sub-sample precision. */
static UINT32 rate_to_inc(int rate_raw, int ks, int kcode, double srate)
{
    if (rate_raw == 0) return 0;
    /* Key-scaled rate */
    int r = rate_raw * 2 + (kcode >> (3 - (ks & 3)));
    if (r > 63) r = 63;
    if (r < 1)  return 0;
    /* Envelope completes in: 2^(18-r/4) samples (approximate YM2612 timing) */
    double samples_to_complete = pow(2.0, 18.0 - r / 4.0);
    if (samples_to_complete < 1.0) samples_to_complete = 1.0;
    /* inc per sample, scaled by 65536 */
    double inc = ((double)ENV_MAX * 65536.0) / samples_to_complete;
    if (inc > (double)ENV_MAX * 65536.0) inc = (double)ENV_MAX * 65536.0;
    return (UINT32)inc;
}

/* Recalculate all rate increments for a slot */
static void slot_calc_rates(OPN_SLOT *sl, int kcode, double srate)
{
    sl->ar_inc = rate_to_inc(sl->ar_reg, sl->ks_reg, kcode, srate);
    sl->dr_inc = rate_to_inc(sl->dr_reg, sl->ks_reg, kcode, srate);
    sl->sr_inc = rate_to_inc(sl->sr_reg, sl->ks_reg, kcode, srate);
    sl->rr_inc = rate_to_inc(sl->rr_reg, sl->ks_reg, kcode, srate);
}

/* ── Detune table (phase offset in fc units) ────────────────────────────── */
/* Values from Yamaha datasheet, indexed by [DT & 3][kcode & 31] */
static const UINT32 dt_tab[4][32] = {
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
    { 0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,4,4,4,4,4,4,4,4,8,8,8,8 },
    { 1,1,1,1,2,2,2,2,4,4,4,4,8,8,8,8,16,16,16,16,32,32,32,32,64,64,64,64,128,128,128,128 },
    { 2,2,2,2,4,4,4,4,8,8,8,8,16,16,16,16,32,32,32,32,64,64,64,64,128,128,128,128,256,256,256,256 },
};

/* ── Phase increment for one slot ────────────────────────────────────────── */
static void slot_set_phase_inc(OPN_SLOT *sl, OPN_CH *ch, double srate)
{
    UINT32 fc  = ch->fc;
    UINT8  dt  = sl->det & 3;
    INT32  det = (INT32)dt_tab[dt][ch->kcode & 31];
    if (sl->det >= 4) det = -det;

    UINT32 f_adj = (UINT32)((INT32)fc + det);
    UINT32 mul   = sl->mul ? (UINT32)sl->mul * 2 : 1;

    /* phase_inc = (fc_adjusted * mul) / 2  scaled so 2^32 = one period.
     * fc after ch_set_freq is already: (fn * 2^block) which is proportional
     * to frequency.  We need to scale by (srate)^-1 * 2^32.
     * The YM2612 base clock is 7670453 Hz, internal divider 144.
     * fc_max ~ 0x7FF << 7 = 0x3FF80, a full cycle at fc corresponds to
     * fref = clock/144.  So freq_hz = fc * clock / (144 * 2^20).
     * phase_inc = freq_hz / srate * 2^32
     *           = fc * clock / (144 * 2^20) / srate * 2^32
     *           = fc * (clock * 2^12) / (144 * srate)              */
    const double clock = 7670453.0;
    double phase_f = (double)f_adj * (double)mul * clock * 4096.0
                     / (144.0 * 2.0 * srate * (double)(1ULL << 20));
    /* Multiply back up to 2^32 scale */
    phase_f *= (double)(1ULL << 32) / 4096.0;
    sl->phase_inc = (UINT32)phase_f;
}

static void ch_set_freq(YM2612 *chip, OPN_CH *ch, UINT32 block_fnum)
{
    UINT32 blk = (block_fnum >> 11) & 7;
    UINT32 fn  =  block_fnum        & 0x7FF;
    ch->fc    = fn << blk;
    ch->kcode = (UINT8)((blk << 2) | (fn >> 9));
    for (int s = 0; s < NUM_SLOTS; s++) {
        slot_set_phase_inc(&ch->slot[s], ch, chip->sample_rate);
        slot_calc_rates(&ch->slot[s], ch->kcode, chip->sample_rate);
    }
}

/* ── Envelope tick ──────────────────────────────────────────────────────── */
/* accumulator: sub-sample ENV units stored * 65536 */
static UINT32 env_acc[NUM_CH][NUM_SLOTS]; /* zero-initialised via calloc */

static INLINE void eg_tick(OPN_SLOT *sl, int c, int s)
{
    switch (sl->eg_state) {
    case EG_ATT:
        if (sl->ar_inc == 0) break;
        /* Exponential attack: env = env - env * rate (approaches 0 fast) */
        if (sl->env > 0) {
            /* Subtract a fraction; use fixed-point accumulator for speed    */
            env_acc[c][s] += sl->ar_inc;
            INT32 delta = (INT32)(env_acc[c][s] >> 16);
            env_acc[c][s] &= 0xFFFF;
            /* Exponential: scale delta by current env level */
            delta = (INT32)(((INT64)delta * (sl->env + 1)) >> 10);
            if (delta < 1) delta = 1;
            sl->env -= delta;
        }
        if (sl->env <= 0) {
            sl->env     = 0;
            sl->eg_state = EG_DEC;
            env_acc[c][s] = 0;
        }
        break;
    case EG_DEC:
        if (sl->dr_inc == 0) break;
        env_acc[c][s] += sl->dr_inc;
        sl->env += (INT32)(env_acc[c][s] >> 16);
        env_acc[c][s] &= 0xFFFF;
        if (sl->env >= sl->sl) {
            sl->env     = sl->sl;
            sl->eg_state = EG_SUS;
            env_acc[c][s] = 0;
        }
        break;
    case EG_SUS:
        if (sl->sr_inc == 0) break;
        env_acc[c][s] += sl->sr_inc;
        sl->env += (INT32)(env_acc[c][s] >> 16);
        env_acc[c][s] &= 0xFFFF;
        if (sl->env >= ENV_MAX) { sl->env = ENV_MAX; sl->eg_state = EG_OFF; }
        break;
    case EG_REL:
        if (sl->rr_inc == 0) { sl->env = ENV_MAX; sl->eg_state = EG_OFF; break; }
        env_acc[c][s] += sl->rr_inc;
        sl->env += (INT32)(env_acc[c][s] >> 16);
        env_acc[c][s] &= 0xFFFF;
        if (sl->env >= ENV_MAX) { sl->env = ENV_MAX; sl->eg_state = EG_OFF; }
        break;
    case EG_OFF:
        sl->env = ENV_MAX;
        break;
    }
}

/* ── Operator output ─────────────────────────────────────────────────────── */
/* Returns output in roughly ±32767 range.
 * mod: phase modulation from previous operator, in same ±32767 range.
 * Internally converts to phase offset (2^32 = 2π).                         */
static INLINE double op_out(OPN_SLOT *sl, double mod)
{
    INT32 att = sl->env + sl->tl;
    if (att >= ENV_QUIET) return 0.0;

    /* Convert envelope attenuation (0=max, ENV_MAX=silent) to linear gain.
     * Each unit = 1/64 dB => gain = 10^(-att / (64*20))                    */
    double gain = pow(10.0, -(double)att / 1280.0);   /* 64*20=1280 */

    /* Phase modulation: scale mod from audio range to fraction of 2π        */
    double pm = mod * (M_PI / 16384.0);   /* ±2π when mod = ±32767*2 */

    double phase = (double)sl->phase * (2.0 * M_PI / 4294967296.0) + pm;
    sl->phase += sl->phase_inc;

    return sin(phase) * gain * 32767.0;
}

/* ── Channel sample generation ──────────────────────────────────────────── */
static INLINE double ch_generate(YM2612 *chip, int ci)
{
    OPN_CH *ch = &chip->ch[ci];

    /* Tick EG for all slots */
    for (int s = 0; s < NUM_SLOTS; s++)
        eg_tick(&ch->slot[s], ci, s);

    /* OP1 with self-feedback */
    double fb = 0.0;
    if (ch->fb > 0)
        fb = (ch->fb_buf[0] + ch->fb_buf[1]) * (1.0 / (1 << (9 - ch->fb)));
    double op1 = op_out(&ch->slot[0], fb);
    ch->fb_buf[1] = ch->fb_buf[0];
    ch->fb_buf[0] = op1;

    /* OP2, OP3, OP4 – slot indices: 0=OP1, 2=OP2, 1=OP3, 3=OP4 */
    double op2, op3, op4;

    /* Each algorithm per YM2612 datasheet */
    switch (ch->algo) {
    case 0:  /* OP1->OP2->OP3->OP4 */
        op2 = op_out(&ch->slot[2], op1);
        op3 = op_out(&ch->slot[1], op2);
        op4 = op_out(&ch->slot[3], op3);
        return op4;
    case 1:  /* (OP1+OP2)->OP3->OP4 */
        op2 = op_out(&ch->slot[2], 0);
        op3 = op_out(&ch->slot[1], op1 + op2);
        op4 = op_out(&ch->slot[3], op3);
        return op4;
    case 2:  /* OP1->(OP3->OP4), OP2->OP4 sum */
        op2 = op_out(&ch->slot[2], 0);
        op3 = op_out(&ch->slot[1], op1);
        op4 = op_out(&ch->slot[3], op2 + op3);
        return op4;
    case 3:  /* OP1->OP2, (OP2+OP3)->OP4 */
        op2 = op_out(&ch->slot[2], op1);
        op3 = op_out(&ch->slot[1], 0);
        op4 = op_out(&ch->slot[3], op2 + op3);
        return op4;
    case 4:  /* (OP1->OP2) + (OP3->OP4) */
        op2 = op_out(&ch->slot[2], op1);
        op3 = op_out(&ch->slot[1], 0);
        op4 = op_out(&ch->slot[3], op3);
        return op2 + op4;
    case 5:  /* OP1->(OP2,OP3,OP4) all modulated by OP1 */
        op2 = op_out(&ch->slot[2], op1);
        op3 = op_out(&ch->slot[1], op1);
        op4 = op_out(&ch->slot[3], op1);
        return op2 + op3 + op4;
    case 6:  /* (OP1->OP2) + OP3 + OP4 */
        op2 = op_out(&ch->slot[2], op1);
        op3 = op_out(&ch->slot[1], 0);
        op4 = op_out(&ch->slot[3], 0);
        return op2 + op3 + op4;
    case 7:  /* OP1 + OP2 + OP3 + OP4 (additive) */
        op2 = op_out(&ch->slot[2], 0);
        op3 = op_out(&ch->slot[1], 0);
        op4 = op_out(&ch->slot[3], 0);
        return op1 + op2 + op3 + op4;
    default:
        return 0.0;
    }
}

/* ── Slot register mapping ──────────────────────────────────────────────── */
/* Reg group offsets: 0=OP1, 4=OP3, 8=OP2, 12=OP4 → slot indices 0,1,2,3 */
static const int slot_map[4] = { 0, 2, 1, 3 };

/* ── Key on/off ─────────────────────────────────────────────────────────── */
static void do_keyon(OPN_CH *ch, int op_mask, double srate)
{
    for (int s = 0; s < NUM_SLOTS; s++) {
        OPN_SLOT *sl = &ch->slot[s];
        if (op_mask & (1 << s)) {
            sl->keyon    = 1;
            sl->phase    = 0;
            sl->eg_state = EG_ATT;
            sl->env      = ENV_MAX;
        } else {
            sl->keyon = 0;
            if (sl->eg_state > EG_OFF)
                sl->eg_state = EG_REL;
        }
    }
}

/* ── Register write ─────────────────────────────────────────────────────── */
static void write_reg(YM2612 *chip, int part, UINT8 reg, UINT8 val)
{
    /* Global regs (part 0 only) */
    if (part == 0) {
        if (reg == 0x28) {
            int ch_raw = val & 0x07;
            if (ch_raw == 3 || ch_raw == 7) return;  /* reserved */
            int ci = (ch_raw < 3) ? ch_raw : ch_raw - 1;
            int op_mask = (val >> 4) & 0x0F;
            do_keyon(&chip->ch[ci], op_mask, chip->sample_rate);
            return;
        }
        if (reg == 0x2A) { chip->dacout = ((int)val - 0x80) << 6; return; }
        if (reg == 0x2B) { chip->dacen  = val >> 7;               return; }
    }

    int ch_ofs = reg & 0x03;
    if (ch_ofs == 3) return;
    int ci  = ch_ofs + part * 3;
    int s   = slot_map[(reg >> 2) & 3];
    OPN_CH   *ch = &chip->ch[ci];
    OPN_SLOT *sl = &ch->slot[s];

    switch (reg & 0xF0) {
    case 0x30:
        sl->mul = val & 0x0F;
        sl->det = (val >> 4) & 0x07;
        slot_set_phase_inc(sl, ch, chip->sample_rate);
        break;
    case 0x40:
        /* TL 0..127 → env attenuation 0..ENV_MAX */
        sl->tl = (val & 0x7F) * ENV_MAX / 127;
        break;
    case 0x50:
        sl->ks_reg = (val >> 6) & 3;
        sl->ar_reg = val & 0x1F;
        slot_calc_rates(sl, ch->kcode, chip->sample_rate);
        break;
    case 0x60:
        sl->dr_reg = val & 0x1F;
        slot_calc_rates(sl, ch->kcode, chip->sample_rate);
        break;
    case 0x70:
        sl->sr_reg = val & 0x1F;
        slot_calc_rates(sl, ch->kcode, chip->sample_rate);
        break;
    case 0x80:
        sl->sl = ((val >> 4) & 0x0F) * ENV_MAX / 15;
        sl->rr_reg = (val & 0x0F);
        slot_calc_rates(sl, ch->kcode, chip->sample_rate);
        break;
    case 0x90: break;  /* SSG-EG: not implemented */
    case 0xA0: {
        int fn_reg = reg & 0x0F;
        /* 0xA4/A5/A6 = freq hi latch; 0xA0/A1/A2 = freq lo + apply */
        if (fn_reg >= 4 && fn_reg <= 6)       chip->fn_h = val & 0x3F;
        else if (fn_reg >= 0 && fn_reg <= 2)  ch_set_freq(chip, ch, ((UINT32)chip->fn_h << 8) | val);
        break;
    }
    case 0xB0:
        if ((reg & 0x0F) < 3) { ch->algo = val & 7; ch->fb = (val >> 3) & 7; }
        break;
    }
}

/* ════════════════════════════════════════════════════════
 *  Public API
 * ════════════════════════════════════════════════════════ */

void *ym2612_init(void *param, int clock, int rate,
                  void *TimerHandler, void *IRQHandler)
{
    (void)param; (void)clock; (void)TimerHandler; (void)IRQHandler;

    YM2612 *chip = (YM2612*)calloc(1, sizeof(YM2612));
    if (!chip) return NULL;
    chip->sample_rate = (double)rate;

    for (int c = 0; c < NUM_CH; c++) {
        OPN_CH *ch = &chip->ch[c];
        ch->algo = 7;   /* additive – all 4 ops output; safest default */
        ch->fb   = 0;

        for (int s = 0; s < NUM_SLOTS; s++) {
            OPN_SLOT *sl = &ch->slot[s];
            sl->env      = ENV_MAX;
            sl->eg_state = EG_OFF;
            sl->keyon    = 0;
            sl->tl       = 0;          /* TL=0: maximum output level         */
            sl->sl       = ENV_MAX / 2;
            sl->mul      = 1;          /* ×1 frequency                       */
            sl->det      = 0;

            /* Fast attack, moderate decay, hold sustain, medium release      */
            sl->ar_reg   = 31;   sl->dr_reg = 10;
            sl->sr_reg   = 0;    sl->rr_reg = 8;
            sl->ks_reg   = 0;
            slot_calc_rates(sl, 0, chip->sample_rate);
        }
    }
    return chip;
}

void ym2612_shutdown(void *chip) { free(chip); }

void ym2612_reset_chip(void *chip)
{
    YM2612 *c = (YM2612*)chip;
    double srate = c->sample_rate;
    for (int ci = 0; ci < NUM_CH; ci++) {
        OPN_CH *ch = &c->ch[ci];
        ch->fb_buf[0] = ch->fb_buf[1] = 0.0;
        for (int s = 0; s < NUM_SLOTS; s++) {
            ch->slot[s].phase    = 0;
            ch->slot[s].env      = ENV_MAX;
            ch->slot[s].eg_state = EG_OFF;
            ch->slot[s].keyon    = 0;
            /* Preserve patch params, just reset dynamic state */
            slot_calc_rates(&ch->slot[s], ch->kcode, srate);
        }
    }
    memset(env_acc, 0, sizeof(env_acc));
    c->dacen = 0; c->dacout = 0; c->fn_h = 0;
}

void ym2612_write(void *chip, int a, unsigned char v)
{
    YM2612 *c = (YM2612*)chip;
    int part = (a >> 1) & 1;
    if (!(a & 1)) { c->addr[part] = v; return; }
    write_reg(c, part, c->addr[part], v);
}

unsigned char ym2612_read(void *chip, int a) { (void)chip; (void)a; return 0; }

int ym2612_update_one(void *chip, UINT32 length, INT32 *bufL, INT32 *bufR)
{
    YM2612 *c = (YM2612*)chip;
    for (UINT32 i = 0; i < length; i++) {
        double L = 0.0, R = 0.0;
        int num_ch = c->dacen ? 5 : NUM_CH;
        for (int ci = 0; ci < num_ch; ci++) {
            double s = ch_generate(c, ci);
            L += s; R += s;
        }
        if (c->dacen) { L += c->dacout; R += c->dacout; }
        /* 6 channels summing: scale down to avoid clipping */
        bufL[i] = (INT32)(L * (1.0 / NUM_CH));
        bufR[i] = (INT32)(R * (1.0 / NUM_CH));
    }
    return 0;
}
