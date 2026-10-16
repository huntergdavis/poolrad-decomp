#ifndef POOLRAD_REPLAY_U6F_1446_FULLSTATE_H
#define POOLRAD_REPLAY_U6F_1446_FULLSTATE_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_U6F_1446_STEPPED = 0,
    PR_U6F_1446_RETURNED,
    PR_U6F_1446_TRANSFER,
    PR_U6F_1446_BLOCKED_COLD_DISPATCH,
    PR_U6F_1446_BLOCKED_THUNK,
    PR_U6F_1446_BLOCKED_PC,
    PR_U6F_1446_BAD_EVENT,
    PR_U6F_1446_ARGUMENT
} pr_u6f_1446_status;

/* Cold INT 3Fh/handler/loader/IRET is an explicit dependency, not a
 * fabricated successful load. The callback reads pre-INT CPU and RAM and
 * returns complete post-handler CPU plus ordered writes, including the
 * real INT frame, loader state, and patched stub. It must not mutate its
 * inputs. STEPPED means a complete event; missing effects must block.
 * This bounded event requires normal return to patched 08D0:0025 with
 * SS:SP restored. Old-byte values, projected INT return frame and far-jump
 * target are validated before any CPU/RAM change. Other dispatch paths
 * are unsupported events; they are not invented normal returns. */
typedef pr_u6f_1446_status (*pr_u6f_1446_cold_dispatch)(
    void *context, const pr_slot_a_directory_cpu *before,
    const uint8_t *ram, size_t ram_size,
    pr_slot_a_directory_cpu *after,
    pr_slot_a_directory_write *writes, size_t capacity, size_t *count);

/* All 313 executable bytes of U6F +1446..157E, its two SYSTEM helpers,
 * warm patched jump, and reviewed U9B +0000 composition. Adjacent inline
 * data is not executed. One source instruction per step except atomic
 * REP, cold dependency event, and the existing U9B composition.
 *
 * Requires disjoint state/journal and exactly 1 MiB A20-off RAM with
 * audited loaded code/data identity. Effective offsets wrap to 16 bits;
 * word/far operands use consecutive physical bytes with A20 wrap, as in
 * the reused insertion/U9B models. Loaded segments: U6F 2A8E, SYSTEM
 * 131C, thunk 08D0, U9B 2CCD. Generic RETF4 honors the live caller frame.
 * FS/GS survive the existing 14-field U9B API. Undefined logic/shift AF
 * and multi-bit shift OF normalize to zero. No host traversal limit.
 * No cold-loader translation or whole-native/strict credit is claimed. */
pr_u6f_1446_status pr_u6f_1446_step(
    pr_slot_a_directory_state *state,
    pr_u6f_1446_cold_dispatch cold_dispatch, void *context,
    pr_slot_a_directory_write *journal, size_t capacity);

#endif
