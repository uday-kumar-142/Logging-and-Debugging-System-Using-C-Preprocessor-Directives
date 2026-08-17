/*
 * dbg.h - Configurable logging & debugging system built on the C preprocessor.
 *
 * Design intent
 * -------------
 * All configuration happens at *compile time* via macro definitions passed on
 * the command line (-D...) or set below.  Each source file includes this header;
 * the preprocessor then either keeps a logging statement, rewrites it to a no-op,
 * or removes it entirely.  When logging is compiled out there is ZERO runtime cost
 * (no branches, no arguments evaluated).
 *
 * Configuration switches
 * ----------------------
 *   CONFIG_LOG_ENABLE    (0|1, default 1)  Master switch. 0 => all log calls vanish.
 *   CONFIG_LOG_LEVEL     default INFO     Minimum severity compiled in.
 *                                          LOG_LEVEL_TRACE < DEBUG < INFO < WARN <
 *                                          LOG_LEVEL_ERROR < LOG_LEVEL_FATAL.
 *   CONFIG_LOG_TIME      (0|1, default 1)  Prefix each line with HH:MM:SS.
 *   CONFIG_LOG_FILENAME  (0|1, default 1)  Prefix each line with file:line.
 *   CONFIG_LOG_COLOR     (0|1, default 0)  ANSI colorized level tags.
 *   CONFIG_DEBUG         (0|1, default 0)  Enables LOG_ASSERT + allocation tracer.
 *   CONFIG_LOG_MAX_LINE  (default 512)     Prefix buffer size used by the backend.
 *
 * Build flags for the two extreme profiles:
 *   Full trace debugging:  -DCONFIG_LOG_LEVEL=LOG_LEVEL_TRACE -DCONFIG_DEBUG=1 \
 *                          -DCONFIG_LOG_COLOR=1 -DCONFIG_LOG_TIME=1
 *   Production release:    -DCONFIG_LOG_ENABLE=0
 */
#ifndef DBG_H
#define DBG_H

#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Level identifiers live in the preprocessor namespace so they can be */
/* used both in #if expressions and as plain integer arguments.        */
/* ------------------------------------------------------------------ */
#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_FATAL 5

/* ------------------------------------------------------------------ */
/* Defaults - every switch can be overridden with -D on the command    */
/* line before this file is included.                                  */
/* ------------------------------------------------------------------ */
#ifndef CONFIG_LOG_ENABLE
#define CONFIG_LOG_ENABLE 1
#endif

#ifndef CONFIG_LOG_LEVEL
#define CONFIG_LOG_LEVEL LOG_LEVEL_INFO
#endif

#ifndef CONFIG_LOG_TIME
#define CONFIG_LOG_TIME 1
#endif

#ifndef CONFIG_LOG_FILENAME
#define CONFIG_LOG_FILENAME 1
#endif

#ifndef CONFIG_LOG_COLOR
#define CONFIG_LOG_COLOR 0
#endif

#ifndef CONFIG_DEBUG
#define CONFIG_DEBUG 0
#endif

#ifndef CONFIG_LOG_MAX_LINE
#define CONFIG_LOG_MAX_LINE 512
#endif

/* Visual Studio lacks __func__ (C99); __FUNCTION__ is the substitute. */
#if defined(_MSC_VER) && !defined(__GNUC__)
#ifndef __func__
#define __func__ __FUNCTION__
#endif
#endif

/* ------------------------------------------------------------------ */
/* Backend API (implemented in src/log.c).  Not meant to be called     */
/* directly by application code - use the LOG_* macros below.          */
/* ------------------------------------------------------------------ */
#if defined(__GNUC__) || defined(__clang__)
#define DBG_PRINTF_ATTR(a, b) __attribute__((format(printf, a, b)))
#else
#define DBG_PRINTF_ATTR(a, b)
#endif

DBG_PRINTF_ATTR(5, 6) void log_write(int level, const char *file, int line,
                                     const char *func, const char *fmt, ...);

void log_init(void);
int  log_get_level(void);
void log_set_level(int level);

#if CONFIG_DEBUG
void dbg_trap(void);
void dbg_report_leaks(void);
#endif

/* ------------------------------------------------------------------ */
/* Master enable / per-severity compile-time selection.                */
/*                                                                     */
/* A statement whose severity is *less than* CONFIG_LOG_LEVEL is       */
/* removed by the preprocessor, so the arguments are never evaluated   */
/* and no code is emitted.  LOG_FATAL is special-cased: it is always   */
/* compiled when CONFIG_LOG_ENABLE is on, so the termination path can  */
/* never silently disappear from a release build.                      */
/* ------------------------------------------------------------------ */
#if CONFIG_LOG_ENABLE
#define DBG_KEEP(sev)  ((sev) >= (CONFIG_LOG_LEVEL))

#if DBG_KEEP(LOG_LEVEL_TRACE)
#define LOG_TRACE(...) \
    log_write(LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FUNC_ENTER(...) \
    LOG_TRACE(">> %s(" #__VA_ARGS__ ")", __func__)
#define LOG_FUNC_EXIT(...) \
    LOG_TRACE("<< %s", __func__)
#else
#define LOG_TRACE(...)      ((void)0)
#define LOG_FUNC_ENTER(...) ((void)0)
#define LOG_FUNC_EXIT(...)  ((void)0)
#endif

#if DBG_KEEP(LOG_LEVEL_DEBUG)
#define LOG_DEBUG(...) \
    log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_DEBUG(...) ((void)0)
#endif

#if DBG_KEEP(LOG_LEVEL_INFO)
#define LOG_INFO(...) \
    log_write(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_INFO(...) ((void)0)
#endif

#if DBG_KEEP(LOG_LEVEL_WARN)
#define LOG_WARN(...) \
    log_write(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_WARN(...) ((void)0)
#endif

#if DBG_KEEP(LOG_LEVEL_ERROR)
#define LOG_ERROR(...) \
    log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define LOG_ERROR(...) ((void)0)
#endif

/* LOG_FATAL ignores the level filter but respects LOG_ENABLE. */
#define LOG_FATAL(...) \
    log_write(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)
#else /* CONFIG_LOG_ENABLE == 0: everything compiles out. */
#define LOG_TRACE(...)      ((void)0)
#define LOG_FUNC_ENTER(...) ((void)0)
#define LOG_FUNC_EXIT(...)  ((void)0)
#define LOG_DEBUG(...)      ((void)0)
#define LOG_INFO(...)       ((void)0)
#define LOG_WARN(...)       ((void)0)
#define LOG_ERROR(...)      ((void)0)
#define LOG_FATAL(...)      ((void)0)
#endif

/* ------------------------------------------------------------------ */
/* Assertions.  Active only when CONFIG_DEBUG is defined as 1.         */
/* On failure the condition, location and message are logged as ERROR  */
/* and the process traps (debugger breakpoint on MSVC/Intel, SIGTRAP   */
/* on POSIX, __builtin_trap() elsewhere).                              */
/* ------------------------------------------------------------------ */
#if CONFIG_DEBUG
#define LOG_ASSERT(cond, ...)                                        \
    do {                                                             \
        if (!(cond)) {                                               \
            log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, \
                      "ASSERT FAILED: %s", #cond);                   \
            if (sizeof(#__VA_ARGS__) > 1)                            \
                log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__,       \
                          __func__, "  message: " __VA_ARGS__);      \
            dbg_trap();                                              \
        }                                                            \
    } while (0)
#else
#define LOG_ASSERT(cond, ...) ((void)0)
#endif

/* ------------------------------------------------------------------ */
/* Allocation tracer.  When CONFIG_DEBUG is off these forward directly */
/* to malloc/free with no overhead.  When on, counts are kept and a    */
/* leak summary is printed at exit via atexit().                       */
/* ------------------------------------------------------------------ */
#if CONFIG_DEBUG
void *dbg_malloc(size_t size);
void  dbg_free(void *p);
#else
#include <stdlib.h>
static inline void *dbg_malloc(size_t size) { return malloc(size); }
static inline void  dbg_free(void *p)       { free(p); }
#define dbg_report_leaks() ((void)0)
#endif

#endif /* DBG_H */