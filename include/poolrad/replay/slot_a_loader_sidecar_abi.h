#ifndef POOLRAD_REPLAY_SLOT_A_LOADER_SIDECAR_ABI_H
#define POOLRAD_REPLAY_SLOT_A_LOADER_SIDECAR_ABI_H
#include "poolrad/replay/slot_a_file_probe_02eb_abi.h"
typedef enum {
 PR_SLOT_A_SIDECAR_STEPPED=0, PR_SLOT_A_SIDECAR_BLOCKED_ASSIGN,
 PR_SLOT_A_SIDECAR_BLOCKED_ALLOCATOR, PR_SLOT_A_SIDECAR_BLOCKED_PRINTER,
 PR_SLOT_A_SIDECAR_BLOCKED_NEXT_CONTEXT, PR_SLOT_A_SIDECAR_BLOCKED_MEMORY,
 PR_SLOT_A_SIDECAR_BLOCKED_PC, PR_SLOT_A_SIDECAR_ARGUMENT
} pr_slot_a_sidecar_status;
typedef pr_slot_a_string_0356_report pr_slot_a_sidecar_report;
/* Native unit6F C8F/D8F -> B0B/B98 -> CAA/DAA -> 02EB/B25, stopping
 * before actual Assign0310. Separate return API starts at independently
 * captured CAF/DAF, and stops before heap/printer/next-context execution.
 * No runtime result, DOS journal or continuous join across unsupported calls.
 * Requires audited original/loaded code and disjoint state/report/RAM.
 * Every accessed span fits one 64KiB segment and RAM<=1MiB. REP is atomic;
 * undefined logic AF follows observed zero. Blocked outcomes preserve CPU/RAM;
 * NULL arguments preserve report. No identity/count/playable credit. */
pr_slot_a_sidecar_status pr_slot_a_loader_sidecar_prefix_step(
 pr_slot_a_directory_state *state,pr_slot_a_sidecar_report *report);
pr_slot_a_sidecar_status pr_slot_a_loader_sidecar_return_step(
 pr_slot_a_directory_state *state,pr_slot_a_sidecar_report *report);
#endif
