//===-- loom/assert.h -----------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_ASSERT_H_
#define _LOOM_ASSERT_H_

#include "loom/config.h"
#include "loom/linkage.h"

#include <assert.h>

LOOM_BEGIN_EXTERN_C

#if defined(DOXYGEN)
  #define loom_assert_debug(...)
#else
  #if LOOM_CONFIGURATION == LOOM_CONFIGURATION_DEBUG
    #define loom_assert_debug(predicate) assert(predicate)
  #else
    #define loom_assert_debug(...)
  #endif
#endif

LOOM_END_EXTERN_C

#endif // _LOOM_ASSERT_H_
