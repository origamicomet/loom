//===-- loom/lock.c -------------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#include "loom/lock.h"

#include "loom/support.h"

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  #include <windows.h>
#endif

LOOM_BEGIN_EXTERN_C

struct loom_lock {
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  CRITICAL_SECTION cs;
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
};

loom_lock_t *loom_lock_create(void) {
  loom_lock_t *lock =
    (loom_lock_t *)calloc(1, sizeof(loom_lock_t));

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  InitializeCriticalSection(&lock->cs);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif

  return lock;
}

void loom_lock_destroy(loom_lock_t *lock) {
  loom_assert_debug(lock != NULL);

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  DeleteCriticalSection(&lock->cs);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif

  free((void *)lock);
}

void loom_lock_acquire(loom_lock_t *lock) {
  loom_assert_debug(lock != NULL);

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  EnterCriticalSection(&lock->cs);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
}

void loom_lock_release(loom_lock_t *lock) {
  loom_assert_debug(lock != NULL);
  
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  LeaveCriticalSection(&lock->cs);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
}

LOOM_END_EXTERN_C
