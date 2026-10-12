/* Portable C model of GAME.OVR unit 0x68 +0x068D (the Up preflight); see
 * docs/re/U68_UP_PREFLIGHT_068D.md. Every instruction path is reproduced in
 * the original's order; the far call goes through the merged unit 0x3bd
 * entry 3 translation. */
#include "poolrad/recovered/overlay_u68_up_preflight.h"

#include <string.h>

static int pr_u68_signed_byte(uint8_t b)
{
    return b < 0x80u ? (int)b : (int)b - 256;   /* cbw */
}

static void pr_u68_store_word(uint8_t *at, uint16_t v)
{
    at[0] = (uint8_t)(v & 0xFFu);
    at[1] = (uint8_t)(v >> 8);
}

pr_u68_up_status pr_u68_up_preflight(pr_u68_up_state *state, pr_u68_up_report *report)
{
    pr_u68_up_report local;
    pr_u68_up_report *r = report ? report : &local;
    uint8_t al = 0;
    int x, y;
    memset(r, 0, sizeof *r);
    if (!state || !state->window1 || state->window1_size < PR_U68_UP_WINDOW1_MIN_SIZE)
        return PR_U68_UP_ARGUMENT;   /* host guard: the original writes es:[di+0x5AA] unchecked */

    /* +0x0693..+0x06A6: [bp-2] = cbw [0x6AAD]; [bp-4] = cbw [0x6AAE]; [bp-6] = zx [0x6AAF] */
    x = pr_u68_signed_byte(state->party_x);
    y = pr_u68_signed_byte(state->party_y);
    r->x_in = x;
    r->y_in = y;
    r->facing = state->facing;
    /* +0x06A9..+0x06AF: les di,[0x49D6]; es:[di+0x5AA] = 0 */
    pr_u68_store_word(state->window1 + PR_U68_UP_FLAG_OFFSET, 0);
    /* +0x06B4..+0x06C0: push x byte, y byte, f byte; lcall 0x3D5:0x2F (unit 0x3bd entry 3) */
    if (pr_u3bd_entry3(&state->field, (uint8_t)state->party_x, (uint8_t)state->party_y,
                       state->facing, &al, &r->field) != PR_U3BD_FIELD_OK)
        return PR_U68_UP_ARGUMENT;
    r->lookup = al;
    /* +0x06C5..+0x06C7: or al,al; je +0x0737 */
    if (al == 0) {
        r->skipped = 1;
        return PR_U68_UP_OK;
    }
    /* +0x06C9..+0x06DC: di = [bp-6]; [bp-2] += cbw [di+0x2736]; [bp-4] += cbw [di+0x273F] */
    if (!state->direction_table ||
        state->direction_table_size < (size_t)state->facing + PR_U68_UP_DY_SKEW + 1u)
        return PR_U68_UP_ARGUMENT;
    r->dx = pr_u68_signed_byte(state->direction_table[state->facing]);
    r->dy = pr_u68_signed_byte(state->direction_table[state->facing + PR_U68_UP_DY_SKEW]);
    x += r->dx;
    y += r->dy;
    r->x_candidate = x;
    r->y_candidate = y;
    /* +0x06DF..+0x0704: cmp [bp-2],15; jle; [0x6AAD] = 15; flag = 1. cmp [bp-2],0; jge; [0x6AAD] = 0; flag = 1 */
    if (x > PR_U68_UP_GRID_MAX) {
        state->party_x = (uint8_t)PR_U68_UP_GRID_MAX;
        pr_u68_store_word(state->window1 + PR_U68_UP_FLAG_OFFSET, 1);
        r->clamped_x_high = 1;
    }
    if (x < 0) {
        state->party_x = 0;
        pr_u68_store_word(state->window1 + PR_U68_UP_FLAG_OFFSET, 1);
        r->clamped_x_low = 1;
    }
    /* +0x070B..+0x0730: the same two compares on [bp-4] writing [0x6AAE] */
    if (y > PR_U68_UP_GRID_MAX) {
        state->party_y = (uint8_t)PR_U68_UP_GRID_MAX;
        pr_u68_store_word(state->window1 + PR_U68_UP_FLAG_OFFSET, 1);
        r->clamped_y_high = 1;
    }
    if (y < 0) {
        state->party_y = 0;
        pr_u68_store_word(state->window1 + PR_U68_UP_FLAG_OFFSET, 1);
        r->clamped_y_low = 1;
    }
    /* +0x0737..+0x073A: mov sp,bp; pop bp; retf */
    r->flag_set = (state->window1[PR_U68_UP_FLAG_OFFSET] == 1u && state->window1[PR_U68_UP_FLAG_OFFSET + 1] == 0u);
    return PR_U68_UP_OK;
}
