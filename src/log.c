/*
 * log.c - runtime backend for the dbg.h logging macros.
 *
 * This file should be compiled with the SAME preprocessor switches as the
 * rest of the project (share dbg.h so the defaults line up).  When
 * CONFIG_LOG_ENABLE == 0 the whole backend collapses to empty stubs and the
 * linker drops everything.
 */
#include "dbg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#if CONFIG_LOG_LEVEL < LOG_LEVEL_TRACE || CONFIG_LOG_LEVEL > LOG_LEVEL_FATAL
#error "CONFIG_LOG_LEVEL must be one of LOG_LEVEL_TRACE..LOG_LEVEL_FATAL"
#endif
#if CONFIG_LOG_FILENAME && !CONFIG_LOG_MAX_LINE
#error "CONFIG_LOG_MAX_LINE must be >= 1"
#endif

/* ------------------------------------------------------------------ */
/* Static tables used by the enabling build.  Guarded so a fully      */
/* disabled build carries no dead data.                                */
/* ------------------------------------------------------------------ */
#define DBG_NUM_LEVELS 6

#if CONFIG_LOG_ENABLE
static const char *const g_level_name[DBG_NUM_LEVELS] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#if CONFIG_LOG_COLOR
static const char *const g_level_color[DBG_NUM_LEVELS] = {
    "\x1b[90m",      /* TRACE - bright black   */
    "\x1b[36m",      /* DEBUG - cyan           */
    "\x1b[32m",      /* INFO  - green          */
    "\x1b[33m",      /* WARN  - yellow         */
    "\x1b[31m",      /* ERROR - red            */
    "\x1b[1;31m"     /* FATAL - bright red     */
};
static const char *const g_reset = "\x1b[0m";
#else
static const char *const g_level_color[DBG_NUM_LEVELS] = {
    "", "", "", "", "", ""
};
static const char *const g_reset = "";
#endif
#endif /* CONFIG_LOG_ENABLE (tables) */

#if CONFIG_LOG_ENABLE
/* Runtime minimum level.  Initialised to the configured compile-time   */
/* threshold; the application may raise/lower it at run time with       */
/* log_set_level().  Note this can only *tighten* or *loosen* filtering */
/* below the compiled threshold - calls below CONFIG_LOG_LEVEL were     */
/* already removed at compile time.                                     */
static volatile int g_min_level = CONFIG_LOG_LEVEL;

/* ------------------------------------------------------------------ */
/* Enabled backend.                                                    */
/* ------------------------------------------------------------------ */

void log_init(void)
{
    g_min_level = CONFIG_LOG_LEVEL;
}

int log_get_level(void)
{
    return g_min_level;
}

void log_set_level(int level)
{
    if (level >= LOG_LEVEL_TRACE && level <= LOG_LEVEL_FATAL)
        g_min_level = level;
}

void log_write(int level, const char *file, int line, const char *func,
               const char *fmt, ...)
{
    char    prefix[CONFIG_LOG_MAX_LINE];
    char    stamp[16];
    size_t  used = 0;

    if (level < g_min_level)
        return;

    /* Compose the prefix piece by piece so no single component can
       overflow the fixed buffer. */
    prefix[0] = '\0';

#if CONFIG_LOG_TIME
    {
        time_t    now = time(NULL);
        struct tm tm_buf;
#if defined(_WIN32)
        localtime_s(&tm_buf, &now);
        strftime(stamp, sizeof stamp, "%H:%M:%S", &tm_buf);
#else
        struct tm *tm_p = localtime_r(&now, &tm_buf);
        if (tm_p == NULL)
            strcpy(stamp, "--:--:--");
        else
            strftime(stamp, sizeof stamp, "%H:%M:%S", tm_p);
#endif
        used += (size_t)snprintf(prefix + used, sizeof prefix - used,
                                 "[%s] ", stamp);
    }
#endif
    if (level >= 0 && level < DBG_NUM_LEVELS)
        used += (size_t)snprintf(prefix + used, sizeof prefix - used,
                                 "%s[%-5s]%s ",
                                 g_level_color[level],
                                 g_level_name[level],
                                 g_reset);
    else
        used += (size_t)snprintf(prefix + used, sizeof prefix - used,
                                 "[%d] ", level);

#if CONFIG_LOG_FILENAME
    if (file)
        used += (size_t)snprintf(prefix + used, sizeof prefix - used,
                                 "%s:%d ", file, line);
#endif
    if (func)
        used += (size_t)snprintf(prefix + used, sizeof prefix - used,
                                 "%s(): ", func);

    /* Emit the composed prefix then the user format string. */
    fputs(prefix, stderr);

    {
        va_list ap;
        va_start(ap, fmt);
        if (fmt)
            vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    fputc('\n', stderr);
    fflush(stderr);

    if (level == LOG_LEVEL_FATAL)
        exit(EXIT_FAILURE);
}

#else /* !CONFIG_LOG_ENABLE: stubs so callers still link cleanly. */

void log_init(void)   {}
int  log_get_level(void) { return CONFIG_LOG_LEVEL; }
void log_set_level(int level) { (void)level; }
void log_write(int level, const char *file, int line, const char *func,
               const char *fmt, ...) { (void)level; (void)file; (void)line;
                                       (void)func; (void)fmt; }

#endif

/* ------------------------------------------------------------------ */
/* Assertion trap (only compiled when CONFIG_DEBUG).                   */
/* ------------------------------------------------------------------ */
#if CONFIG_DEBUG
void dbg_trap(void)
{
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
    /* Hard breakpoint under a debugger; raises int3 otherwise. */
    __debugbreak();
#elif defined(__has_builtin) && __has_builtin(__builtin_trap)
    __builtin_trap();
#else
    abort();
#endif
}
#endif

/* ------------------------------------------------------------------ */
/* Allocation tracer (only compiled when CONFIG_DEBUG).  The whole     */
/* thing is cheap by design: two counters and a lazy atexit hook.      */
/* ------------------------------------------------------------------ */
#if CONFIG_DEBUG
static size_t  g_alloc_count = 0;
static size_t  g_alloc_bytes = 0;
static int     g_leak_hook   = 0;

void dbg_report_leaks(void)
{
    if (g_alloc_count > 0)
        fprintf(stderr,
                "\n[dbg] %zu allocation(s) never freed (%.1f KiB total)\n",
                g_alloc_count, (double)g_alloc_bytes / 1024.0);
}

void *dbg_malloc(size_t size)
{
    void *p = malloc(size);
    if (!g_leak_hook) {
        g_leak_hook = 1;
        atexit(dbg_report_leaks);
    }
    if (p) {
        g_alloc_count++;
        g_alloc_bytes += size;
    }
    return p;
}

void dbg_free(void *p)
{
    if (p) {
        free(p);
        if (g_alloc_count > 0)
            g_alloc_count--;
    }
}
#endif /* CONFIG_DEBUG */