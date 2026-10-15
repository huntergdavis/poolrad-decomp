#ifndef POOLRAD_REPLAY_SYSTEM_GETMEM_HOOK_B0B_JOIN_H
#define POOLRAD_REPLAY_SYSTEM_GETMEM_HOOK_B0B_JOIN_H
#include "poolrad/replay/system_getmem_real_hook_abi.h"
typedef enum {
    PR_GETMEM_HOOK_B0B_STEPPED = 0,
    PR_GETMEM_HOOK_B0B_BLOCKED_PRINTER,
    PR_GETMEM_HOOK_B0B_BLOCKED_POSTHOOK,
    PR_GETMEM_HOOK_B0B_BLOCKED_WRAPPER,
    PR_GETMEM_HOOK_B0B_BLOCKED_TARGET,
    PR_GETMEM_HOOK_B0B_BLOCKED_MEMORY,
    PR_GETMEM_HOOK_B0B_BLOCKED_PC,
    PR_GETMEM_HOOK_B0B_ARGUMENT
} pr_getmem_hook_b0b_status;
/* One original instruction in the real-hook adapter, callback 398A:005F
 * far CALL, or existing SYSTEM B0B helper. REP remains one atomic step.
 * No snapshot bridge: actual CPU/RAM and the existing helper's real RETF 4
 * carry execution to callback :0064, blocked before original 0709:0BF5
 * (return :0069). Nonzero :03C7 retry/error remains blocked for every AL.
 * The zero-argument route and unknown-target/frame guards stay delegated
 * to the unchanged hook adapter. Requires pinned original/loaded code,
 * nonoverlapping state/report/RAM, RAM <= 1 MiB and spans within 64 KiB.
 * Gates preserve CPU/RAM; NULL arguments preserve report. No full-C,
 * continuous allocator, identity/count or fabricated hook-outcome credit. */
pr_getmem_hook_b0b_status pr_system_getmem_hook_b0b_step(
    pr_slot_a_directory_state *state, pr_system_getmem_report *report);
#endif
