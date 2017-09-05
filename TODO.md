TODO:

* Make use of affinity sets on Mac.
  * [Overview](http://superuser.com/questions/149312)
  * [Reference](http://yyshen.github.io/2015/01/18/binding_threads_to_cores_osx.html)

* Determine processor topology.
  * Specifically, we should detect hyper-threading.

* Amalgamate.
  * Append, in include order, all headers.
    * Remove header.
    * Remove header guards.

BUG:

* Work queues will erroneously calculate depth due to integer overflow.

FEATURE:

* Allow user to specify that hardware threads should be ignored.

* Fiber backed tasks.

* Continuations.
  * Provide an example that leverages continuations to pause tasks while performing asynchronous I/O.

* Support tracing.
  * Use ETW on Windows.
  * Use DTrace probes on macOS and Linux.

PERF:

* Pin threads to every second logical core on hyper-threaded processors when running less workers than cores. For example, on a processor with two cores and four threads:

  Worker 1 => 1 (Core 1, Thread 1)
  Worker 2 => 3 (Core 2, Thread 1)
  Worker 3 => 2 (Core 1, Thread 2)
  Worker 4 => 4 (Core 2, Thread 2)

  https://github.com/ponylang/ponyc/blob/master/src/libponyrt/sched/cpu.c#L89

* Determine if `SetThreadIdealProcessor` improves scheduling.
  * If not, remove to simplify code.

* Leverage futexes if available.
  * Use `WaitOnAddress` and `WakeByAddressSingle` (or `WakeByAddressAll`) on Windows 8 or newer.

MEM:

* Utilize `STACK_SIZE_PARAM_IS_A_RESERVATION` for worker threads?

REFACTOR:

* Use `SetThreadGroupAffinity` instead of `SetThreadAffinityMask`?

* Extract entropy gathering from `loom_prng_create`.
  * Allow manual override, for testing.

SMELL:

INVESTIGATE:

* Should we check return value when joining threads on Windows?
