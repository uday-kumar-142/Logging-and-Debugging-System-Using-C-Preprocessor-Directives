/*
 * demo.c - exercises every level, the debug helpers and shows how the
 * same source produces different output under different -D switches.
 */
#include <stdio.h>
#include <string.h>
#include "dbg.h"

/* The zero-cost proof lives in demonstrate_zero_cost(): when the trace
   level is compiled out the whole loop body disappears, so the counter
   shown on stdout reveals whether code was really removed. */
#define TRACE_LOOP_BYTES 4

static int worker(int n)
{
    LOG_FUNC_ENTER(n);

    if (n > 10)
        LOG_WARN("input %d exceeds expected range [0, 10]", n);
    else
        LOG_INFO("input %d within range", n);

    LOG_TRACE("computing result with size %u seed %d",
              (unsigned)sizeof(int), n);

    LOG_FUNC_EXIT(0);
    return n * 2;
}

static void demonstrate_zero_cost(void)
{
    volatile int side_effect = 0;

    /* In a trace-enabled build this register is incremented; in any
       other build the LOG_TRACE line disappears and with it the loop
       body, so nothing is ever evaluated. */
    for (int i = 0; i < TRACE_LOOP_BYTES; i++)
        LOG_TRACE("side effect #%d", side_effect++);

    printf("  side effects executed: %d"
           "  (LOG_TRACE compiled in? "
#if   CONFIG_LOG_ENABLE
#     if DBG_KEEP(LOG_LEVEL_TRACE)
           "yes"
#     else
           "no - loop body compiled out"
#     endif
#else
           "no - logging entirely disabled"
#endif
           ")\n",
           side_effect);
}

static void demo_config_summary(void)
{
    printf("------------------------------------------------------------\n");
    printf("Build configuration:\n");
    printf("  CONFIG_LOG_ENABLE   = %d\n", CONFIG_LOG_ENABLE);
    printf("  CONFIG_LOG_LEVEL    = %s (%d)\n",
#if CONFIG_LOG_ENABLE
#if   CONFIG_LOG_LEVEL == LOG_LEVEL_TRACE
           "TRACE"
#elif CONFIG_LOG_LEVEL == LOG_LEVEL_DEBUG
           "DEBUG"
#elif CONFIG_LOG_LEVEL == LOG_LEVEL_INFO
           "INFO"
#elif CONFIG_LOG_LEVEL == LOG_LEVEL_WARN
           "WARN"
#elif CONFIG_LOG_LEVEL == LOG_LEVEL_ERROR
           "ERROR"
#else
           "FATAL"
#endif
#else
           "(logging disabled)"
#endif
           ,
           CONFIG_LOG_LEVEL);
    printf("  CONFIG_DEBUG        = %d\n", CONFIG_DEBUG);
    printf("  CONFIG_LOG_TIME     = %d\n", CONFIG_LOG_TIME);
    printf("  CONFIG_LOG_COLOR    = %d\n", CONFIG_LOG_COLOR);
    printf("  CONFIG_LOG_FILENAME = %d\n", CONFIG_LOG_FILENAME);
    printf("------------------------------------------------------------\n\n");
}

int main(int argc, char **argv)
{
    int *leaky = NULL;

    (void)argv;
    demo_config_summary();
    log_init();

    LOG_TRACE("trace: finest granularity, removed below CONFIG_LOG_LEVEL");
    LOG_DEBUG("debug: stepping through logic");
    LOG_INFO("info: application started, pid=%d", 4242);
    LOG_WARN("warn: disk space low, %.1f%% of quota used", 92.5);

    printf("\n--- worker calls ---\n");
    worker(3);
    worker(42);

    printf("\n--- zero-cost demonstration ---\n");
    demonstrate_zero_cost();

    printf("\n--- memory demo (CONFIG_DEBUG=%d) ---\n", CONFIG_DEBUG);
    leaky = (int *)dbg_malloc(64 * sizeof *leaky);
    LOG_DEBUG("allocated a leaky buffer at %p", (void *)leaky);
    (void)leaky;

    printf("\n--- assertion demo (CONFIG_DEBUG=%d) ---\n", CONFIG_DEBUG);
    printf("  pass: ");
    int x = 2;
    LOG_ASSERT(x == 2, "x should still be 2");
    printf("  ok\n");
    (void)x;

    if (argc > 1) {
        printf("  deliberate failure:\n");
        LOG_ASSERT(x == 3, "x was changed unexpectedly to %d", x);
    } else {
        printf("  (skipping deliberate failure; rerun with an argument to "
               "see the trap)\n");
    }

    printf("\n--- fatal (terminates the process) ---\n");
    LOG_FATAL("unrecoverable fault in %s", __func__);

    return 0;
}