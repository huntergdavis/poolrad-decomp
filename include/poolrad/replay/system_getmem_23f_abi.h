#ifndef POOLRAD_REPLAY_SYSTEM_GETMEM_23F_ABI_H
#define POOLRAD_REPLAY_SYSTEM_GETMEM_23F_ABI_H
#include "poolrad/recovered/slot_a_directory_fullstate.h"
typedef enum {
    PR_SYSTEM_GETMEM_STEPPED = 0,
    PR_SYSTEM_GETMEM_BLOCKED_CORE,
    PR_SYSTEM_GETMEM_BLOCKED_NORMALIZER,
    PR_SYSTEM_GETMEM_BLOCKED_RUNTIME_ERROR,
    PR_SYSTEM_GETMEM_BLOCKED_CALLER,
    PR_SYSTEM_GETMEM_BLOCKED_MEMORY,
    PR_SYSTEM_GETMEM_BLOCKED_PC,
    PR_SYSTEM_GETMEM_ARGUMENT
} pr_system_getmem_status;
typedef struct {
    unsigned dependency_valid;
    unsigned call_is_near;
    uint16_t call_pc;
    uint16_t callee_segment;
    uint16_t callee_pc;
    uint16_t return_pc;
} pr_system_getmem_report;
/* One original instruction in three independent audited fragments:
 * 2A8E:19B5 -> 131C:0245 wrapper prefix; 131C:030C requested-size store;
 * and 131C:0363 split writers through the real return to 2A8E:19BA.
 * Core :0245 -> :0308 and normalizer :030F -> :0491 block before near
 * frame pushes. Report uses original AF8 and marks near calls explicitly.
 * :00E2 blocks before runtime-error effects; :19BA before caller stores.
 * No snapshot/result bridge, core selection, normalization or heap outcome
 * injection is provided. Split return DX:AX comes from the actual stack.
 * Requires audited original/loaded code and live CPU/RAM, with state,
 * report and RAM nonoverlapping. Spans fit one 64 KiB segment and image
 * <= 1 MiB. Unsupported states preserve CPU/RAM; NULL preserves report.
 * No full-core, game identity, playable-route or count credit. */
pr_system_getmem_status pr_system_getmem_23f_step(
    pr_slot_a_directory_state *state, pr_system_getmem_report *report);
#endif
