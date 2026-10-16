#ifndef POOLRAD_REPLAY_U6F_02AE_FULLSTATE_H
#define POOLRAD_REPLAY_U6F_02AE_FULLSTATE_H

#include "poolrad/replay/u6f_02ae_abi.h"

typedef enum {
    PR_U6F_02AE_FULL_STEPPED = 0,
    PR_U6F_02AE_FULL_RETURNED,
    PR_U6F_02AE_FULL_BLOCKED_UI,
    PR_U6F_02AE_FULL_BLOCKED_DOS,
    PR_U6F_02AE_FULL_BLOCKED_MEMORY,
    PR_U6F_02AE_FULL_BLOCKED_PC,
    PR_U6F_02AE_FULL_BAD_EVENT,
    PR_U6F_02AE_FULL_ARGUMENT
} pr_u6f_02ae_full_status;

/* Explicit whole RES_0709:0BF5 display/key dependency. Called AFTER the
 * real far CALL has pushed its frame, at loaded 0F2D:0BF5. Read-only inputs;
 * supply every post-callee CPU field and ordered byte write, including
 * callee stack/local/helper/display/key effects. Returning STEPPED promises
 * completeness; a partial event must block. Old-byte chains and projected
 * RETF6 frame/normal caller return/SS:SP are validated atomically. Only a
 * normal return to 2A8E:031F is supported. Missing callback blocks without
 * changes. This contract does not translate, infer or verify UI effects. */
typedef pr_u6f_02ae_full_status (*pr_u6f_02ae_ui_service)(
    void *context, const pr_slot_a_directory_cpu *before,
    const uint8_t *ram, size_t ram_size,
    pr_slot_a_directory_cpu *after,
    pr_slot_a_directory_write *writes, size_t capacity, size_t *count);

/* All 129 bytes / 51 instructions of the U6F +02AE caller, composing
 * the unchanged entry/Copy/mkdir/IOResult model and B0B/C3B/B98 adapters.
 * One original instruction per step except atomic REP/DOS/UI events.
 * Generic outer RETF, live SS locals, retry and AH on MOV AL are retained.
 * Uses their bounded linear spans: RAM <= 1MiB, operands must fit one
 * segment without A20 wrap. Unsupported spans/DF/PCs block before changes.
 * State, RAM and journal must be disjoint; callbacks may not mutate inputs.
 * Loaded code identity must be host-audited: U6F=2A8E, SYSTEM=131C,
 * resident=0F2D. Undefined logic AF normalizes to zero. No host retry cap.
 * DOS and UI remain supplied dependencies, with no whole-native/strict
 * credit or natural nonzero/nonfive error-route claim. */
pr_u6f_02ae_full_status pr_u6f_02ae_fullstate_step(
    pr_slot_a_directory_state *state,
    pr_slot_a_directory_service dos, void *dos_context,
    pr_u6f_02ae_ui_service ui, void *ui_context,
    pr_slot_a_directory_write *journal, size_t capacity);

#endif
