//===-- loom.h ------------------------------------------*- mode: C++11 -*-===//
//
//                            __
//                           |  |   ___ ___ _____
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_H_
#define _LOOM_H_

#include "loom/config.h"
#include "loom/linkage.h"
#include "loom/types.h"
#include "loom/support.h"

LOOM_BEGIN_EXTERN_C

typedef enum loom_kind_of_work loom_kind_of_work_t;

typedef struct loom_work loom_work_t;

typedef struct loom_permit loom_permit_t;

typedef enum loom_task_flags loom_task_flags_t;
typedef struct loom_task loom_task_t;

typedef struct loom_handle loom_handle_t;

/// Type of work.
enum loom_kind_of_work {
  LOOM_WORK_NONE = 0,
  LOOM_WORK_CPU  = 1,

  // Force `loom_uint32_t` storage and alignment.
  __LOOM_KIND_OF_WORK_FORCE_STORAGE_AND_ALIGNMENT__ = 0x7ffffffful
};

typedef void (*loom_kernel_fn)(void *);

/// A schedulable unit of work.
struct loom_work {
  loom_kind_of_work_t kind;

  union {
    struct {
      loom_kernel_fn kernel;
      void *data;
    } cpu;
  };
};

/// \brief Specifies a task that is permitted to run by another task.
///
/// \details Permits are a simplified implementation of reverse dependencies,
/// as described by Charles Bloom in his blog posts.
///
/// By using permits the following is achieved:
///
///  (1) Simpler code. Scheduling and permitting code is substantially smaller
///      and simpler than normal dependencies.
///
///  (2) Guaranteed scheduling without overhead. By maintaining references to
///      any non-queued tasks in a "permit" any submitted tasks are
///      guaranteed to be scheduled without an additional data-structure.
///      Either by being inserted into a woker's queue immediately, or by being
///      inserted later upon the completion of the final permitting task.
///
///  (3) Improved scheduling characteristics. Permits adhere to Charles Bloom's
///      guiding principles:
///
///        1. Always yield worker threads when they cannot schedule work.
///        2. Never have a worker thread sleep when it can schedule work.
///        3. Never wake a worker thread only to yield it immediately.
///
///  (4) Reduced memory footprint. Normal dependencies require more memory
///      due to additional members in task descriptions and the use of empty
///      parent tasks to collect dependencies for tasks with more than `n`
///      dependencies.
///
struct loom_permit {
  loom_permit_t *next;
  loom_task_t *task;
};

/// Various flags that affects task behavior.
enum loom_task_flags {
};

// TODO(mtwilliams): Optimize number of embedded permits.
#ifndef LOOM_EMBEDDED_PERMITS
  #define LOOM_EMBEDDED_PERMITS 1
#endif

/// A schedulable unit of work and its permits.
struct loom_task {
  /// Globally unique identifier.
  loom_uint32_t id;

  /// \copydoc loom_task_flags_t
  loom_uint32_t flags;

  /// Work to perform.
  loom_work_t work;

  /// \brief Linked-list of tasks blocked by this task.
  ///
  /// \note The first few permits are allocated along with the task to improve
  ///       data locality.
  ///
  loom_permit_t permits[LOOM_EMBEDDED_PERMITS];

  /// Number of tasks blocked by this task.
  loom_uint32_t blocks;

  /// Number of outstanding tasks blocking this task.
  loom_uint32_t blockers;

  /// Decremented after completion.
  loom_uint32_t *barrier;
};

struct loom_handle {
#if LOOM_CONFIGURATION == LOOM_CONFIGURATION_DEBUG
  // We check for stale handles in debug builds to ease development.
  loom_uint32_t index;
  loom_uint32_t id;
#else
  void *opaque;
#endif
};

extern const loom_handle_t LOOM_INVALID_HANDLE;

typedef void (*loom_prologue_fn)(const loom_task_t *task,
                                 void *context);

typedef void (*loom_epilogue_fn)(const loom_task_t *task,
                                 void *context);

typedef struct loom_prologue {
  loom_prologue_fn fn;
  void *context;
} loom_prologue_t;

typedef struct loom_epilogue {
  loom_epilogue_fn fn;
  void *context;
} loom_epilogue_t;

/// \define LOOM_WORKER_LIMIT
/// \brief Maximum number of worker threads at any point in time.
#ifndef LOOM_WORKER_LIMIT
  #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
    #define LOOM_WORKER_LIMIT (32 - 1)
  #elif LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
    #define LOOM_WORKER_LIMIT (64 - 1)
  #endif
#endif

typedef struct loom_options {
  /// Number of worker threads to spawn.
  ///
  /// \note Setting this to `-n` will result in a worker thread being spawned
  ///       for each core minus `n`, with a maximum of `LOOM_WORKER_LIMIT`
  ///       worker threads being spawned.
  ///
  loom_int32_t workers;

  /// Indicates that you'll routinely call `loom_do_some_work` or similar on
  /// the main thread.
  ///
  /// \warning Specifying this flag but failing to do so can cause hangs.
  ///
  loom_bool_t main_thread_does_work;

  /// A callback to invoke prior to scheduling a task.
  loom_prologue_t prologue;

  /// A callback to invoke after scheduling a task.
  loom_epilogue_t epilogue;

  /// Size of task pool.
  loom_size_t tasks;

  /// Size of permit pool.
  loom_size_t permits;

  /// Size of work queues.
  loom_size_t queue;
} loom_options_t;

extern LOOM_PUBLIC
  void loom_initialize(const loom_options_t *options);

extern LOOM_PUBLIC
  void loom_shutdown(void);

extern LOOM_PUBLIC
  void loom_bring_up_workers(unsigned n);

extern LOOM_PUBLIC
  void loom_bring_down_workers(unsigned n);

extern LOOM_PUBLIC
  loom_handle_t loom_empty(loom_uint32_t flags);

extern LOOM_PUBLIC
  loom_handle_t loom_describe(loom_kernel_fn kernel,
                              void *data,
                              loom_uint32_t flags);

extern LOOM_PUBLIC
  void loom_permits(loom_handle_t task,
                    loom_handle_t permitee);

extern LOOM_PUBLIC
  void loom_kick(loom_handle_t task);

extern LOOM_PUBLIC
  void loom_kick_n(unsigned n,
                   const loom_handle_t *tasks);

/// \brief Kicks a task and waits for it to be completed.
extern LOOM_PUBLIC
  void loom_kick_and_wait(loom_handle_t task);

/// \brief Kicks all tasks and waits for all to be completed.
extern LOOM_PUBLIC
  void loom_kick_and_wait_n(unsigned n,
                            const loom_handle_t *tasks);

/// \brief Kicks a task and does work while waiting for it to be completed.
extern LOOM_PUBLIC
  void loom_kick_and_do_work_while_waiting(loom_handle_t task);

/// \brief Kicks all tasks and does work while waiting for all to be completed.
extern LOOM_PUBLIC
  void loom_kick_and_do_work_while_waiting_n(unsigned n,
                                             const loom_handle_t *tasks);

/// \brief Schedules an available task, if there are any.
/// \warning You should only call this from the main thread!
/// \returns If a task was completed, i.e. if some work was performed.
extern LOOM_PUBLIC
  loom_bool_t loom_do_some_work(void);

LOOM_END_EXTERN_C

#endif // _LOOM_H_
