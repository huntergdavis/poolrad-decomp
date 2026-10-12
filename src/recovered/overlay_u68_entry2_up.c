/* Bounded Up-key route through GAME.OVR unit 0x68 public entry 2.
 * Instruction evidence: docs/re/U68_ENTRY2_UP_SLICE.md. */
#include "poolrad/recovered/overlay_u68_entry2_up.h"

static pr_u68_e2_up_status pr_u68_e2_stop(pr_u68_e2_up_status status,
                                           uint16_t offset, uint16_t *at)
{
    if (at)
        *at = offset;
    return status;
}

pr_u68_e2_up_status pr_u68_entry2_up_slice(pr_u68_e2_up_state *state,
                                            uint8_t *al_out,
                                            uint16_t *stop_offset)
{
    static const uint8_t controls[5] = {0x0Du, 0x0Au, 0x0Fu, 1u, 0u};
    static const uint8_t exit_words_low[4] = {1u, 0x11u, 0x26u, 0x16u};
    pr_u68_e2_key_event event = {0, 0, 0};

    if (!state || !al_out || !state->preflight.window1 ||
        state->preflight.window1_size < PR_U68_E2_WINDOW1_MIN_SIZE)
        return pr_u68_e2_stop(PR_U68_E2_UP_ARGUMENT, 0x098Fu, stop_offset);

    /* +0x098F..+0x0995: clear window-1 word 0x592 before selector gate. */
    state->preflight.window1[PR_U68_E2_WORD_592] = 0;
    state->preflight.window1[PR_U68_E2_WORD_592 + 1u] = 0;
    if (state->selector_49fa != 1u) /* +0x099A..+0x09A1 */
        return pr_u68_e2_stop(PR_U68_E2_UP_OUTSIDE_SLICE, 0x0BACu, stop_offset);

    /* +0x09A4..+0x09D0: local flag=0, string copy then prompt. Both
     * external calls must be host-provided before an input result exists. */
    if (!state->hooks || !state->hooks->prepare_prompt ||
        state->hooks->prepare_prompt(state->hooks->ctx, state,
                                      PR_U68_E2_PROMPT_SOURCE_CS) != 0)
        return pr_u68_e2_stop(PR_U68_E2_UP_HOOK, 0x09B2u, stop_offset);
    if (!state->hooks->key_input ||
        state->hooks->key_input(state->hooks->ctx, state,
                                 PR_U68_E2_PROMPT_MESSAGE_DS, controls,
                                 &event) != 0)
        return pr_u68_e2_stop(PR_U68_E2_UP_HOOK, 0x09D0u, stop_offset);
    if (!event.provided)
        return pr_u68_e2_stop(PR_U68_E2_UP_HOOK, 0x09D0u, stop_offset);

    /* +0x09D5..+0x0AB2: only the extended Up branch belongs to this
     * slice. Other key/selector branches are deliberately unmodelled. */
    if (!event.extended)
        return pr_u68_e2_stop(PR_U68_E2_UP_OUTSIDE_SLICE, 0x09E1u, stop_offset);
    if (event.key_al != PR_U68_E2_KEY_UP)
        return pr_u68_e2_stop(PR_U68_E2_UP_OUTSIDE_SLICE, 0x0ABCu, stop_offset);
    if (pr_u68_up_preflight(&state->preflight, NULL) != PR_U68_UP_OK)
        return pr_u68_e2_stop(PR_U68_E2_UP_PREFLIGHT, 0x0AB2u, stop_offset);

    /* +0x0AB5..+0x0BA9: [BP-2]=1; common resident call then take exit. */
    if (!state->hooks->resident_290b ||
        state->hooks->resident_290b(state->hooks->ctx, state) != 0)
        return pr_u68_e2_stop(PR_U68_E2_UP_HOOK, 0x0B9Bu, stop_offset);

    /* +0x0D1C..+0x0D3F: resident effects are visible before the selector
     * copy and optional far exit call. The latter precedes flag_84e4=1. */
    state->previous_selector_49f9 = state->selector_49fa;
    if (state->flag_84e4 == 0u) {
        if (!state->hooks->exit_3f1_0137 ||
            state->hooks->exit_3f1_0137(state->hooks->ctx, state,
                                         exit_words_low) != 0)
            return pr_u68_e2_stop(PR_U68_E2_UP_HOOK, 0x0D35u, stop_offset);
        state->flag_84e4 = 1u;
    }

    *al_out = event.key_al; /* +0x0D3F..+0x0D4B: AL=prompt key */
    return pr_u68_e2_stop(PR_U68_E2_UP_OK, 0x0D4Cu, stop_offset);
}
