//===-- loom/bits.h -------------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_BITS_H_
#define _LOOM_BITS_H_

#include "loom/config.h"
#include "loom/linkage.h"

#include "loom/types.h"

LOOM_BEGIN_EXTERN_C

#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  #include <intrin.h>
  #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86 || \
      LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
    #pragma intrinsic(_BitScanForward)
    #pragma intrinsic(_BitScanReverse)
    #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
      #pragma intrinsic(_BitScanForward64)
      #pragma intrinsic(_BitScanReverse64)
    #endif
  #endif
#endif

/// \def LOOM_BITS_TO_BYTES
/// \brief Computes the number of bytes required to represent `n` bits.
#define LOOM_BITS_TO_BYTES(n) \
  (((n) + 7) / 8)

/// \def LOOM_BYTES_TO_BITS
/// \brief Computes the number of bits represented by `n` bytes.
#define LOOM_BYTES_TO_BITS(n) \
  ((n) * 8)

/// Counts number of leading zeros.
static LOOM_INLINE unsigned loom_clz_u32(loom_uint32_t v) {
#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  unsigned long bit;
  return _BitScanReverse((unsigned long *)&bit, v) ? (31 - bit) : 32;
#elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
      LOOM_COMPILER == LOOM_COMPILER_GCC
  return v ? __builtin_clzl(v) : 32;
#endif
}

/// Counts number of trailing zeros.
static LOOM_INLINE unsigned loom_ctz_u32(loom_uint32_t v) {
#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  unsigned long bit;
  return _BitScanForward((unsigned long *)&bit, v) ? bit : 32;
#elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
      LOOM_COMPILER == LOOM_COMPILER_GCC
  return v ? __builtin_ctzl(v) : 32;
#endif
}

#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  /// Counts number of leading zeros.
  static LOOM_INLINE unsigned loom_clz_u64(loom_uint64_t v) {
  #if LOOM_COMPILER == LOOM_COMPILER_MSVC
    unsigned long bit;
    return _BitScanReverse64(&bit, v) ? (63 - bit) : 64;
  #elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
        LOOM_COMPILER == LOOM_COMPILER_GCC
    return v ? __builtin_clzl(v) : 64;
  #endif
  }

  /// Counts number of trailing zeros.
  static LOOM_INLINE unsigned loom_ctz_u64(loom_uint64_t v) {
  #if LOOM_COMPILER == LOOM_COMPILER_MSVC
    unsigned long bit;
    return _BitScanForward64(&bit, v) ? bit : 64;
  #elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
        LOOM_COMPILER == LOOM_COMPILER_GCC
    return v ? __builtin_ctzl(v) : 32;
  #endif
  }
#endif

#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
  #define loom_clz_native(v) loom_clz_u32(v)
  #define loom_ctz_native(v) loom_ctz_u32(v)
#elif LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  #define loom_clz_native(v) loom_clz_u64(v)
  #define loom_ctz_native(v) loom_ctz_u64(v)
#endif

LOOM_END_EXTERN_C

#endif // _LOOM_BITS_H_
