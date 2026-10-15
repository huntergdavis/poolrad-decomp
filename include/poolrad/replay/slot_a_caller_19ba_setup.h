#ifndef POOLRAD_REPLAY_SLOT_A_CALLER_19BA_SETUP_H
#define POOLRAD_REPLAY_SLOT_A_CALLER_19BA_SETUP_H
#include "poolrad/recovered/slot_a_directory_fullstate.h"
typedef enum {
    PR_SLOT_A_CALLER_19BA_STEPPED = 0,
    PR_SLOT_A_CALLER_19BA_BLOCKED_LOADER,
    PR_SLOT_A_CALLER_19BA_BLOCKED_MEMORY,
    PR_SLOT_A_CALLER_19BA_BLOCKED_PC,
    PR_SLOT_A_CALLER_19BA_ARGUMENT
} pr_slot_a_caller_19ba_status;
typedef struct {
    unsigned dependency_valid;
    uint16_t caller_unit, call_pc, callee_unit, callee_pc, return_pc;
    unsigned call_is_near;
} pr_slot_a_caller_19ba_report;
/* One instruction at loaded unit-6F 2A8E:19BA..19D6, starting from a
 * separately supplied actual allocation return. Blocks at :19D7 before
 * near CALL :0AEC (return :19DA), including its return-word push.
 * No allocator/loader effects, call bridge, identity or count credit.
 * Requires audited original/loaded code and nonoverlapping state/report/RAM.
 * RAM <= 1 MiB; each word must fit its 64 KiB segment and host image.
 * Individual 16-bit address/SP arithmetic wraps. Failed steps preserve
 * CPU/RAM atomically; NULL arguments also preserve report. */
pr_slot_a_caller_19ba_status pr_slot_a_caller_19ba_step(
    pr_slot_a_directory_state *state, pr_slot_a_caller_19ba_report *report);
#endif
