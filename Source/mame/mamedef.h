#pragma once
// Minimal MAME type definitions required by the YM2612 emulator core.
// Sourced from the MAME project (GPL v2+).

#include <stdint.h>

typedef signed char        INT8;
typedef signed short       INT16;
typedef signed int         INT32;
typedef signed long long   INT64;
typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef unsigned long long UINT64;

typedef INT32 stream_sample_t;

#ifndef INLINE
#  if defined(_MSC_VER)
#    define INLINE __forceinline
#  else
#    define INLINE static inline __attribute__((always_inline))
#  endif
#endif

#define BUILD_YM2612 1
