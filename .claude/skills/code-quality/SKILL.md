---
name: code-quality
description: Checks to run on a change to a C project, especially one that ships a library, before pushing it or opening a pull request. They cover what automated reviewers such as CodeQL and LLM code reviewers keep finding in C code. The static patterns CodeQL flags come first. Then come changes in what callers of the library observe that a comparison of the program's output does not show: error and cancel paths, callbacks, logging, threads, global state. Use it whenever you change the C sources, before every push of such a change, and when a review bot comments on a pull request.
---

# Code quality checks for C projects

Automated reviewers find two kinds of problem in C changes, and both are
cheaper to catch before pushing than after:

- **Static patterns**, flagged by tools such as CodeQL. "Note"-level findings
  often appear only as review comments, even when the check itself passes.
- **Behaviour changes that callers observe**, found by reviewers that read a
  change against its callers (LLM code reviewers, humans). The test suite
  usually drives the project's own program and compares its output, so it
  can't see these.

Before calling a pull request done, read the review comments on its latest
head, not only the check results. The project's own notes (AGENTS.md,
CONTRIBUTING) may name the bots and add rules of their own.

## 1. Static patterns

| CodeQL rule | what to write instead |
|---|---|
| `cpp/loop-variable-changed`: a `for` counter changed in the body, e.g. `i++` to skip a paired element | A `while` loop, or a `for` loop whose header alone advances the counter. |
| `cpp/commented-out-code`: a comment that parses as code, e.g. `/* x = y; */` | Delete the code, or describe it in words. |
| `cpp/world-writable-file-creation`: `fopen(name, "w")` and similar | For output files, `open(name, O_WRONLY \| O_CREAT \| O_TRUNC, 0644)` and `fdopen()`. For scratch files, `mkstemp()` (mode 0600) or the library's equivalent. |
| `cpp/equality-on-floats` | Compare with a tolerance, or say in a comment why the values are exact. |

Also:

- **Formatting:** if CI runs a formatter, run the **same version** locally on
  every file you touched. Formatters change their output between versions, so
  a newer local one can pass code that CI rejects. Install the pinned version
  in a virtualenv (`pip install 'clang-format==N.*'`) if the system one
  differs.
- **Warnings:** build with the project's warning flags and fix new warnings.
  Making a function `static` lets the compiler see its call sites, which can
  bring up new ones, such as `-Wformat-truncation`.

## 2. Keep what callers of the library observe

A library has callers other than the project's program. For every change, go
through what a caller can observe besides the output:

- **Return values on every path**, including error, cancel and empty-input
  paths, as the header documents them. For example, a function documented to
  return NULL on cancel must not return a half-initialised object.
- **Error reporting:** the same messages, codes and order. Errors raised in
  work done elsewhere (another thread, a later pass) are kept and reported in
  the original order, on the caller's thread.
- **Callbacks** (progress, cancel, error, allocation hooks): the same number
  of calls, on the calling thread, in the same order. A cancel must take
  effect as early as before. Don't start work before the first cancel check
  that the old code would not have started.
- **Logging:** a caller may have installed its own log handler or sink. Unless
  the library itself owns the handler, don't assume what it shows or drops.
  Skipping messages early is safe only where the library knows for certain
  they would be dropped.
- **Threads:** no callback on a thread the caller didn't create, unless the
  API says so. Don't add threads that a caller can't avoid. Check what
  happens when the caller is itself multi-threaded.
- **Global and static state, environment variables, files and signals:** a
  library that reads an environment variable or keeps static state has a new
  interface. Document it, and keep the old behaviour when it isn't set.
- **ABI:** struct sizes and layouts, enum values and exported prototypes in the
  installed headers. Changing these needs a soname change, so ask the
  maintainer first.

Check these with small client programs built against the library, not only
with the project's program:

- one that counts the callback calls;
- one that cancels at the N-th cancel check and times how long the call takes
  to return afterwards;
- one that installs its own log handler;
- one that frees everything it gets back, to run under valgrind.

Run each against the old and the new library and compare the numbers. These
throwaway programs belong in a scratch directory, not in the repository.

## 3. Compare more than the test suite

- **Compare side by side:** build the base branch and the change in two
  worktrees and run both over every input in the test suite plus a few large
  generated ones. Use several option sets, each with its values (for example
  `-color-count 8`, not a bare `-color-count`). Compare the output, **stderr**
  and the **exit status**.
- **Degenerate inputs:** empty input, one element, input that the options
  reduce to nothing. Divisions by a count and loops that assume at least one
  item fail only there.
- **Memory checks:** run valgrind memcheck, or ASan and UBSan builds, on the
  program and on the client programs, including the cancel and error paths.
  If the build uses libtool, the uninstalled program is a wrapper script: run
  tools through `./libtool --mode=execute`, or the system's installed library
  may be loaded instead.
- **Performance claims:** measure wall time as the best of several runs, and
  say on what machine. Profilers that count instructions, such as callgrind,
  show where the work is, not how long it takes. Measure the library and the
  program separately when they behave differently.

## 4. Threads

If a change runs code on several threads:

- List the shared structures each task reads and writes, and check that each
  task writes only its own data. Data handed to a task before it is queued, or
  read after the task is waited for, needs that hand-off to synchronise it.
- **The result must not depend on the number of threads.** Compare one
  thread, several, and more threads than processors. Allocate results per
  task, and merge them in a fixed order on one thread.
- **ThreadSanitizer** understands pthread mutexes and condition variables.
  It doesn't understand locking built directly on futexes or atomics in an
  uninstrumented library. Every hand-off through such a library is then
  reported as a race. Annotate those hand-offs with
  `__tsan_acquire`/`__tsan_release` (from `<sanitizer/tsan_interface.h>`) in a
  throwaway build, and judge what remains.
- **Lock contention:** a library-wide lock that every task takes (a logger, an
  allocator, a global cache) can make more threads slower than one. Measure
  1, 2, 4 and all processors before choosing a default.

### If the project uses GLib

- `GMutex`, `GCond` and the `GThreadPool` queue use futexes on Linux, so they
  need the TSan annotations above. Reports that remain inside
  `g_thread_pool_*`, such as memory GLib allocates in one thread and frees with
  `g_free_sized` in another, are GLib's own.
- `g_log()` formats the message before any handler sees it, and takes a
  process-wide lock. A debug message in a hot loop costs a `printf` even when
  nothing shows it, and it serialises threads. Check the level before
  formatting, but only where the code owns the handler (see Logging above).
- GLib log handlers run on the thread that logs, so a `g_debug()` in a worker
  runs the caller's handler on that worker.
- `GThreadPool` hands out items in FIFO order. To keep the lock cheap,
  queue runs of consecutive items rather than single small items.
