#ifndef POOLRAD_REPLAY_U3B0_TEXT_0634_H
#define POOLRAD_REPLAY_U3B0_TEXT_0634_H
#include "poolrad/recovered/overlay_u3b0_entry_01da_cpu.h"
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"
typedef struct { size_t outer_jumps, inner_jumps; pr_u3b0_01da_limits drawer; } pr_u3b0_text_limits;
typedef enum {
    PR_U3B0_TEXT_BOUNDARY=0, /* stopped before +0691 */
    PR_U3B0_TEXT_ARGUMENT,
    PR_U3B0_TEXT_LIMIT, /* before +0677 or taken +068F jump; not a return */
    PR_U3B0_TEXT_CODE,
    PR_U3B0_TEXT_CALLEE,
    PR_U3B0_TEXT_TRANSFER
} pr_u3b0_text_status;
typedef struct {
    size_t outer_jumps,inner_jumps;uint16_t site;
    pr_u3b0_01da_status status;pr_u3b0_01da_diagnostic drawer;
} pr_u3b0_text_diagnostic;
/* Live +0634 CPU/frame and shared RAM/EGA bus; caller supplies source identity,
 * prior effects and drawer environment. Budgets count taken outer JNE at
 * +068F and inner JMP at +0677; a limit stops before that jump after earlier
 * effects. Nested stops preserve state. No arbitrary-PC resume, prompt return,
 * entry2 provided, native parity or proof of prior effects is implied. */
pr_u3b0_text_status pr_u3b0_text_0634(pr_bus *bus,size_t ram_size,pr_r0899_cpu *cpu,
    const pr_u3b0_text_limits *limits,pr_u3b0_text_diagnostic *diagnostic);
#endif
