//===-- loom/thread.c -----------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#include "loom/thread.h"

#include "loom/support.h"

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  #include <windows.h>
#endif

#if LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
    LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  #include <limits.h>
  #include <unistd.h>
  #include <pthread.h>
  #include <sched.h>
#endif

#if LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  // We use Linux specific process control to set thread name from within our
  // entry point, rather than setting thread name in `loom_thread_spawn`.
  #include <sys/prctl.h>
#endif

LOOM_BEGIN_EXTERN_C

struct loom_thread {
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  HANDLE handle;
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  pthread_t handle;
#endif
};

typedef struct loom_thread_start_info {
  char name[17];
  loom_thread_entry_point_fn entry_point;
  void *entry_point_arg;
} loom_thread_start_info_t;

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  #if LOOM_COMPILER == LOOM_COMPILER_MSVC
    #pragma pack(push, 8)
    struct THREAD_NAME_INFO {
      DWORD dwType;
      LPCSTR szName;
      DWORD dwThreadID;
      DWORD dwFlags;
    };
    #pragma pack(pop)
  #endif

  static DWORD WINAPI thread_entry_point(LPVOID thread_start_info_ptr) {
    const loom_thread_start_info_t *thread_start_info =
      (const loom_thread_start_info_t *)thread_start_info_ptr;

    loom_thread_entry_point_fn entry_point = thread_start_info->entry_point;
    void *entry_point_arg = thread_start_info->entry_point_arg;

  #if LOOM_CONFIGURATION == LOOM_CONFIGURATION_DEBUG
    // TODO(mtwilliams): Raise exception regardless of compiler?
    #if LOOM_COMPILER == LOOM_COMPILER_MSVC
      THREAD_NAME_INFO thread_name_info;
      thread_name_info.dwType = 0x1000;
      thread_name_info.szName = &thread_start_info->name[0];
      thread_name_info.dwThreadID = GetCurrentThreadId();
      thread_name_info.dwFlags = 0;

      #pragma warning(push)
      #pragma warning(disable: 6320 6322)
        __try {
          RaiseException(0x406D1388, 0, sizeof(THREAD_NAME_INFO)/sizeof(ULONG_PTR), (ULONG_PTR *)&thread_name_info);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        }
      #pragma warning(pop)
    #endif
  #endif

    free(thread_start_info_ptr);

    entry_point(entry_point_arg);

    return 0x00000000ul;
  }
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  static void *loom_thread_entry_point(void *thread_start_info_ptr) {
    const loom_thread_start_info_t *thread_start_info =
      (const loom_thread_start_info_t *)thread_start_info_ptr;

    loom_thread_entry_point_fn entry_point = thread_start_info->entry_point;
    void *entry_point_arg = thread_start_info->entry_point_arg;

  #if LOOM_CONFIGURATION == LOOM_CONFIGURATION_DEBUG
    #if LOOM_PLATFORM == LOOM_PLATFORM_MAC
      pthread_setname_np(&thread_start_info->name[0]);
    #elif LOOM_PLATFORM == LOOM_PLATFORM_LINUX
      prctl(PR_SET_NAME, &thread_start_info->name[0], 0, 0, 0);
    #endif
  #endif

    free(thread_start_info_ptr);

    entry_point(entry_point_arg);

    return NULL;
  }
#endif

#if LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
    LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  static size_t round_and_bound_stack(size_t stack) {
    if (stack <= PTHREAD_STACK_MIN)
      return PTHREAD_STACK_MIN;

    const size_t page = getpagesize();

    if (stack % page)
      // Round up to nearest page.
      stack += page - (stack % page);

    return stack;
  }
#endif

loom_thread_t *loom_thread_spawn(loom_thread_entry_point_fn entry_point,
                                 void *entry_point_arg,
                                 const loom_thread_options_t *options) {
  loom_thread_t *thread =
    (loom_thread_t *)calloc(1, sizeof(loom_thread_t));

  loom_thread_start_info_t *thread_start_info =
    (loom_thread_start_info_t *)calloc(1, sizeof(loom_thread_start_info_t));

  if (options->name) {
    // NOTE(mtwilliams): Silently truncates thread name.
    static const loom_size_t length = sizeof(thread_start_info->name);
    strncpy(&thread_start_info->name[0], options->name, length - 1);
    thread_start_info->name[length - 1] = '\0';
  } else {
    thread_start_info->name[0] = '\0';
  }

  thread_start_info->entry_point = entry_point;
  thread_start_info->entry_point_arg = entry_point_arg;

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  thread->handle = CreateThread(NULL,
                                options->stack,
                                (LPTHREAD_START_ROUTINE)&thread_entry_point,
                                (LPVOID)thread_start_info,
                                CREATE_SUSPENDED,
                                NULL);

  if (~options->affinity != 0) {
    // NOTE(mtwilliams): This will truncate to 32 bits, and therefore 32 cores,
    // if on 32 bit. There's not much we can do.
    SetThreadAffinityMask(thread->handle, (DWORD_PTR)options->affinity);

    if (options->affinity) {
      if (options->affinity & (options->affinity - 1)) {
        DWORD processor;

        // TODO(mtwilliams): Replace intrinsics with common utility functions.
      #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
        _BitScanForward(&processor, options->affinity);
      #else LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
        _BitScanForward64(&processor, options->affinity);
      #endif

        SetThreadIdealProcessor(thread->handle, processor);
      }
    }
  }

  // Be free!
  ResumeThread(thread->handle);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  pthread_attr_t attributes;

  pthread_attr_init(&attributes);

  pthread_attr_setstacksize(&attributes, round_and_bound_stack(options->stack));

#if LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  cpu_set_t cpus;

  if (~options->affinity) {
    CPU_ZERO(&cpus);

  #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
    static const unsigned n = 32;
  #else LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
    static const unsigned n = 64;
  #endif

    // Naive, but simpler than iterating set bits.
    for (unsigned cpu = 0; cpu < n; ++)
      if ((options->affinity >> cpu) & 1)
        CPU_SET(cpu, &cpus);
  } else {
    // HACK(mtwilliams): Technically opaque, so we shouldn't do this.
    memset((void *)&cpus, ~0, sizeof(cpus));
  }

  pthread_attr_setaffinity_np(&attributes, CPU_SETSIZE, &cpus);
#endif

  pthread_create(&thread->handle,
                 &attributes,
                 &thread_entry_point,
                 (void *)thread_start_info);

  pthread_attr_destroy(&attributes);

  #if LOOM_PLATFORM == LOOM_PLATFORM_MAC
    // NOTE(mtwilliams): Mach doesn't allow explicit thread placement, so we
    // have to ignore affinity. However, we can use "affinity sets" (as of
    // Mac OS X 10.5) as a hint to Mach to schedule a thread on a particular
    // logical processor.
    if (options->affinity != ~0ull) {
      if (options->affinity & (options->affinity - 1)) {
        // TODO(mtwilliams): Hint for a specific processor.
      }
    }
  #endif
#endif

  return thread;
}

void loom_thread_join(loom_thread_t *thread) {
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  // INVESTIGATE(mtwilliams): Should we check that this returns `WAIT_OBJECT_0`?
  WaitForSingleObject(thread->handle, INFINITE);
  CloseHandle(thread->handle);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  pthread_join(thread->handle, NULL);
#endif

  free((void *)thread);
}

void loom_thread_detach(loom_thread_t *thread) {
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  CloseHandle(thread->handle);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  pthread_detach(thread->handle);
#endif

  free((void *)thread);
}

void loom_thread_terminate(loom_thread_t *thread) {
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  // I drink your milkshake...
  TerminateThread(thread->handle, 0x0);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  pthread_cancel(thread->handle);
#endif

  free((void *)thread);
}

void loom_thread_yield(void) {
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  if (SwitchToThread() == 0)
    // Windows didn't schedule another thread, so try to force it.
    Sleep(0);
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC || \
      LOOM_PLATFORM == LOOM_PLATFORM_LINUX
  sched_yield();
#endif
}

LOOM_END_EXTERN_C
