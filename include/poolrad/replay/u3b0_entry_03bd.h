#ifndef POOLRAD_REPLAY_U3B0_ENTRY_03BD_H
#define POOLRAD_REPLAY_U3B0_ENTRY_03BD_H
#include "poolrad/replay/u3b0_clock_draw_0432.h"
#include "poolrad/recovered/overlay_u3b0_item_split_00cb_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
typedef enum { PR_U3B0_ENTRY_REACHED=0, PR_U3B0_ENTRY_ARGUMENT, PR_U3B0_ENTRY_CODE,
    PR_U3B0_ENTRY_PROFILE, PR_U3B0_ENTRY_CALLEE, PR_U3B0_ENTRY_TRANSFER } pr_u3b0_entry_status;
typedef struct {
    int phase; /* 0,1 string copies; 2 item split; 3 clock/draw */
    pr_system_0b25_status store_status;
    pr_u3b0_00cb_status split_status; int split_callee_status;
    pr_u3b0_clock_draw_status clock_draw_status; pr_u3b0_clock_draw_diagnostic clock_draw;
} pr_u3b0_entry_diagnostic;
/* Starts at source +03BD using the caller's actual far frame and arguments.
 * Composes real setup, clocks and the existing single-pass prompt pipeline.
 * See U3B0_ENTRY_03BD_DIAGNOSTIC.md for profiles and bounded stops. */
pr_u3b0_entry_status pr_u3b0_entry_03bd(pr_bus *bus,size_t ram_size,pr_r0899_cpu *cpu,
    const pr_u3b0_clock_draw_env *env,pr_u3b0_entry_diagnostic *diagnostic);
#endif
