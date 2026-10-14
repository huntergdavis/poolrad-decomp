#ifndef POOLRAD_REPLAY_SLOT_A_STRING_0356_ABI_H
#define POOLRAD_REPLAY_SLOT_A_STRING_0356_ABI_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_SLOT_A_STRING_0356_STEPPED = 0,
    PR_SLOT_A_STRING_0356_BLOCKED_0425_ABI,
    PR_SLOT_A_STRING_0356_BLOCKED_DELETE,
    PR_SLOT_A_STRING_0356_BLOCKED_MEMORY,
    PR_SLOT_A_STRING_0356_BLOCKED_PC,
    PR_SLOT_A_STRING_0356_ARGUMENT
} pr_slot_a_string_0356_status;

typedef struct {
    unsigned dependency_valid;
    uint16_t caller;
    uint16_t call_pc;
    uint16_t callee_segment;
    uint16_t callee_pc;
    uint16_t return_pc;
    unsigned caller_is_overlay;
} pr_slot_a_string_0356_report;

/* One instruction on the reached saved-A 2A8E:196A -> 1026:0356 route,
 * including its SYSTEM 131C:0B25, 0B57 and 0C10 helpers. REP instructions
 * are atomic host steps. The Delete call at resident +03FC composes the
 * complete SYSTEM CBC body and its B57/B98/B25 dependencies. Return to
 * 2A8E:196F blocks before resident 0802:0425, preserving CPU/RAM and
 * reporting that ABI.
 *
 * Requires actual live CPU/RAM and audited original/loaded code identity.
 * State/report objects must not overlap each other or RAM. Host spans must
 * fit ram_size <= 1 MiB and one 64 KiB segment; unsupported spans block
 * atomically at the current instruction. DF-set string steps are outside
 * this reached slice. Undefined logic AF is normalized to observed zero.
 * All blocked outcomes preserve state; NULL arguments also preserve report.
 * This is a bounded native ABI adapter, not a full-C identity claim. */
pr_slot_a_string_0356_status pr_slot_a_string_0356_step(
    pr_slot_a_directory_state *state, pr_slot_a_string_0356_report *report);

/* Historical prefix-only entry point, retained for the immutable PR410 ABI
 * audit. It reports BLOCKED_DELETE before pushing the +03FC return frame;
 * normal callers use pr_slot_a_string_0356_step to execute composed Delete. */
pr_slot_a_string_0356_status pr_slot_a_string_0356_prefix_step(
    pr_slot_a_directory_state *state, pr_slot_a_string_0356_report *report);

#endif
