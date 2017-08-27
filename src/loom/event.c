//===-- loom/event.c ------------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#include "loom/event.h"

#include "loom/support.h"

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  #include <windows.h>
#endif

LOOM_BEGIN_EXTERN_C

struct loom_event {
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  HANDLE handle;
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
};

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  static DWORD timeout_to_windows(unsigned timeout) {
    if (timeout == -1)
      return INFINITE;
    return timeout;
  }
#endif

loom_event_t *loom_event_create(loom_bool_t manual) {
  loom_event_t *event =
    (loom_event_t *)calloc(1, sizeof(loom_event_t));

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  event->handle = CreateEvent(NULL, manual, FALSE, NULL);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif

  return event;
}

void loom_event_destroy(loom_event_t *event) {
  loom_assert_debug(event != NULL);

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  CloseHandle(event->handle);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif

  free((void *)event);
}

void loom_event_signal(loom_event_t *event) {
  loom_assert_debug(event != NULL);

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  SetEvent(event->handle);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
}

void loom_event_unsignal(loom_event_t *event) {
  loom_assert_debug(event != NULL);
  
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  ResetEvent(event->handle);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
}

loom_bool_t loom_event_wait(loom_event_t *event,
                            unsigned timeout) {
  loom_assert_debug(event != NULL);
  
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  return (WaitForSingleObject(event->handle, timeout_to_windows(timeout)) == WAIT_OBJECT_0);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
}

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  static DWORD wait_on_multiple(DWORD n,
                                loom_event_t **events,
                                DWORD timeout,
                                BOOL all) {
    loom_assert_debug(n <= MAXIMUM_WAIT_OBJECTS);

    HANDLE handles[MAXIMUM_WAIT_OBJECTS];

    for (DWORD handle = 0; handle < n; ++handle)
      handles[handle] = events[handle]->handle;

    return WaitForMultipleObjects(n, &handles[0], all, timeout_to_windows(timeout));
  }
#endif

unsigned loom_event_wait_on_any(unsigned n,
                                loom_event_t **events,
                                unsigned timeout) {
  loom_assert_debug(events != NULL);

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  const DWORD result = wait_on_multiple(n, events, timeout, FALSE);

  if ((result >= WAIT_OBJECT_0) && (result <= (WAIT_OBJECT_0 + n - 1)))
    return (result - WAIT_OBJECT_0) + 1;

  return 0;
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
}

loom_bool_t loom_event_wait_on_all(unsigned n,
                                   loom_event_t **events,
                                   unsigned timeout) {
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  const DWORD result = wait_on_multiple(n, events, timeout, TRUE);

  if (result == WAIT_TIMEOUT)
    return false;

  if (result == WAIT_FAILED)
    return false;

  return true;
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
}

LOOM_END_EXTERN_C
