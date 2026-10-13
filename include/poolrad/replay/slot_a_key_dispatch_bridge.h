#ifndef POOLRAD_REPLAY_SLOT_A_KEY_DISPATCH_BRIDGE_H
#define POOLRAD_REPLAY_SLOT_A_KEY_DISPATCH_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

/* Live caller state at unit 0x1C:+0x339A, after entry 2 returns. The host
 * supplies the returned AL and the current DS fields. These are not inferred
 * from the saved world file: loader, menu, and gameplay initialization run
 * between that file transfer and this call. */
typedef struct {
    uint8_t *window0;       /* live far [DS:49D2], not the raw saved frame */
    size_t window0_size;
    uint8_t map_id_84dc;
    uint16_t active_off_5d92;
    uint16_t active_seg_5d94;
    uint16_t copied_off_4436;
    uint16_t copied_seg_4438;
    uint8_t local_key_bp_minus_1;
} pr_slot_a_key_return_state;

typedef enum {
    PR_SLOT_A_KEY_RETURN_OK = 0,
    PR_SLOT_A_KEY_RETURN_ARGUMENT
} pr_slot_a_key_return_status;

/* Apply the nine original instructions at +0x339A..+0x33B8 in order. The
 * result is before +0x33B9 LES of the window-1 pointer. The call to entry 2
 * and every later gameplay effect remain outside this step. A bad host
 * window leaves state unchanged. */
pr_slot_a_key_return_status pr_slot_a_key_return_step(
    pr_slot_a_key_return_state *state, uint8_t returned_al);

#endif
