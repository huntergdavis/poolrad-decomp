#ifndef POOLRAD_REPLAY_SLOT_A_POST_TRANSFER_H
#define POOLRAD_REPLAY_SLOT_A_POST_TRANSFER_H

/* GAME.OVR unit 0x6F +0x18EA..+0x191D, the straight-line post-transfer
 * state step after the save-handle close. The ten transfer spans have
 * already been copied into pr_slot_a_native_state. This step does not
 * perform the close, party loading, sidecar loading, menu return or game
 * loop initialization. See docs/re/SLOT_A_POST_TRANSFER.md. */
#include <stdint.h>

#include "poolrad/replay/slot_a_native_save.h"

#define PR_SLOT_A_POST_W0_WORD_1FE 0x1FEu
#define PR_SLOT_A_POST_W0_BYTE_1F8 0x1F8u
#define PR_SLOT_A_POST_W1_WORD_67C 0x67Cu

typedef struct {
    uint8_t b49f6; /* low byte of logical shift-right-one of window-0 word +0x1FE */
    uint8_t b49f5; /* low bit of that same word */
    uint8_t b49e2; /* window-0 byte +0x1F8 */
} pr_slot_a_post_transfer_ds;

typedef enum {
    PR_SLOT_A_POST_TRANSFER_OK = 0,
    PR_SLOT_A_POST_TRANSFER_ARGUMENT
} pr_slot_a_post_transfer_status;

/* Applies only the 14 instructions in the span. On a null argument, neither
 * object is changed. The word clear is intentionally before the later party
 * loop, which may increment the same window-1 word once per loaded member. */
pr_slot_a_post_transfer_status pr_slot_a_post_transfer_apply(
    pr_slot_a_native_state *world, pr_slot_a_post_transfer_ds *ds);

#endif
