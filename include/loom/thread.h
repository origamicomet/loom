//===-- loom/thread.h -----------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_THREAD_H_
#define _LOOM_THREAD_H_

#include "loom/config.h"
#include "loom/linkage.h"

#include "loom/types.h"

LOOM_BEGIN_EXTERN_C

typedef struct loom_thread_options {
  /// A name to associate with the new thread.
  ///
  /// \note The semantics of this differs per platform. Usually, threads
  /// are nameless objects. Thus this only makes sense in the context of a
  /// debugger or tooling.
  ///
  const char *name;

  /// A bitmask of all the logical cores the new thread can be scheduled on.
#if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
  loom_uint32_t affinity;
#elif LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
  loom_uint64_t affinity;
#endif

  /// The maximum size (in bytes) of the stack to provide the new thread.
  ///
  /// \note If zero, a reasonable default is chosen.
  ///
  loom_size_t stack;
} loom_thread_options_t;

typedef struct loom_thread loom_thread_t;

typedef void (*loom_thread_entry_point_fn)(void *);

extern LOOM_LOCAL
  loom_thread_t *loom_thread_spawn(loom_thread_entry_point_fn entry_point,
                                   void *entry_point_arg,
                                   const loom_thread_options_t *options);

extern LOOM_LOCAL
  void loom_thread_join(loom_thread_t *thread);

extern LOOM_LOCAL
  void loom_thread_detach(loom_thread_t *thread);

extern LOOM_LOCAL
  void loom_thread_terminate(loom_thread_t *thread);

extern LOOM_LOCAL
  void loom_thread_yield(void);

LOOM_END_EXTERN_C

#endif // _LOOM_THREAD_H_
