#ifndef POOLRAD_REPLAY_SLOT_A_STRING_0425_ABI_H
#define POOLRAD_REPLAY_SLOT_A_STRING_0425_ABI_H

#include "poolrad/replay/slot_a_string_0356_abi.h"

typedef enum {
    PR_SLOT_A_STRING_0425_STEPPED = 0,
    PR_SLOT_A_STRING_0425_BLOCKED_CALLER_CONTINUATION,
    PR_SLOT_A_STRING_0425_BLOCKED_DELETE,
    PR_SLOT_A_STRING_0425_BLOCKED_MEMORY,
    PR_SLOT_A_STRING_0425_BLOCKED_PC,
    PR_SLOT_A_STRING_0425_ARGUMENT
} pr_slot_a_string_0425_status;

typedef pr_slot_a_string_0356_report pr_slot_a_string_0425_report;

/* One original instruction on 2A8E:196F -> 1026:0425 -> 2A8E:1974,
 * including nested +0356 and SYSTEM helpers. REP is one atomic host step.
 * +1974 blocks before the caller continuation prefix; its report names the
 * following +197E -> SYSTEM 0B25 call, return +1983. Delete still blocks
 * before its frame push. Every blocked outcome preserves CPU and RAM.
 * Actual audited live CPU/RAM and original/loaded code identity are required.
 * State/report must not alias each other or RAM. Spans must fit one 64 KiB
 * segment and ram_size <= 1 MiB. Incoming DF is cleared by reached CLD;
 * DF-set STOS/REP outside those reached paths blocks. Undefined logic AF
 * is normalized to zero. This is a bounded ABI adapter, not a count claim. */
pr_slot_a_string_0425_status pr_slot_a_string_0425_step(
    pr_slot_a_directory_state *state, pr_slot_a_string_0425_report *report);

#endif
