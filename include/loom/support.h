//===-- loom/support.h ----------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_SUPPORT_H_
#define _LOOM_SUPPORT_H_

#include "loom/config.h"
#include "loom/linkage.h"

/// \def LOOM_INLINE
/// \brief Code should be inlined.
#if defined(DOXYGEN)
  #define LOOM_INLINE
#else
  #if defined(_MSC_VER)
    #define LOOM_INLINE __forceinline
  #elif defined(__clang__) || defined(__GNUC__)
    #define LOOM_INLINE __inline __attribute__ ((always_inline))
  #endif
#endif

/// \def LOOM_TRAP
/// \brief Errant, but reachable, code path.
#if defined(DOXYGEN)
  #define LOOM_TRAP()
#else
  #if defined(_MSC_VER)
    #define LOOM_TRAP() __debugbreak()
  #elif defined(__GNUC__)
    #define LOOM_TRAP() __builtin_trap()
  #endif
#endif

/// \def LOOM_UNREACHABLE
/// \brief Code is unreachable.
#if defined(DOXYGEN)
  #define LOOM_UNREACHABLE()
#else
  #if defined(_MSC_VER)
    #define LOOM_UNREACHABLE() __assume(0)
  #elif defined(__clang__)
    #define LOOM_UNREACHABLE() __builtin_unreachable()
  #elif defined(__GNUC__)
    #if __GNUC_VERSION__ >= 40500
      #define LOOM_UNREACHABLE() __builtin_unreachable()
    #else
      #include <signal.h>
      #define LOOM_UNREACHABLE() do { ::signal(SIGTRAP); } while(0, 0)
    #endif
  #endif
#endif

/// \def LOOM_THREAD_LOCAL
/// \brief Marks a static variable as "thread local" meaning each thread
/// has its own unique copy.
#if defined(DOXYGEN)
  #define LOOM_THREAD_LOCAL
#else
  #if (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_THREADS__)
    #define LOOM_THREAD_LOCAL _Thread_local
  #else
    #if defined(_MSC_VER)
      #define LOOM_THREAD_LOCAL __declspec(thread)
    #elif defined(__clang__) || defined(__GNUC__)
      #define LOOM_THREAD_LOCAL __thread
    #endif
  #endif
#endif

#include "loom/assert.h"

#endif // _LOOM_SUPPORT_H_
