#ifndef POOLRAD_REPLAY_SLOT_A_LOADER_CAF_GATE_H
#define POOLRAD_REPLAY_SLOT_A_LOADER_CAF_GATE_H
#include "poolrad/replay/slot_a_string_0356_abi.h"
typedef enum {
 PR_LOADER_CAF_STEPPED=0,
 PR_LOADER_CAF_BLOCKED_GETMEM,
 PR_LOADER_CAF_BLOCKED_NEXT_CONTEXT,
 PR_LOADER_CAF_BLOCKED_MEMORY,
 PR_LOADER_CAF_BLOCKED_PC,
 PR_LOADER_CAF_ARGUMENT
} pr_loader_caf_status;
/* One original instruction at loaded 2A8E:CAF..CB9. Requires audited
 * original/loaded code identity and live CPU/RAM. CBA stops BEFORE its far
 * CALL/frame/body with original AF8:023F, return CBF metadata. Zero AL goes
 * via CB3 to a separate D8F next-context stop. No allocator result is supplied.
 * RAM <=1 MiB; multi-byte accesses must fit one segment and host RAM.
 * Effective stack offsets wrap 16 bits; unsupported spans block atomically.
 * Logic AF follows observed zero. State/report/RAM must not overlap.
 * Blocked outcomes preserve CPU/RAM; NULL arguments also preserve report.
 * Bounded adapter only; no new function identity or continuous path claim. */
pr_loader_caf_status pr_slot_a_loader_caf_step(
 pr_slot_a_directory_state *state,pr_slot_a_string_0356_report *report);
#endif
