//===-- loom.c ------------------------------------------*- mode: C++11 -*-===//
//
//                            __
//                           |  |   ___ ___ _____
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#include "loom.h"

#include "loom/atomics.h"
#include "loom/bits.h"
#include "loom/thread.h"
#include "loom/lock.h"
#include "loom/event.h"
#include "loom/prng.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  // Used to determine processor topology when choosing number of workers.
  #include <windows.h>
#endif

LOOM_BEGIN_EXTERN_C

const loom_handle_t LOOM_INVALID_HANDLE = {
#if LOOM_CONFIGURATION == LOOM_CONFIGURATION_DEBUG
  0xffffffff, 0xffffffff
#else
  NULL
#endif
};

/// \brief A lock-free, single-producer, multiple-consumer, doubly-ended queue
/// of tasks.
///
/// \details The producer can push and pop elements in last-in first-out order,
/// while any number of consumers can steal elements in first-in first-out
/// order.
///
/// This is an implementation of the data structure described by Chase and Lev
/// in their paper "Dynamic Circular Work-Stealing Deque." A more approachable
/// description is available on Stefan Reinalter's blog.
///
/// \warning Do not pop in any thread other than the producer thread!
///
typedef struct loom_work_queue {
  loom_uint32_t top;
  loom_uint32_t bottom;

  loom_task_t **tasks;

  loom_uint32_t size;
  loom_uint32_t size_minus_one;
} loom_work_queue_t;

static loom_work_queue_t *loom_work_queue_create(loom_size_t size) {
  loom_work_queue_t *wq =
    (loom_work_queue_t *)calloc(1, sizeof(loom_work_queue_t));

  wq->top = wq->bottom = 0;

  wq->tasks = (loom_task_t **)calloc(size, sizeof(loom_task_t *));

  wq->size = size;
  wq->size_minus_one = size - 1;

  return wq;
}

void loom_work_queue_destroy(loom_work_queue_t *wq) {
  free((void *)wq->tasks);
  free((void *)wq);
}

/// Pushes @task into @wq, returning the new depth of @wq.
static loom_uint32_t loom_work_queue_push(loom_work_queue_t *wq, loom_task_t *task) {
  const loom_uint32_t bottom = loom_atomic_load_u32(&wq->bottom);
  const loom_uint32_t top = loom_atomic_load_u32(&wq->top);

  // Make sure we won't overflow the dequeue.
  loom_assert_debug((bottom - top) <= wq->size);

  loom_atomic_store_ptr((void *volatile *)&wq->tasks[bottom % wq->size], (void *)task);

  // Ensure task is published prior to advertising.
  loom_atomic_barrier();

  loom_atomic_store_u32(&wq->bottom, bottom + 1);

  return (bottom - top + 1);
}

/// Tries to pop a task from @wq.
static loom_task_t *loom_work_queue_pop(loom_work_queue_t *wq) {
  const loom_uint32_t bottom = loom_atomic_decr_u32(&wq->bottom);
  const loom_uint32_t top = loom_atomic_load_u32(&wq->top);

  if (top <= bottom) {
    // Non-empty.
    loom_task_t *task =
      (loom_task_t *)loom_atomic_load_ptr((void *volatile *)&wq->tasks[bottom % wq->size]);

    if (top != bottom)
      // Still more than one task left in the queue.
      return task;

    // This is the last task in the queue. Potential race against steal.
    if (loom_atomic_cmp_and_xchg_u32(&wq->top, top, top + 1) != top)
      task = NULL;

    loom_atomic_store_u32(&wq->bottom, top + 1);

    return task;
  } else {
    // Empty.
    loom_atomic_store_u32(&wq->bottom, top);
    return NULL;
  }
}

/// Tries to steal a task from @wq.
static loom_task_t *loom_work_queue_steal(loom_work_queue_t *wq) {
  const loom_uint32_t top = loom_atomic_load_u32(&wq->top);

  loom_atomic_acquire();

  const loom_uint32_t bottom = loom_atomic_load_u32(&wq->bottom);

  if (top < bottom) {
    // Non-empty.
    loom_task_t *task =
      (loom_task_t *)loom_atomic_load_ptr((void *volatile *)&wq->tasks[top % wq->size]);

    if (loom_atomic_cmp_and_xchg_u32(&wq->top, top, top + 1) != top)
      // Lost to a pop or another steal.
      return NULL;

    return task;
  } else {
    // Empty.
    return NULL;
  }
}

/// Returns depth of @wq.
/// \warning May change after calling.
static loom_uint32_t loom_work_queue_depth(loom_work_queue_t *wq) {
  const loom_uint32_t bottom = loom_atomic_load_u32(&wq->bottom);
  const loom_uint32_t top = loom_atomic_load_u32(&wq->top);
  return (bottom - top);
}

/// Returns true if @wq is empty.
static loom_bool_t loom_work_queue_is_empty(loom_work_queue_t *wq) {
  const loom_uint32_t bottom = loom_atomic_load_u32(&wq->bottom);
  const loom_uint32_t top = loom_atomic_load_u32(&wq->top);
  return (bottom == top);
}

typedef struct loom_free_list {
  loom_uint32_t next;
  loom_uint32_t entries[0];
} loom_free_list_t;

static loom_free_list_t *loom_free_list_alloc(loom_size_t size) {
  loom_free_list_t *fl =
    (loom_free_list_t *)calloc(size + 1, sizeof(loom_uint32_t));

  fl->next = 0;

  for (unsigned slot = 0; slot < (size - 1); ++slot)
    fl->entries[slot] = slot + 1;

  fl->entries[size - 1] = 0xffffffff;

  return fl;
}

static void loom_free_list_free(loom_free_list_t *fl) {
  free((void *)fl);
}

static void loom_free_list_push(loom_free_list_t *fl, loom_uint32_t entry) {
  while (1) {
    const loom_uint32_t prev = loom_atomic_load_u32(&fl->next);

    loom_atomic_store_u32(&fl->entries[entry], prev);

    if (loom_atomic_cmp_and_xchg_u32(&fl->next, prev, entry) != prev)
      // Retry.
      continue;

    return;
  }
}

static loom_uint32_t loom_free_list_pop(loom_free_list_t *fl) {
  while (1) {
    const loom_uint32_t entry = loom_atomic_load_u32(&fl->next);
    const loom_uint32_t next  = loom_atomic_load_u32(&fl->entries[entry]);

    if (loom_atomic_cmp_and_xchg_u32(&fl->next, entry, next) != entry)
      // Retry.
      continue;

    loom_assert_debug(entry != 0xffffffff);

    return entry;
  }
}

typedef struct loom_task_pool {
  loom_size_t size;
  loom_uint32_t id;
  loom_task_t *tasks;
  loom_free_list_t *freelist;
} loom_task_pool_t;

static loom_task_pool_t *loom_task_pool_create(loom_size_t size) {
  loom_task_pool_t *pool =
    (loom_task_pool_t *)calloc(1, sizeof(loom_task_pool_t));

  pool->size = size;

  pool->id = 0;

  pool->tasks = (loom_task_t *)calloc(size, sizeof(loom_task_t));
  pool->freelist = loom_free_list_alloc(size);

  return pool;
}

static void loom_task_pool_destroy(loom_task_pool_t *pool) {
  free((void *)pool->tasks);

  loom_free_list_free(pool->freelist);

  free((void *)pool);
}

static loom_task_t *loom_task_pool_acquire(loom_task_pool_t *pool) {
  const loom_uint32_t index = loom_free_list_pop(pool->freelist);
  loom_task_t *task = &pool->tasks[index];
  task->id = loom_atomic_incr_u32(&pool->id);
  return task;
}

static void loom_task_pool_return(loom_task_pool_t *pool,
                                  loom_task_t *task) {
  const loom_uint32_t index = task - pool->tasks;
  loom_free_list_push(pool->freelist, index);
}

typedef struct loom_permit_pool {
  loom_size_t size;
  loom_permit_t *permits;
  loom_free_list_t *freelist;
} loom_permit_pool_t;

static loom_permit_pool_t *loom_permit_pool_create(loom_size_t size) {
  loom_permit_pool_t *pool =
    (loom_permit_pool_t *)calloc(1, sizeof(loom_permit_pool_t));

  pool->size = size;

  pool->permits = (loom_permit_t *)calloc(size, sizeof(loom_permit_t));
  pool->freelist = loom_free_list_alloc(size);

  return pool;
}

static void loom_permit_pool_destroy(loom_permit_pool_t *pool) {
  free((void *)pool->permits);

  loom_free_list_free(pool->freelist);

  free((void *)pool);
}

static loom_permit_t *loom_permit_pool_acquire(loom_permit_pool_t *pool) {
  const loom_uint32_t index = loom_free_list_pop(pool->freelist);
  loom_permit_t *permit = &pool->permits[index];
  return permit;
}

static void loom_permit_pool_return(loom_permit_pool_t *pool,
                                    loom_permit_t *permit) {
  const loom_uint32_t index = (loom_size_t)(permit - pool->permits) / sizeof(loom_permit_t);
  loom_free_list_push(pool->freelist, index);
}

typedef struct loom_worker {
  loom_uint32_t id;

  // Backing system thread.
  loom_thread_t *thread;

  // Non-zero when the worker should shutdown.
  loom_uint32_t shutdown;
} loom_worker_t;

typedef struct loom_task_scheduler {
  // Held whenever performing managerial tasks.
  loom_lock_t *lock;

  loom_prologue_t prologue;
  loom_epilogue_t epilogue;

  loom_uint32_t n;

  // We have a hard limit of 31 worker threads on x86 and 63 worker threads
  // on x86_64. This isn't a limitation of the operating system, usually, but
  // has to do with the cost of manipulating the various bitfields atomically.
  loom_worker_t workers[LOOM_WORKER_LIMIT];
  loom_work_queue_t *queues[LOOM_WORKER_LIMIT + 1];

  // Bitset that tracks online workers.
  loom_native_t online;

  // Bitset used by workers to indicate excess work.
  loom_native_t work;

  // Raised whenever excess work is pushed to a work queue.
  loom_event_t *work_to_steal;

  // Whenever work is pushed to the main thread's work queue `work_to_steal`
  // should be signaled, regardless of of queue depth.
  loom_bool_t always_steal_from_main_thread;

  // Raised while one or more workers has an unhandled message.
  loom_event_t *message;

  loom_task_pool_t *tasks;
  loom_permit_pool_t *permits;

  // Work queues are lazily initialized.
  loom_size_t size_of_each_work_queue;
} loom_task_scheduler_t;

// We provide a default prologue and epilogue so we can unconditionally call.
static void default_prologue_and_epilogue(const loom_task_t *task, void *data) {
  (void)task;
  (void)data;
}

static loom_task_scheduler_t *loom_task_scheduler_create(loom_size_t tasks,
                                                         loom_size_t permits,
                                                         loom_size_t queue) {
  loom_task_scheduler_t *task_scheduler =
    (loom_task_scheduler_t *)calloc(1, sizeof(loom_task_scheduler_t));

  task_scheduler->lock = loom_lock_create();

  task_scheduler->prologue.fn = &default_prologue_and_epilogue;
  task_scheduler->epilogue.fn = &default_prologue_and_epilogue;

  task_scheduler->n = 0;

  task_scheduler->queues[0] = loom_work_queue_create(queue);

  for (unsigned worker = 0; worker < LOOM_WORKER_LIMIT; ++worker) {
    task_scheduler->workers[worker].id = worker + 1;
    task_scheduler->workers[worker].thread = NULL;
    task_scheduler->workers[worker].shutdown = 0;

    // Work queues are lazily allocated.
    task_scheduler->queues[worker + 1] = NULL;
  }

  task_scheduler->online = 1;
  task_scheduler->work   = 0;

  task_scheduler->work_to_steal = loom_event_create(false);

  task_scheduler->message = loom_event_create(true);

  task_scheduler->tasks = loom_task_pool_create(tasks);
  task_scheduler->permits = loom_permit_pool_create(permits);

  task_scheduler->size_of_each_work_queue = queue;

  return task_scheduler;
}

static void loom_task_scheduler_destroy(loom_task_scheduler_t *task_scheduler) {
  loom_lock_destroy(task_scheduler->lock);

  for (unsigned worker = 0; worker < LOOM_WORKER_LIMIT; ++worker)
    if (task_scheduler->queues[worker])
      loom_work_queue_destroy(task_scheduler->queues[worker]);

  loom_event_destroy(task_scheduler->work_to_steal);

  loom_event_destroy(task_scheduler->message);

  loom_task_pool_destroy(task_scheduler->tasks);
  loom_permit_pool_destroy(task_scheduler->permits);

  free((void *)task_scheduler);
}

static loom_task_scheduler_t *S = NULL;

// We use a thread-local pointer to track the appropriate queue. This makes
// handling submissions much easier.
static LOOM_THREAD_LOCAL loom_work_queue_t *Q = NULL;

// We also track the index of the queue to simplify house keeping.
static LOOM_THREAD_LOCAL loom_uint32_t q = 0;

// We maintain a pseduo-random number generator per-thread to reduce false
// sharing, and implications of multi-threaded access.
static LOOM_THREAD_LOCAL loom_prng_t *P = NULL;

static loom_task_t *loom_acquire_a_task(void) {
  loom_task_t *task = loom_task_pool_acquire(S->tasks);
  return task;
}

static void loom_return_a_task(loom_task_t *task) {
  loom_task_pool_return(S->tasks, task);
}

static loom_permit_t *loom_acquire_a_permit(loom_task_t *task) {
  const loom_uint32_t blocker = loom_atomic_incr_u32(&task->blocks) - 1;

  if (blocker < LOOM_EMBEDDED_PERMITS) {
    if (blocker > 0)
      task->permits[blocker - 1].next = &task->permits[blocker];

    return &task->permits[blocker];
  } else {
    loom_permit_t **next = &task->permits[LOOM_EMBEDDED_PERMITS - 1].next;

    while (*next)
      next = &(*next)->next;

    *next = loom_permit_pool_acquire(S->permits);

    return *next;
  }
}

static void loom_return_a_permit(loom_permit_t *permit) {
  void *address = (void *)permit;

  const void *lower = (const void *)&S->permits->permits[0];
  const void *upper = (const void *)&S->permits->permits[S->permits->size - 1];

  if ((address < lower) || (address > upper))
    // Ingore if embedded.
    return;

  loom_permit_pool_return(S->permits, permit);
}

static void loom_signal_availability_of_work(void) {
  loom_atomic_set_native(&S->work, q);
  loom_event_signal(S->work_to_steal);
}

static void loom_submit_a_task(loom_task_t *task) {
  if (loom_atomic_cmp_and_xchg_u32(&task->blockers, 0, 0xffffffff) != 0)
    // Can't schedule yet. Should be picked up later.
    return;

  const loom_uint32_t work = loom_work_queue_push(Q, task);

  if (work > 1) {
    // We've got more work queued than we are able to schedule. Signal another
    // worker to steal some.
    loom_signal_availability_of_work();
  } else {
    if (q == 0) {
      if (S->always_steal_from_main_thread) {
        // No guarantee that the main thread will schedule work, so wake a
        // worker to steal, just in case.
        loom_signal_availability_of_work();
      }
    }
  }
}

// Try to grab a task from this worker's queue.
static loom_task_t *loom_grab_a_task(void) {
  while (!loom_work_queue_is_empty(Q))
    if (loom_task_t *task = loom_work_queue_pop(Q))
      return task;

  return NULL;
}

// Try to steal a task from other worker's queues.
static loom_task_t *loom_steal_a_task(void) {
  // To reduce contention, we only attempt to steal from a victim a few times,
  // opting to move on to the next victim if we don't succeed. In the
  // unforunate case we fail to steal from every victim, we try again in a
  // different order.

  while (1) {
    // Race is fine as the newly onlined worker will pick up work.
    const loom_native_t online = loom_atomic_load_native(&S->online);
    const loom_native_t offline = ~online;

    loom_native_t victims = loom_atomic_load_native(&S->work);

    // Make sure we don't try to steal from ourself.
    victims &= ~(1 << q);

    if (!victims)
      // No work to steal.
      return NULL;

    // Naively enumerating the work work queues introduces a bias toward
    // earlier work queues and will more than likely cause cascading starvation
    // of worker threads, degenerating scheduling into a free-for-all. To
    // combat this, we rotate `victims` by a random amount and enumerate as we
    // would normally, taking the rotation into count when selecting the work
    // queue to victimize.
    static const unsigned w = sizeof(victims) * CHAR_BIT;
    const loom_native_t r = loom_prng_grab_u32(P) % w;
    victims = (victims << r) | (victims >> ((-r) % w));

    while (victims) {
      const unsigned v = (loom_ctz_native(victims) + (w - r)) % w;

      // Retry try a few times, in case of contention.
      for (unsigned attempts = 0; attempts < 3; ++attempts)
        if (loom_task_t *task = loom_work_queue_steal(S->queues[v]))
          return task;

    #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
      const loom_bool_t draining = ((offline & (1ul << v)) != 0) | (v == 0);
    #elif LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
      const loom_bool_t draining = ((offline & (1ull << v)) != 0) | (v == 0);
    #endif

      if (draining)
        if (loom_work_queue_is_empty(S->queues[v]))
          // Drained all work from an offline worker's queue.
          loom_atomic_reset_native(&S->work, v);

      victims &= (victims - 1);
    }
  }
}

static void loom_unblock_any_permitted(loom_task_t *task) {
  // Tasks should not be modified by other threads once scheduled, so no race.
  if (task->blocks > 0) {
    loom_permit_t *permit = &task->permits[0];

    while (permit) {
      if (loom_atomic_decr_u32(&permit->task->blockers) == 0) {
        // Submit to this worker's queue.
        loom_submit_a_task(permit->task);
      }

      loom_permit_t *const next = permit->next;
      loom_return_a_permit(permit);
      permit = next;
    }
  }
}

static void loom_schedule_a_task(loom_task_t *task) {
  S->prologue.fn(task, S->prologue.context);

  switch (task->work.kind) {
    case LOOM_WORK_NONE:
      // Do nothing.
      break;

    case LOOM_WORK_CPU:
      task->work.cpu.kernel(task->work.cpu.data);
      break;
  }

  S->epilogue.fn(task, S->epilogue.context);

  if (task->barrier)
    loom_atomic_decr_u32(task->barrier);

  loom_unblock_any_permitted(task);

  // TODO(mtwilliams): Copy to stack and return to pool immediately?
  loom_return_a_task(task);
}

static void loom_worker_thread(void *worker_ptr) {
  loom_worker_t *worker = (loom_worker_t *)worker_ptr;

  Q = S->queues[worker->id];
  q = worker->id;

  if (P == NULL)
    P = loom_prng_create();

startup:
  loom_atomic_set_native(&S->online, q);

  while (1) {
  waiting:
    // Wait until there's work to steal, or a message to handle.
    loom_event_t *events[2] = {S->message, S->work_to_steal};
    switch (loom_event_wait_on_any(2, events, -1)) {
      case 1:
        if (loom_atomic_load_u32(&worker->shutdown))
          goto shutdown;

        // False wake up.
        goto waiting;

      case 2:
        // Work to be stolen!
        goto stealing;
    }

  work_in_queue:
    while (1) {
      if (loom_atomic_load_u32(&worker->shutdown))
        goto shutdown;

      if (loom_task_t *task = loom_grab_a_task())
        loom_schedule_a_task(task);
      else
        goto exhausted;
    }

  exhausted:
    // No work left in our queue.
    loom_atomic_reset_native(&S->work, q);

  stealing:
    // Steal work until none is left at all.
    while (1) {
      if (loom_atomic_load_u32(&worker->shutdown))
        goto shutdown;

      if (loom_task_t *task = loom_steal_a_task())
        loom_schedule_a_task(task);
      else if (!loom_work_queue_is_empty(Q))
        // Work in our queue.
        goto work_in_queue;
      else
        goto waiting;
    }
  }

shutdown:
  loom_atomic_reset_native(&S->online, q);

  // Let another thread drain our queue, or take over stealing work.
  loom_signal_availability_of_work();
}

// Returns the number of logical cores available.
static loom_uint32_t number_of_cores(void) {
#if LOOM_PLATFORM == LOOM_PLATFORM_WINDOWS
  SYSTEM_INFO system_info;
  GetNativeSystemInfo(&system_info);
  return system_info.dwNumberOfProcessors;
#elif LOOM_PLATFORM == LOOM_PLATFORM_MAC
#elif LOOM_PLATFORM == LOOM_PLATFORM_LINUX
#endif
}

static loom_uint32_t choose_number_of_workers(loom_int32_t workers) {
  if (workers < 0)
    workers = number_of_cores() + workers;

  if (workers >= LOOM_WORKER_LIMIT)
    workers = LOOM_WORKER_LIMIT - 1;

  return workers;
}

void loom_initialize(const loom_options_t *options) {
  loom_assert_debug(options != NULL);

  // Prevent double initialization.
  loom_assert_debug(S == NULL);

  S = loom_task_scheduler_create(options->tasks,
                                 options->permits,
                                 options->queue);

  if (options->prologue.fn)
    S->prologue = options->prologue;

  if (options->epilogue.fn)
    S->epilogue = options->epilogue;

  S->always_steal_from_main_thread = !options->main_thread_does_work;

  const loom_uint32_t workers =
    choose_number_of_workers(options->workers);

  loom_bring_up_workers(workers);

  Q = S->queues[0];
  q = 0;

  if (P == NULL)
    P = loom_prng_create();
}

void loom_shutdown(void) {
  loom_assert_debug(S != NULL);

  while (loom_atomic_load_native(&S->work))
    if (!loom_do_some_work())
      loom_thread_yield();

  loom_bring_down_workers(S->n);

  loom_task_scheduler_destroy(S);

  S = NULL;
  Q = NULL;
  q = 0;
}

void loom_bring_up_workers(unsigned n) {
  loom_lock_acquire(S->lock);

  // REFACTOR(mtwilliams): Silently limit?
  loom_assert_debug(S->n + n <= LOOM_WORKER_LIMIT);

  for (unsigned worker = S->n; n > 0; --n, ++worker) {
    // Make sure not already online.
    loom_assert_debug(S->workers[worker].thread == NULL);

    loom_thread_options_t worker_thread_options;

    char worker_thread_name[17];
    snprintf(&worker_thread_name[0], 16, "Worker %02u", worker + 1);
    worker_thread_name[16] = '\0';

    worker_thread_options.name = &worker_thread_name[0];

  #if LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86
    worker_thread_options.affinity = (1ul << worker);
  #elif LOOM_ARCHITECTURE == LOOM_ARCHITECTURE_X86_64
    worker_thread_options.affinity = (1ull << worker);
  #endif

    worker_thread_options.stack = 0;

    if (S->queues[worker + 1] == NULL)
      S->queues[worker + 1] = loom_work_queue_create(S->size_of_each_work_queue);

    S->workers[worker].thread = loom_thread_spawn(&loom_worker_thread,
                                                  (void *)&S->workers[worker],
                                                  &worker_thread_options);

    S->n += 1;
  }

  loom_lock_release(S->lock);
}

void loom_bring_down_workers(unsigned n) {
  loom_lock_acquire(S->lock);

  // REFACTOR(mtwilliams): Silently limit?
  loom_assert_debug(S->n >= n);

  // Post shutdown messages.
  for (unsigned worker = S->n; worker > S->n - n; --worker) {
    // Make sure not already offline.
    loom_assert_debug(S->workers[worker - 1].thread != NULL);

    // Flag for shutdown.
    loom_atomic_store_u32(&S->workers[worker - 1].shutdown, 1);
  }

  // Wait until acknowledged.
  loom_event_signal(S->message);

  for (unsigned worker = S->n; n > 0; --n, --worker) {
    loom_thread_join(S->workers[worker - 1].thread);

    // No longer any associated worker thread.
    S->workers[worker - 1].thread = NULL;

    // Prevent shutdown if brought up again.
    loom_atomic_store_u32(&S->workers[worker - 1].shutdown, 0);

    S->n -= 1;
  }

  loom_event_unsignal(S->message);

  loom_lock_release(S->lock);
}

static loom_handle_t task_to_handle(loom_task_t *task) {
  loom_handle_t handle;

#if LOOM_CONFIGURATION == LOOM_CONFIGURATION_DEBUG
  handle.index = task - S->tasks->tasks;
  handle.id = task->id;
#else
  handle.opaque = (void *)task;
#endif

  return handle;
}

static loom_task_t *handle_to_task(loom_handle_t handle) {
#if LOOM_CONFIGURATION == LOOM_CONFIGURATION_DEBUG
  loom_task_t *task = &S->tasks->tasks[handle.index];
  loom_assert_debug(task->id == handle.id);
  return task;
#else
  return (loom_task_t *)handle.opaque;
#endif
}

loom_handle_t loom_empty(loom_uint32_t flags) {
  loom_task_t *task = loom_acquire_a_task();

  task->flags = flags;

  task->work.kind = LOOM_WORK_NONE;

  memset((void *)&task->permits[0], 0, LOOM_EMBEDDED_PERMITS * sizeof(loom_permit_t));

  task->blocks = 0;
  task->blockers = 0;

  task->barrier = NULL;

  return task_to_handle(task);
}

loom_handle_t loom_describe(loom_kernel_fn kernel,
                            void *data,
                            loom_uint32_t flags) {
  loom_task_t *task = loom_acquire_a_task();

  task->flags = flags;

  task->work.kind = LOOM_WORK_CPU;
  task->work.cpu.kernel = kernel;
  task->work.cpu.data = data;

  memset((void *)&task->permits[0], 0, LOOM_EMBEDDED_PERMITS * sizeof(loom_permit_t));

  task->blocks = 0;
  task->blockers = 0;

  task->barrier = NULL;

  return task_to_handle(task);
}

static void permit(loom_task_t *task,
                   loom_task_t *permitee) {
  loom_permit_t *permit = loom_acquire_a_permit(task);

  permit->next = NULL;
  permit->task = permitee;

  loom_atomic_incr_u32(&permitee->blockers);
}

void loom_permits(loom_handle_t task,
                  loom_handle_t permitee) {
  permit(handle_to_task(task), handle_to_task(permitee));
}

void loom_kick(loom_handle_t task) {
  loom_kick_n(1, &task);
}

void loom_kick_n(unsigned n, const loom_handle_t *tasks) {
  for (unsigned i = 0; i < n; ++i) {
    loom_task_t *task = handle_to_task(tasks[i]);
    loom_submit_a_task(task);
  }
}

void loom_kick_and_wait(loom_handle_t task) {
  loom_kick_and_wait_n(1, &task);
}

static loom_bool_t is_zero_yet(volatile loom_uint32_t *v) {
  return (loom_atomic_load_u32(v) == 0)
      && (loom_atomic_cmp_and_xchg_u32(v, 0, 0) == 0);
}

static void kick(unsigned n, const loom_handle_t *tasks, loom_uint32_t *barrier) {
  for (unsigned i = 0; i < n; ++i) {
    loom_task_t *task = handle_to_task(tasks[i]);
    task->barrier = barrier;
  }

  for (unsigned i = 0; i < n; ++i) {
    loom_task_t *task = handle_to_task(tasks[i]);
    loom_submit_a_task(task);
  }
}

void loom_kick_and_wait_n(unsigned n, const loom_handle_t *tasks) {
  loom_uint32_t outstanding = n;

  kick(n, tasks, &outstanding);

  while (!is_zero_yet(&outstanding))
    loom_thread_yield();
}

void loom_kick_and_do_work_while_waiting(loom_handle_t task) {
  loom_kick_and_do_work_while_waiting_n(1, &task);
}

void loom_kick_and_do_work_while_waiting_n(unsigned n,
                                           const loom_handle_t *tasks) {
  loom_uint32_t outstanding = n;

  kick(n, tasks, &outstanding);

  while (!is_zero_yet(&outstanding))
    if (!loom_do_some_work())
      loom_thread_yield();
}

loom_bool_t loom_do_some_work(void) {
  loom_assert_debug(q == 0);

  if (loom_task_t *task = loom_grab_a_task()) {
    loom_schedule_a_task(task);
    return true;
  }

  if (loom_task_t *task = loom_steal_a_task()) {
    loom_schedule_a_task(task);
    return true;
  }

  return false;
}

LOOM_END_EXTERN_C
