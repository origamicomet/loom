//===-- loom/atomics.h ----------------------------------*- mode: C++11 -*-===//
//
//                            __
//                           |  |   ___ ___ _____
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_ATOMICS_H_
#define _LOOM_ATOMICS_H_

#include "loom/config.h"
#include "loom/linkage.h"

#include "loom/types.h"

#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  #include <intrin.h>

  #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86 || \
      LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
    #pragma intrinsic(_InterlockedIncrement)
    #pragma intrinsic(_InterlockedDecrement)
    #pragma intrinsic(_InterlockedCompareExchange)
    #pragma intrinsic(_interlockedbittestandset)
    #pragma intrinsic(_interlockedbittestandreset)

    #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
      #pragma intrinsic(_InterlockedIncrement64)
      #pragma intrinsic(_InterlockedDecrement64)
      #pragma intrinsic(_InterlockedCompareExchange64)
      #pragma intrinsic(_interlockedbittestandset64)
      #pragma intrinsic(_interlockedbittestandreset64)
    #endif
  #endif

  #pragma intrinsic(_ReadWriteBarrier)
#elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
      LOOM_COMPILER == LOOM_COMPILER_GCC
#endif

LOOM_BEGIN_EXTERN_C

// NOTE(mtwilliams): Aligned loads and stores are always atomic on x86/x86_64.

// PERF(mtwilliams): As of Visual Studo 2005, volatile implies acquire and
// release semantics, which may be expensive on certain plaforms.

#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  #define loom_atomic_acquire() _ReadWriteBarrier()
  #define loom_atomic_release() _ReadWriteBarrier()
  #define loom_atomic_fence() _ReadWriteBarrier()
  #define loom_atomic_barrier() MemoryBarrier()
#elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
      LOOM_COMPILER == LOOM_COMPILER_GCC
  #define loom_atomic_acquire() asm volatile("" ::: "memory")
  #define loom_atomic_release() asm volatile("" ::: "memory")
  #define loom_atomic_fence() asm volatile("" ::: "memory")
  #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
    #define loom_atomic_barrier() asm volatile("lock; orl $0, (%%esp)" ::: "memory")
  #elif LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
    #define loom_atomic_barrier() asm volatile("lock; orl $0, (%%rsp)" ::: "memory")
  #endif
#endif

static LOOM_INLINE loom_uint32_t loom_atomic_load_u32(const volatile loom_uint32_t *m) {
#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86 || \
    LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  return *m;
#endif
}

static LOOM_INLINE void loom_atomic_store_u32(volatile loom_uint32_t *m, loom_uint32_t v) {
#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86 || \
    LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  *m = v;
#endif
}

static LOOM_INLINE loom_uint32_t loom_atomic_cmp_and_xchg_u32(volatile loom_uint32_t *m,
                                                              loom_uint32_t expected,
                                                              loom_uint32_t desired) {
#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  return _InterlockedCompareExchange((volatile long *)m, desired, expected);
#elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
      LOOM_COMPILER == LOOM_COMPILER_GCC
#endif
}

static LOOM_INLINE loom_uint32_t loom_atomic_incr_u32(volatile loom_uint32_t *m) {
#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  return _InterlockedIncrement((volatile long *)m);
#elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
      LOOM_COMPILER == LOOM_COMPILER_GCC
#endif
}

static LOOM_INLINE loom_uint32_t loom_atomic_decr_u32(volatile loom_uint32_t *m) {
#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  return _InterlockedDecrement((volatile long *)m);
#elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
      LOOM_COMPILER == LOOM_COMPILER_GCC
#endif
}

static LOOM_INLINE unsigned loom_atomic_set_u32(volatile loom_uint32_t *m, unsigned bit) {
#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  return _interlockedbittestandset((volatile long *)m, bit);
#elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
      LOOM_COMPILER == LOOM_COMPILER_GCC
#endif
}

static LOOM_INLINE unsigned loom_atomic_reset_u32(volatile loom_uint32_t *m, unsigned bit) {
#if LOOM_COMPILER == LOOM_COMPILER_MSVC
  return _interlockedbittestandreset((volatile long *)m, bit);
#elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
      LOOM_COMPILER == LOOM_COMPILER_GCC
#endif
}

// NOTE(mtwilliams): We only support 64-bit atomics on x86_64 rather than
// falling back to `cmpxchg8b`.

#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  static LOOM_INLINE loom_uint64_t loom_atomic_load_u64(const volatile loom_uint64_t *m) {
  #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86 || \
      LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
    return *m;
  #endif
  }

  static LOOM_INLINE void loom_atomic_store_u64(volatile loom_uint64_t *m, loom_uint64_t v) {
  #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86 || \
      LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
    *m = v;
  #endif
  }

  static LOOM_INLINE loom_uint64_t loom_atomic_cmp_and_xchg_u64(volatile loom_uint64_t *m,
                                                                loom_uint64_t expected,
                                                                loom_uint64_t desired) {
  #if LOOM_COMPILER == LOOM_COMPILER_MSVC
    return _InterlockedCompareExchange64((volatile __int64 *)m, desired, expected);
  #elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
        LOOM_COMPILER == LOOM_COMPILER_GCC
  #endif
  }

  static LOOM_INLINE loom_uint64_t loom_atomic_incr_u64(volatile loom_uint64_t *m) {
  #if LOOM_COMPILER == LOOM_COMPILER_MSVC
    return _InterlockedIncrement64((volatile __int64 *)m);
  #elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
        LOOM_COMPILER == LOOM_COMPILER_GCC
  #endif
  }

  static LOOM_INLINE loom_uint64_t loom_atomic_decr_u64(volatile loom_uint64_t *m) {
  #if LOOM_COMPILER == LOOM_COMPILER_MSVC
    return _InterlockedDecrement64((volatile __int64 *)m);
  #elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
        LOOM_COMPILER == LOOM_COMPILER_GCC
  #endif
  }

  static LOOM_INLINE unsigned loom_atomic_set_u64(volatile loom_uint64_t *m, unsigned bit) {
  #if LOOM_COMPILER == LOOM_COMPILER_MSVC
    return _interlockedbittestandset64((volatile __int64 *)m, bit);
  #elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
        LOOM_COMPILER == LOOM_COMPILER_GCC
  #endif
  }

  static LOOM_INLINE unsigned loom_atomic_reset_u64(volatile loom_uint64_t *m, unsigned bit) {
  #if LOOM_COMPILER == LOOM_COMPILER_MSVC
    return _interlockedbittestandreset64((volatile __int64 *)m, bit);
  #elif LOOM_COMPILER == LOOM_COMPILER_CLANG || \
        LOOM_COMPILER == LOOM_COMPILER_GCC
  #endif
  }
#endif

static LOOM_INLINE void *loom_atomic_load_ptr(void *const volatile *m) {
#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86 || \
    LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  return *m;
#endif
}

static LOOM_INLINE void loom_atomic_store_ptr(void *volatile *m, void *v) {
#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86 || \
    LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  *m = v;
#endif
}

#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
  #define loom_atomic_load_native(m) loom_atomic_load_u32(m)
  #define loom_atomic_store_native(m, v) loom_atomic_store_u32(m, v)
  #define loom_atomic_cmp_and_xchg_native(m, expected, desired) loom_atomic_cmp_and_xchg_u32(m, expected, desired)
  #define loom_atomic_incr_native(m) loom_atomic_incr_u32(m)
  #define loom_atomic_decr_native(m) loom_atomic_decr_u32(m)
  #define loom_atomic_set_native(m, bit) loom_atomic_set_u32(m, bit)
  #define loom_atomic_reset_native(m, bit) loom_atomic_reset_u32(m, bit)
#elif LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  #define loom_atomic_load_native(m) loom_atomic_load_u64(m)
  #define loom_atomic_store_native(m, v) loom_atomic_store_u64(m, v)
  #define loom_atomic_cmp_and_xchg_native(m, expected, desired) loom_atomic_cmp_and_xchg_u64(m, expected, desired)
  #define loom_atomic_incr_native(m) loom_atomic_incr_u64(m)
  #define loom_atomic_decr_native(m) loom_atomic_decr_u64(m)
  #define loom_atomic_set_native(m, bit) loom_atomic_set_u64(m, bit)
  #define loom_atomic_reset_native(m, bit) loom_atomic_reset_u64(m, bit)
#endif

LOOM_END_EXTERN_C

#endif // _LOOM_ATOMICS_H_
