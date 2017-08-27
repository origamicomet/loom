//===-- loom/prng.h -------------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_PRNG_H_
#define _LOOM_PRNG_H_

#include "loom/config.h"
#include "loom/linkage.h"

#include "loom/types.h"

LOOM_BEGIN_EXTERN_C

typedef struct loom_prng loom_prng_t;

extern LOOM_LOCAL
  loom_prng_t *loom_prng_create(void);

extern LOOM_LOCAL
  void loom_prng_destroy(loom_prng_t *prng);

extern LOOM_LOCAL
  loom_uint32_t loom_prng_grab_u32(loom_prng_t *prng);

LOOM_END_EXTERN_C

#endif // _LOOM_PRNG_H_
