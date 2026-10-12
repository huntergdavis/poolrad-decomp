/* The first in-memory state transition after the ten world-save transfers;
 * original instruction map: docs/re/SLOT_A_POST_TRANSFER.md. */
#include "poolrad/replay/slot_a_post_transfer.h"

pr_slot_a_post_transfer_status pr_slot_a_post_transfer_apply(
    pr_slot_a_native_state *world, pr_slot_a_post_transfer_ds *ds)
{
    uint16_t word;
    if (!world || !ds) return PR_SLOT_A_POST_TRANSFER_ARGUMENT;

    /* +0x18EA..+0x18F5: ES:[DI+0x1FE], SHR AX, store low AL. */
    word = (uint16_t)((uint16_t)world->window0[PR_SLOT_A_POST_W0_WORD_1FE] |
                      ((uint16_t)world->window0[PR_SLOT_A_POST_W0_WORD_1FE + 1u] << 8));
    ds->b49f6 = (uint8_t)(word >> 1);

    /* +0x18F8..+0x1904: reload the word, AND AX,1, store low AL. */
    word = (uint16_t)((uint16_t)world->window0[PR_SLOT_A_POST_W0_WORD_1FE] |
                      ((uint16_t)world->window0[PR_SLOT_A_POST_W0_WORD_1FE + 1u] << 8));
    ds->b49f5 = (uint8_t)(word & 1u);

    /* +0x1907..+0x1910: one byte from the first window. */
    ds->b49e2 = world->window0[PR_SLOT_A_POST_W0_BYTE_1F8];

    /* +0x1913..+0x191D: AX=0, store a 16-bit word to the second window. */
    world->window1[PR_SLOT_A_POST_W1_WORD_67C] = 0u;
    world->window1[PR_SLOT_A_POST_W1_WORD_67C + 1u] = 0u;
    return PR_SLOT_A_POST_TRANSFER_OK;
}
