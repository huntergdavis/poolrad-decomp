#ifndef POOLRAD_REPLAY_U6F_01B8_ENTRY_ABI_H
#define POOLRAD_REPLAY_U6F_01B8_ENTRY_ABI_H

#include "poolrad/replay/u6f_02ae_abi.h"

typedef enum {
    PR_U6F_01B8_ENTRY_STEPPED = 0,
    PR_U6F_01B8_ENTRY_BOUNDARY,
    PR_U6F_01B8_ENTRY_TRANSFER,
    PR_U6F_01B8_ENTRY_BLOCKED_DOS,
    PR_U6F_01B8_ENTRY_BLOCKED_UI,
    PR_U6F_01B8_ENTRY_BLOCKED_MEMORY,
    PR_U6F_01B8_ENTRY_BLOCKED_PC,
    PR_U6F_01B8_ENTRY_BAD_EVENT,
    PR_U6F_01B8_ENTRY_ARGUMENT
} pr_u6f_01b8_entry_status;

/* Original U6F [01B8,01C3): 11 bytes / five instructions, then the
 * unchanged bounded pr_u6f_02ae_step composition. Boundary means actual
 * normal callee return to loaded 2A8E:01C3, BEFORE that instruction.
 * Supplying a state already at that PC also reports the boundary; this
 * stateless stepper does not certify the state's execution history.
 * A callee RETF through a changed live frame reports TRANSFER.
 *
 * One instruction per step except reused atomic REP/DOS events. Missing
 * DOS blocks at 131C:1821; UI is unsupported and blocks BEFORE 02E9 setup.
 * Complete DOS events obey the existing read-only/ordered-write contract.
 * Every blocked/rejected step preserves CPU/RAM, including failed CALL
 * stack access; previous completed instructions remain committed.
 *
 * Inputs/state/journal must be disjoint and loaded code identity audited.
 * RAM <= 1 MiB; operand spans fit RAM and a segment without A20 wrap.
 * Reused DF restrictions and undefined logic-AF normalization apply.
 * All 16 CPU fields survive unless the real instruction/service changes
 * them. Generic caller frame/arguments remain live; no frame is invented.
 * This stops before file setup, retry/list traversal, direct UI +0239 or
 * outer RETF4. No native, whole-function or strict credit is claimed. */
pr_u6f_01b8_entry_status pr_u6f_01b8_entry_step(
    pr_slot_a_directory_state *state,
    pr_slot_a_directory_service service, void *context,
    pr_slot_a_directory_write *journal, size_t capacity);

#endif
