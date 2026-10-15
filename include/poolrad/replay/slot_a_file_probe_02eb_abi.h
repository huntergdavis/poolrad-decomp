#ifndef POOLRAD_REPLAY_SLOT_A_FILE_PROBE_02EB_ABI_H
#define POOLRAD_REPLAY_SLOT_A_FILE_PROBE_02EB_ABI_H
#include "poolrad/replay/slot_a_string_0356_abi.h"

typedef enum {
    PR_SLOT_A_FILE_PROBE_STEPPED = 0,
    PR_SLOT_A_FILE_PROBE_BLOCKED_RUNTIME,
    PR_SLOT_A_FILE_PROBE_BLOCKED_CALLER,
    PR_SLOT_A_FILE_PROBE_BLOCKED_MEMORY,
    PR_SLOT_A_FILE_PROBE_BLOCKED_PC,
    PR_SLOT_A_FILE_PROBE_ARGUMENT
} pr_slot_a_file_probe_status;

/* One instruction at reached 2A8E:19A8 or resident 1026:02EB..0353,
 * including the actual B25 copy. Assign/Reset/Close/IOResult block before
 * pushing their far-call frames and report original dependency addresses.
 * No runtime results, callbacks, replay journals or DOS effects supplied.
 * Independent native states may audit fragments after those boundaries;
 * these are not a composed execution across unsupported calls.
 * At 2A8E:19AD block before any caller instruction, with a zero report.
 * Requires audited code identity and live CPU/RAM. State/report/RAM must
 * not overlap. Spans fit one 64 KiB segment and ram_size <= 1 MiB.
 * Unsupported spans/PCs block with CPU/RAM unchanged; NULL preserves report.
 * REP is atomic, DF-set copy steps outside reached CLD remain unsupported,
 * and undefined logic AF is normalized to zero. No identity/count credit. */
pr_slot_a_file_probe_status pr_slot_a_file_probe_02eb_step(
    pr_slot_a_directory_state *state, pr_slot_a_string_0356_report *report);
#endif
