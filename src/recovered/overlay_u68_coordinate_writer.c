/* Portable C model of GAME.OVR unit 0x68 +0x073B (the party coordinate
 * writer); see docs/re/U68_COORDINATE_WRITER_073B.md. Every instruction
 * path is reproduced in the original's order; entries 2 and 4 of unit
 * 0x3bd are the merged translations, the other two far calls are hooks. */
#include "poolrad/recovered/overlay_u68_coordinate_writer.h"

#include <string.h>

static int pr_u68_step_signed_byte(uint8_t b)
{
    return b < 0x80u ? (int)b : (int)b - 256;   /* cbw */
}

/* cmp byte, 0; jge +2; mov byte, 0xF; cmp byte, 0xF; jle +2; mov byte, 0 (signed compares) */
static void pr_u68_step_wrap(uint8_t *v, unsigned *low, unsigned *high)
{
    if (pr_u68_step_signed_byte(*v) < 0) {
        *v = (uint8_t)PR_U68_STEP_GRID_MAX;
        *low = 1;
    }
    if (pr_u68_step_signed_byte(*v) > PR_U68_STEP_GRID_MAX) {
        *v = 0;
        *high = 1;
    }
}

pr_u68_step_status pr_u68_step_coordinate_writer(pr_u68_step_state *state, pr_u68_step_report *report)
{
    pr_u68_step_report local;
    pr_u68_step_report *r = report ? report : &local;
    const pr_u68_step_hooks *h;
    pr_u3bd_nibble_state nib;
    pr_u3bd_cell_state cell;
    pr_u3bd_nibble_status nib_status;
    pr_u3bd_cell_status cell_status;
    uint8_t al = 0;
    unsigned i;
    memset(r, 0, sizeof *r);
    if (!state) {
        r->stop_offset = 0x073Bu;
        return PR_U68_STEP_ARGUMENT;
    }
    h = state->hooks;

    /* +0x073E..+0x0742: push word [0x2628]; lcall 0x8AE:0x00DC */
    r->calls[r->call_count++] = PR_U68_STEP_CALL_RESIDENT;
    r->stop_offset = 0x0742u;
    if (!h || !h->resident_8ae_00dc ||
        h->resident_8ae_00dc(h->ctx, state->word_2628) != 0)
        return PR_U68_STEP_HOOK;

    /* +0x0747..+0x0752: al = [di + 0x2736], di = zx [0x6AAF]; add byte [0x6AAD], al */
    r->x_in = state->party_x;
    r->y_in = state->party_y;
    r->stop_offset = 0x074Eu;
    if (!state->direction_table ||
        state->direction_table_size <= (size_t)state->facing)
        return PR_U68_STEP_ARGUMENT;
    r->dx = pr_u68_step_signed_byte(state->direction_table[state->facing]);
    state->party_x = (uint8_t)(state->party_x + state->direction_table[state->facing]);
    r->x_sum = state->party_x;
    /* +0x0756..+0x0761: al = [di + 0x273F]; add byte [0x6AAE], al */
    r->stop_offset = 0x075Du;
    if (state->direction_table_size <= (size_t)state->facing + PR_U68_STEP_DY_SKEW)
        return PR_U68_STEP_ARGUMENT;
    r->dy = pr_u68_step_signed_byte(state->direction_table[state->facing + PR_U68_STEP_DY_SKEW]);
    state->party_y = (uint8_t)(state->party_y + state->direction_table[state->facing + PR_U68_STEP_DY_SKEW]);
    r->y_sum = state->party_y;

    /* +0x0765..+0x0778: x < 0 -> 15, x > 15 -> 0; +0x077D..+0x0790: the same on y */
    pr_u68_step_wrap(&state->party_x, &r->wrapped_x_low, &r->wrapped_x_high);
    pr_u68_step_wrap(&state->party_y, &r->wrapped_y_low, &r->wrapped_y_high);
    r->x_out = state->party_x;
    r->y_out = state->party_y;

    /* +0x0795..+0x07A6: push x, y, facing bytes; lcall 0x3D5:0x2A; [0x6AB0] = al */
    nib.map_id = state->map_id;
    nib.buffer = state->buffer;
    nib.frame_residue = state->entry2_frame_residue;
    r->calls[r->call_count++] = PR_U68_STEP_CALL_ENTRY2;
    r->stop_offset = 0x07A1u;
    nib_status = pr_u3bd_entry2(&nib, state->party_x, state->party_y,
                                state->facing, &al, &r->entry2);
    if (nib_status == PR_U3BD_NIBBLE_ARGUMENT)
        return PR_U68_STEP_LOOKUP;
    state->lookup_b0 = al;

    /* +0x07A9..+0x07B5: mov di, 0x6DDE; push ds; push di; push 3; push 1; lcall 0xAF8:0x16A8 */
    r->calls[r->call_count++] = PR_U68_STEP_CALL_FILL;
    for (i = 0; i < PR_U68_STEP_FILL_COUNT; i++)
        state->ds_6dde[i] = (uint8_t)PR_U68_STEP_FILL_VALUE;

    /* +0x07BA..+0x07C7: push x, y bytes; lcall 0x3D5:0x34; [0x6AB1] = al */
    cell.map_id = state->map_id;
    cell.buffer = state->buffer;
    r->calls[r->call_count++] = PR_U68_STEP_CALL_ENTRY4;
    r->stop_offset = 0x07C2u;
    cell_status = pr_u3bd_entry4(&cell, state->party_x, state->party_y,
                                 &al, &r->entry4);
    if (cell_status != PR_U3BD_CELL_OK)
        return PR_U68_STEP_LOOKUP;
    state->lookup_b1 = al;

    /* +0x07CA..+0x07D8: les di,[0x49D6]; ax = es:[di + 0x594] & 1; or ax, ax; jbe */
    r->stop_offset = 0x07CEu;
    if (!state->window1 || state->window1_size < PR_U68_STEP_WINDOW1_MIN_SIZE)
        return PR_U68_STEP_ARGUMENT;
    r->window_bit = (unsigned)(state->window1[PR_U68_STEP_WINDOW1_BIT_OFFSET] & 1u);
    /* +0x07DA..+0x07E0 (bit set): push 1; push 2; +0x07E7..+0x07ED (clear): push 1; push 1; lcall 0x8C:0x25 */
    r->u7d_first = 1;
    r->u7d_second = r->window_bit ? 2u : 1u;
    r->calls[r->call_count++] = PR_U68_STEP_CALL_U7D_ENTRY1;
    r->stop_offset = r->window_bit ? 0x07E0u : 0x07EDu;
    h = state->hooks;
    if (!h || !h->u7d_entry1 ||
        h->u7d_entry1(h->ctx, r->u7d_first, r->u7d_second) != 0)
        return PR_U68_STEP_HOOK;
    /* +0x07F2..+0x07F3: pop bp; retf (the routine has no frame locals; no `mov sp, bp`) */
    r->stop_offset = 0x07F4u;
    return PR_U68_STEP_OK;
}
