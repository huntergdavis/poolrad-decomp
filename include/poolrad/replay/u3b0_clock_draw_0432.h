#ifndef POOLRAD_REPLAY_U3B0_CLOCK_DRAW_0432_H
#define POOLRAD_REPLAY_U3B0_CLOCK_DRAW_0432_H
#include "poolrad/replay/u3b0_draw_wait_047a.h"
#include "poolrad/recovered/resident_0709_time_0c3f_cpu.h"
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"

typedef struct {
    pr_dos_gettime_service time_service;
    void *time_context;
    size_t printer_loopbacks, glyph_loopbacks;
    pr_u3b0_draw_wait_env draw;
} pr_u3b0_clock_draw_env;
typedef enum { PR_U3B0_CLOCK_DRAW_REACHED=0, PR_U3B0_CLOCK_DRAW_ARGUMENT,
    PR_U3B0_CLOCK_DRAW_CODE, PR_U3B0_CLOCK_DRAW_CALLEE, PR_U3B0_CLOCK_DRAW_TRANSFER } pr_u3b0_clock_draw_status;
typedef struct {
    int phase; /* 0 first clock, 1 second clock, 2 label, 3 drawer */
    pr_r08ae_time_cpu_status clock_status;
    pr_r08ae_time_cpu_diagnostic clock;
    pr_r0709_printer_cpu_status printer_status;
    pr_r0709_printer_cpu_diagnostic printer;
    pr_u3b0_draw_wait_status draw_status;
    pr_u3b0_draw_wait_diagnostic draw;
} pr_u3b0_clock_draw_diagnostic;
/* Caller supplies the genuine +0432 prompt frame, loaded code/relocations,
 * live data, DOS service and all downstream services. Executes two clocks,
 * optional label printer and +047A drawer/wait; nested stops retain effects.
 * See U3B0_CLOCK_DRAW_0432_DIAGNOSTIC.md for profile and limitations. */
pr_u3b0_clock_draw_status pr_u3b0_clock_draw_0432(pr_bus *bus, size_t ram_size,
    pr_r0899_cpu *cpu, const pr_u3b0_clock_draw_env *env, pr_u3b0_clock_draw_diagnostic *diagnostic);
#endif
