#ifndef POOLRAD_REPLAY_U3B0_DRAW_WAIT_047A_H
#define POOLRAD_REPLAY_U3B0_DRAW_WAIT_047A_H
#include "poolrad/replay/u3b0_wait_048e.h"
typedef struct { pr_u3b0_01da_limits drawer;pr_u3b0_pass_env pass; } pr_u3b0_draw_wait_env;
typedef enum { PR_U3B0_DRAW_WAIT_REACHED=0,PR_U3B0_DRAW_WAIT_ARGUMENT,
    PR_U3B0_DRAW_WAIT_CODE,PR_U3B0_DRAW_WAIT_CALLEE,PR_U3B0_DRAW_WAIT_TRANSFER } pr_u3b0_draw_wait_status;
typedef struct {
    pr_u3b0_01da_status drawer_status;pr_u3b0_01da_diagnostic drawer;
    pr_u3b0_wait_status wait_status;pr_u3b0_wait_diagnostic wait;
} pr_u3b0_draw_wait_diagnostic;
/* Live +047A state after prior prompt setup, clock and any reached label
 * drawing. Caller owns source identity, state, services and all earlier
 * effects; no boolean establishes provenance. Composes real +01DA then wait.
 * Existing bus, flat-RAM, code-stability and drawer profiles apply; nested
 * nonreturns retain all earlier effects. REACHED reports wait_status, not a
 * completed prompt or entry2 hook. No arbitrary-PC resume or parity claim. */
pr_u3b0_draw_wait_status pr_u3b0_draw_wait_047a(pr_bus *bus,size_t ram_size,
    pr_r0899_cpu *cpu,const pr_u3b0_draw_wait_env *env,pr_u3b0_draw_wait_diagnostic *diagnostic);
#endif
