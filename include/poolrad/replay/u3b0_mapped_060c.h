#ifndef POOLRAD_REPLAY_U3B0_MAPPED_060C_H
#define POOLRAD_REPLAY_U3B0_MAPPED_060C_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
typedef enum {
    PR_U3B0_MAPPED_BOUNDARY=0, /* stopped at +06CF */
    PR_U3B0_MAPPED_TEXT_SEARCH, /* stopped before +0634 */
    PR_U3B0_MAPPED_ARGUMENT,
    PR_U3B0_MAPPED_CODE,
    PR_U3B0_MAPPED_LEAF,
    PR_U3B0_MAPPED_TRANSFER
} pr_u3b0_mapped_status;
typedef struct { uint16_t system_segment; } pr_u3b0_mapped_env;
typedef struct { uint16_t site; pr_system_leaf_status status; } pr_u3b0_mapped_diagnostic;
/* Live source-bound +060C state; caller owns CPU/bus, loaded sets/table,
 * stack, prior prompt effects and stable code. Models UpCase, membership,
 * space and gated mapped-key path; stops before list search or +06CF. Caller
 * supplies relocated SYSTEM segment and stable code. Non-aperture CS/DS/SS
 * windows and nonwrapping stack are required for flat-RAM SYSTEM leaves.
 * Reached target/return checks and partial effects are retained on failure.
 * This is not a full prompt, entry2 hook or proof of prior effects. */
pr_u3b0_mapped_status pr_u3b0_mapped_060c(pr_bus *bus,size_t ram_size,
    pr_r0899_cpu *cpu,const pr_u3b0_mapped_env *env,pr_u3b0_mapped_diagnostic *diagnostic);
#endif
