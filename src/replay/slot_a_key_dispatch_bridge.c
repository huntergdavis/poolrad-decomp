#include "poolrad/replay/slot_a_key_dispatch_bridge.h"

#define PR_SLOT_A_KEY_MAP_WORD 0x1E4u
#define PR_SLOT_A_KEY_WINDOW0_MIN (PR_SLOT_A_KEY_MAP_WORD + 2u)

pr_slot_a_key_return_status pr_slot_a_key_return_step(
    pr_slot_a_key_return_state *state, uint8_t returned_al)
{
    if (!state || !state->window0 ||
        state->window0_size < PR_SLOT_A_KEY_WINDOW0_MIN)
        return PR_SLOT_A_KEY_RETURN_ARGUMENT;

    state->local_key_bp_minus_1 = returned_al; /* +339A */
    state->copied_off_4436 = state->active_off_5d92; /* +339D..+33A4 */
    state->copied_seg_4438 = state->active_seg_5d94; /* +33A0..+33A7 */
    state->window0[PR_SLOT_A_KEY_MAP_WORD] = state->map_id_84dc;
    state->window0[PR_SLOT_A_KEY_MAP_WORD + 1u] = 0u; /* +33AB..+33B8 */
    return PR_SLOT_A_KEY_RETURN_OK;
}

pr_slot_a_key_gate_status pr_slot_a_key_gate_next(
    const pr_slot_a_key_gate_state *state, uint16_t *script_entry_out)
{
    uint16_t word_594;
    if (!state || !script_entry_out || !state->window1 ||
        state->window1_size < 0x596u)
        return PR_SLOT_A_KEY_GATE_ARGUMENT;

    word_594 = (uint16_t)((uint16_t)state->window1[0x594u] |
                          ((uint16_t)state->window1[0x595u] << 8));
    if (state->local_key_bp_minus_1 != 0x48u || word_594 > 1u ||
        state->exit_flag_49ff != 0u)
        return PR_SLOT_A_KEY_GATE_OUTSIDE_SLICE;

    *script_entry_out = state->script_entry_49e3;
    return PR_SLOT_A_KEY_GATE_BLOCKED_SCRIPT_3171;
}
