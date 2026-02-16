#pragma once
/*
 * YM2612 FM sound chip emulator
 * Based on the MAME/Genesis Plus GX implementation by Jarek Burczynski
 * and Tatsuyuki Satoh, with improvements by Eke-Eke.
 * License: GPL v2+
 *
 * This is the minimal public API surface needed to drive a single YM2612
 * instance from C++ plugin code.
 */

#include "mamedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialise a YM2612 instance.
 * clock      – chip clock in Hz (7670453 for NTSC Mega Drive)
 * rate       – output sample rate in Hz (e.g. 44100 / 48000)
 * Returns an opaque handle; NULL on failure. */
void *ym2612_init(void *param, int clock, int rate,
                  void *TimerHandler, void *IRQHandler);

/* Destroy the instance created by ym2612_init. */
void  ym2612_shutdown(void *chip);

/* Reset the chip to power-on state (all channels silent). */
void  ym2612_reset_chip(void *chip);

/* Write to a YM2612 register.
 * a  – address/data select: 0 = addr port 0, 1 = data port 0,
 *                           2 = addr port 1, 3 = data port 1
 * v  – byte value */
void  ym2612_write(void *chip, int a, unsigned char v);

/* Read status register. */
unsigned char ym2612_read(void *chip, int a);

/* Generate 'length' stereo sample pairs into bufL and bufR.
 * Buffers must be at least 'length' INT32 elements each. */
int   ym2612_update_one(void *chip, UINT32 length,
                        INT32 *bufL, INT32 *bufR);

#ifdef __cplusplus
}
#endif
