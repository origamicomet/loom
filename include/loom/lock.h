//===-- loom/lock.h -------------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_LOCK_H_
#define _LOOM_LOCK_H_

#include "loom/config.h"
#include "loom/linkage.h"

#include "loom/types.h"

LOOM_BEGIN_EXTERN_C

typedef struct loom_lock loom_lock_t;

extern LOOM_LOCAL
  loom_lock_t *loom_lock_create(void);

extern LOOM_LOCAL
  void loom_lock_destroy(loom_lock_t *lock);

extern LOOM_LOCAL
  void loom_lock_acquire(loom_lock_t *lock);

extern LOOM_LOCAL
  void loom_lock_release(loom_lock_t *lock);

LOOM_END_EXTERN_C

#endif // _LOOM_LOCK_H_
