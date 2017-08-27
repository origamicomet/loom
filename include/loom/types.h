//===-- loom/types.h ------------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_TYPES_H_
#define _LOOM_TYPES_H_

#include "loom/config.h"
#include "loom/linkage.h"

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

// TODO(mtwilliams): Fallbacks for older versions of Visual C/C++.

LOOM_BEGIN_EXTERN_C

typedef size_t loom_size_t;

typedef int8_t loom_int8_t;
typedef uint8_t loom_uint8_t;

typedef int16_t loom_int16_t;
typedef uint16_t loom_uint16_t;

typedef int32_t loom_int32_t;
typedef uint32_t loom_uint32_t;

typedef int64_t loom_int64_t;
typedef uint64_t loom_uint64_t;

#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
  typedef loom_uint32_t loom_native_t;
#elif LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  typedef loom_uint64_t loom_native_t;
#endif

typedef bool loom_bool_t;

LOOM_END_EXTERN_C

#endif // _LOOM_TYPES_H_
