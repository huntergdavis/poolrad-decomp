/* Portable C model of GAME.OVR unit 0x68 +0x04DF.
 * Instruction and caller evidence: docs/re/U68_MOVEMENT_FLAG_04DF.md. */
#include "poolrad/recovered/overlay_u68_movement_flag.h"

#include "poolrad/recovered/overlay_u68_record_scan.h"

static int pr_u68_move_null(pr_u68_far p)
{
    return (p.offset | p.segment) == 0u;
}

static const pr_u68_segment_view *pr_u68_move_view(const pr_u68_move_state *state,
                                                    uint16_t selector)
{
    size_t i;
    for (i = 0; i < state->view_count; ++i)
        if (state->views[i].selector == selector)
            return &state->views[i];
    return NULL;
}

static uint16_t pr_u68_move_word(const uint8_t *bytes, uint16_t offset)
{
    return (uint16_t)(bytes[offset] | ((uint16_t)bytes[(unsigned)offset + 1u] << 8));
}

static int pr_u68_move_signed_byte(uint8_t value)
{
    return value < 0x80u ? (int)value : (int)value - 256;
}

pr_u68_move_status pr_u68_movement_flag(pr_u68_move_state *state,
                                         uint8_t *al_out)
{
    pr_u68_far current;
    size_t visited_count = 0;
    uint8_t found = 0;

    if (!state || !al_out)
        return PR_U68_MOVE_ARGUMENT;
    current = state->head; /* +0x04E9..+0x04F3 */

    /* +0x04F6..+0x0543: null or first hit ends the loop. Even on a hit,
     * the current node's next far pointer is loaded before loop exit. */
    while (!pr_u68_move_null(current) && !found) {
        const pr_u68_segment_view *view;
        uint16_t next_at, next_segment_at;
        uint8_t roll;
        size_t i;

        if (!state->views || !state->view_count || !state->visited ||
            !state->visited_capacity)
            return PR_U68_MOVE_ARGUMENT;
        for (i = 0; i < visited_count; ++i)
            if (state->visited[i].offset == current.offset &&
                state->visited[i].segment == current.segment)
                return PR_U68_MOVE_CYCLE;
        if (visited_count == state->visited_capacity)
            return PR_U68_MOVE_CAPACITY;
        state->visited[visited_count++] = current;

        view = pr_u68_move_view(state, current.segment);
        if (!view || !view->bytes || view->size < PR_U68_RECORD_SEGMENT_SIZE)
            return PR_U68_MOVE_ARGUMENT;
        if (!state->hooks.roll_9e_entry8)
            return PR_U68_MOVE_HOOK;

        /* +0x050D..+0x051F: set AL=1, push AX; set AL=0x64, push AX;
         * far call 0xB0:0x48. AH is not fixed here, but the callee reads
         * only the low bytes. Its RNG effects remain behind the hook. */
        roll = state->hooks.roll_9e_entry8(state->hooks.ctx, 0x64u, 1u);
        if (roll <= view->bytes[(uint16_t)(current.offset + 0x78u)] &&
            view->bytes[(uint16_t)(current.offset + 0x10Cu)] == 0)
            found = 1; /* +0x0521..+0x052C */

        /* +0x0530..+0x0543: advance even after found. Host views do not
         * represent a word beginning at offset FFFF. */
        next_at = (uint16_t)(current.offset + 0x104u);
        next_segment_at = (uint16_t)(current.offset + 0x106u);
        if (next_at == 0xFFFFu || next_segment_at == 0xFFFFu)
            return PR_U68_MOVE_ARGUMENT;
        current.offset = pr_u68_move_word(view->bytes, next_at);
        current.segment = pr_u68_move_word(view->bytes, next_segment_at);
    }

    state->ds_6ddf = 0; /* +0x0545: always, including empty and miss */
    if (found) {
        uint8_t facing, x, y, second_selector;
        int dx, dy;

        if (!state->hooks.mark_0109)
            return PR_U68_MOVE_HOOK;
        /* +0x0550..+0x055D: first map mark uses current DS bytes. */
        state->hooks.mark_0109(state->hooks.ctx, state->party_x,
                               state->party_y, state->facing);

        /* +0x0560..+0x059B: read state again after the first mark. CBW
         * sign-extends each byte; the local callee reads only low bytes. */
        facing = state->facing;
        if (!state->direction_table ||
            state->direction_table_size < (size_t)facing + 10u)
            return PR_U68_MOVE_ARGUMENT;
        dx = pr_u68_move_signed_byte(state->direction_table[facing]);
        x = (uint8_t)(pr_u68_move_signed_byte(state->party_x) + dx);
        dy = pr_u68_move_signed_byte(state->direction_table[(size_t)facing + 9u]);
        y = (uint8_t)(pr_u68_move_signed_byte(state->party_y) + dy);
        second_selector = (uint8_t)(((unsigned)state->facing + 4u) % 8u);
        state->hooks.mark_0109(state->hooks.ctx, x, y, second_selector);
    }

    *al_out = found; /* +0x059E..+0x05AA */
    return PR_U68_MOVE_OK;
}
