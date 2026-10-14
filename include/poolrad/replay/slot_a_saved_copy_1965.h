#ifndef POOLRAD_REPLAY_SLOT_A_SAVED_COPY_1965_H
#define POOLRAD_REPLAY_SLOT_A_SAVED_COPY_1965_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_SLOT_A_SAVED_COPY_STEPPED = 0,
    PR_SLOT_A_SAVED_COPY_BLOCKED_0356_ABI,
    PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY,
    PR_SLOT_A_SAVED_COPY_BLOCKED_PC,
    PR_SLOT_A_SAVED_COPY_ARGUMENT
} pr_slot_a_saved_copy_status;

typedef struct {
    unsigned dependency_valid;
    uint16_t caller_unit;
    uint16_t call_pc;
    uint16_t callee_segment; /* original segment, not the runtime relocation */
    uint16_t callee_pc;
    uint16_t return_pc;
} pr_slot_a_saved_copy_report;

/* One native instruction at reached unit-6F 2A8E:1965 or SYSTEM
 * 131C:0B0B..0B22. Inputs must be actual live CPU/RAM at that boundary;
 * this never constructs a caller frame from a save file. Original/loaded
 * code identity is the host's responsibility (private audit supplied).
 *
 * Linear RAM spans must fit ram_size <= 1 MiB and stay within their 64 KiB
 * segment. Unsupported spans block before any CPU/RAM change, including
 * the entire REP instruction. Forward copy order and pointer wrap after
 * individual instructions are preserved. State and report objects must not
 * overlap RAM or each other. XOR CH,CH normalizes undefined AF to observed 0.
 *
 * At 2A8E:196A, BLOCKED_0356_ABI reports the next original call and leaves
 * CPU/RAM unchanged. The existing value-only 0356 translation does not
 * supply its full CPU/stack ABI. All non-STEPPED outcomes preserve state;
 * NULL top-level arguments also preserve report. No game identity credit. */
pr_slot_a_saved_copy_status pr_slot_a_saved_copy_1965_step(
    pr_slot_a_directory_state *state, pr_slot_a_saved_copy_report *report);

#endif
