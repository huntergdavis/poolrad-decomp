#ifndef POOLRAD_REPLAY_U3B0_WAIT_048E_H
#define POOLRAD_REPLAY_U3B0_WAIT_048E_H
#include "poolrad/replay/u3b0_prompt_pass.h"
typedef enum {
    PR_U3B0_WAIT_PASS=0, /* nested pass result in diagnostic */
    PR_U3B0_WAIT_ANIMATION, /* before +04AD */
    PR_U3B0_WAIT_CLOCK, /* before +0540 */
    PR_U3B0_WAIT_ARGUMENT
} pr_u3b0_wait_status;
typedef struct { pr_u3b0_pass_status status;pr_u3b0_pass_diagnostic pass; } pr_u3b0_wait_diagnostic;
/* One source-bound +048E wait iteration, then the existing +0566 pass on the
 * no-animation/no-clock route. Caller owns the post-drawer frame, prior
 * prompt effects, bus and all pass services/provenance. A bus word read may
 * update EGA latches. Animation and clock branches stop before their effects;
 * all already completed state changes remain. Never repeats the prompt loop
 * or asserts entry2 provided, full prompt, native parity or gameplay. */
pr_u3b0_wait_status pr_u3b0_wait_048e(pr_bus *bus,size_t ram_size,pr_r0899_cpu *cpu,
    const pr_u3b0_pass_env *env,pr_u3b0_wait_diagnostic *diagnostic);
#endif
