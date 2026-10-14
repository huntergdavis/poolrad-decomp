#ifndef POOLRAD_REPLAY_SLOT_A_CONTINUATION_1974_H
#define POOLRAD_REPLAY_SLOT_A_CONTINUATION_1974_H
#include "poolrad/replay/slot_a_string_0356_abi.h"

typedef enum {
    PR_SLOT_A_CONTINUATION_1974_STEPPED = 0,
    PR_SLOT_A_CONTINUATION_1974_BLOCKED_B98_ABI,
    PR_SLOT_A_CONTINUATION_1974_BLOCKED_MEMORY,
    PR_SLOT_A_CONTINUATION_1974_BLOCKED_PC,
    PR_SLOT_A_CONTINUATION_1974_ARGUMENT
} pr_slot_a_continuation_1974_status;
typedef pr_slot_a_string_0356_report pr_slot_a_continuation_1974_report;

/* One addressed instruction on reached unit-6F 2A8E:1974 -> 1999,
 * including SYSTEM 131C:B25 and B0B. REP is an atomic host step. The
 * original +1999 -> AF8:B98 call (return +199E) blocks before frame push.
 * Requires audited live CPU/RAM and original/loaded code identity. State,
 * RAM and report must not overlap. Host spans must fit ram_size <= 1 MiB
 * and one 64 KiB segment. Unsupported spans/PCs block with no CPU/RAM
 * change; NULL arguments also preserve report. Reached helpers clear DF;
 * DF-set string steps outside that path block. Undefined logic AF is zero.
 * No game identity, full-C count, UI, or playable-route claim. */
pr_slot_a_continuation_1974_status pr_slot_a_continuation_1974_step(
    pr_slot_a_directory_state *state, pr_slot_a_continuation_1974_report *report);
#endif
