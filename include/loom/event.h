//===-- loom/event.h ------------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_EVENT_H_
#define _LOOM_EVENT_H_

#include "loom/config.h"
#include "loom/linkage.h"

#include "loom/types.h"

LOOM_BEGIN_EXTERN_C

typedef struct loom_event loom_event_t;

extern LOOM_LOCAL
  loom_event_t *loom_event_create(loom_bool_t manual);

extern LOOM_LOCAL
  void loom_event_destroy(loom_event_t *event);

extern LOOM_LOCAL
  void loom_event_signal(loom_event_t *event);

extern LOOM_LOCAL
  void loom_event_unsignal(loom_event_t *event);

extern LOOM_LOCAL
  loom_bool_t loom_event_wait(loom_event_t *event,
                              unsigned timeout);

extern LOOM_LOCAL
  unsigned loom_event_wait_on_any(unsigned n,
                                  loom_event_t **events,
                                  unsigned timeout);

extern LOOM_LOCAL
  loom_bool_t loom_event_wait_on_all(unsigned n,
                                     loom_event_t **events,
                                     unsigned timeout);

LOOM_END_EXTERN_C

#endif // _LOOM_EVENT_H_
