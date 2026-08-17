# Configurable Logging & Debugging via C Preprocessor Directives

A small, dependency-free C11 logging/debugging kit whose behavior is decided
entirely by compiler command-line switches.  Every `LOG_*` statement either
survives as a real call, becomes `((void)0)`, or is removed so completely that
its arguments are never even evaluated — there is **zero runtime overhead** for
anything you compile out.

Verified with GCC 16.1 (MinGW-w64 / UCRT) on Windows.

## Layout

| File               | Role                                                        |
|--------------------|-------------------------------------------------------------|
| `include/dbg.h`    | All configuration switches + the public `LOG_*` macros      |
| `src/log.c`        | Backend: formatting, color, output, runtime level, trap, leak tracker |
| `src/demo.c`       | Demonstration for every feature and build profile           |
| `Makefile`         | Three presets: `all` (dev/trace), `release`, `errors-only`  |

## Build

```sh
make all            # dev: TRACE level, colors, timing, CONFIG_DEBUG=1
make release        # CONFIG_LOG_ENABLE=0 -> everything removed
make errors-only    # only WARN/ERROR/FATAL survive compilation
make run            # build + run the dev profile
./demo.exe          # stops at LOG_FATAL (exit 1); leak report on stderr
./demo.exe trap     # forces LOG_ASSERT(x==3) to fail and trap the process
```

## Configuration switches (all overridable with `-D`)

| Macro                  | Default            | Effect                                                     |
|------------------------|--------------------|------------------------------------------------------------|
| `CONFIG_LOG_ENABLE`    | 1                  | Master on/off. `0` removes every logging call.             |
| `CONFIG_LOG_LEVEL`     | `LOG_LEVEL_INFO`   | Lowest severity kept: `TRACE < DEBUG < INFO < WARN < ERROR < FATAL`. |
| `CONFIG_LOG_TIME`      | 1                  | Prefix each line with `HH:MM:SS`.                          |
| `CONFIG_LOG_FILENAME`  | 1                  | Prefix each line with `file:line`.                         |
| `CONFIG_LOG_COLOR`     | 0                  | ANSI-colored level tags.                                   |
| `CONFIG_DEBUG`         | 0                  | Enables `LOG_ASSERT` and the allocation tracer.            |
| `CONFIG_LOG_MAX_LINE`  | 512                | Prefix assembly buffer size.                               |

## API

```c
LOG_TRACE(fmt, ...)     LOG_DEBUG(fmt, ...)     LOG_INFO(fmt, ...)
LOG_WARN(fmt, ...)      LOG_ERROR(fmt, ...)     LOG_FATAL(fmt, ...)
LOG_FUNC_ENTER(args)    LOG_FUNC_EXIT(args)     /* TRACE-level call tracing */
LOG_ASSERT(cond, ...)                          /* CONFIG_DEBUG only */
dbg_malloc(n)  dbg_free(p)                     /* leak tracked when CONFIG_DEBUG */

log_init();  log_get_level();  log_set_level(n);   /* runtime tuning */
```

## Design

### 1. Levels live in the preprocessor namespace

Severities are plain macros (`#define LOG_LEVEL_TRACE 0` … `FATAL 5`), so the
same tokens can be compared inside `#if` expressions *and* passed as plain
`int` values to the backend.  There is no enum to get out of sync.

### 2. Compile-time elimination, not filtering

```c
#if CONFIG_LOG_ENABLE
#define LOG_TRACE(...) \
    log_write(LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_TRACE(...) ((void)0)
#endif
```

`log.h` wraps this in `DBG_KEEP(sev)` so each call site is kept only if
`sev >= CONFIG_LOG_LEVEL`.  When a call is eliminated, its argument
expressions are **never** compiled in, so no side effect runs — proven in
`demonstrate_zero_cost()` where a counter stays `0` unless TRACE is compiled
in.  `LOG_FATAL` deliberately ignores the level filter: the termination path
must never silently vanish from a release build.

### 3. Runtime level as a second, independent gate

`g_min_level` starts at `CONFIG_LOG_LEVEL`.  `log_set_level()` can raise it
at run time so the same binary can log more or less depending on a command
line flag — but anything *below* the compiled threshold was already removed,
so "minimal binary decides minimal behavior".

### 4. Format-string safety at compile time

The backend is declared with `__attribute__((format(printf, 5, 6)))` on
GCC/Clang, so misuse like `LOG_INFO("%d", "text")` is a compile-time error,
exactly as with `printf` itself.

### 5. Zero-size assertions and leak tracking

`LOG_ASSERT(cond, ...)` expands to `((void)0)` when `CONFIG_DEBUG` is off; no
code, no evaluation of `cond`.  When on, a failure logs the condition text
(via `#cond`), any message (via `#__VA_ARGS__`), and traps through
`dbg_trap()` — `__debugbreak()` on MSVC, `__builtin_trap()` otherwise.  The
allocator shadow (`dbg_malloc`/`dbg_free`) keeps two counters and registers an
`atexit` leak report; with `CONFIG_DEBUG` off it forwards straight to
`malloc`/`free`.

### 6. The whole backend can disappear

With `CONFIG_LOG_ENABLE=0`, `src/log.c` compiles to empty stubs and the level
tables are guarded out, so a production binary carries no logging data at
all (the demo's binaries shrink accordingly).

## Output samples

```
[01:20:01] [WARN ] src/demo.c:98 main(): warn: disk space low, 92.5% of quota used
[01:20:01] [ERROR] src/demo.c:120 main(): ASSERT FAILED: x == 3
[01:20:01] [ERROR] src/demo.c:120 main():   message: x was changed unexpectedly to 2
[01:20:01] [1;31m[FATAL][0m src/demo.c:127 main(): unrecoverable fault in main
[dbg] 1 allocation(s) never freed (0.2 KiB total)
```

## Extensions

- Redirect output to a file instead of stderr (`log_set_output(FILE *)`).
- Record the allocation call site per allocation for leak *location* reporting.
- Thread-safety wrapper around `log_write` (a single mutex guards the shared
  prefix buffer).
- A panic-handler hook so `LOG_FATAL` can dump a backtrace before exiting.