/* Portable C model of GAME.OVR unit 0x7d entry 1 (+0x03D5) and its near
 * callee +0x02F2; see docs/re/U7D_ENTRY1.md. Bounded model, not strict full
 * C: the near call to +0x0000 is a host hook. */
#include "poolrad/recovered/overlay_u7d_entry1.h"

#include <string.h>

static uint16_t pr_u7d_load_word(const uint8_t *at)
{
    return (uint16_t)(at[0] | (at[1] << 8));
}

static void pr_u7d_store_word(uint8_t *at, uint16_t v)
{
    at[0] = (uint8_t)(v & 0xFFu);
    at[1] = (uint8_t)(v >> 8);
}

/* Host guard: every node reachable from the head can be written at +0x30. */
static int pr_u7d_list_ok(const pr_u7d_node *n)
{
    size_t count = 0;
    for (; n; n = n->next) {
        if (!n->record || n->record_size < PR_U7D_NODE_MIN_SIZE || ++count > PR_U7D_LIST_CAP)
            return 0;
    }
    return 1;
}

pr_u7d_status pr_u7d_carry_normalize(uint16_t w[PR_U7D_WORDS], const uint16_t limit[PR_U7D_WORDS],
                                     pr_u7d_node *list_head, unsigned *carries, unsigned *walks,
                                     unsigned *bumped)
{
    unsigned i;
    pr_u7d_node *n;
    if (!w || !limit || !pr_u7d_list_ok(list_head))
        return PR_U7D_ARGUMENT;
    /* +0x02F8..+0x039A: for i := 0 to 6 */
    for (i = 0; i < PR_U7D_WORDS; i++) {
        if (w[i] < limit[i])              /* +0x0318..+0x031D: cmp ax, dx; jb (unsigned) */
            continue;
        if (i != PR_U7D_WORDS - 1u) {     /* +0x031F: cmp [bp-1], 6; je +0x0363 */
            w[i + 1] = (uint16_t)(w[i + 1] + 1u);      /* +0x0325..+0x0332 */
            w[i] = (uint16_t)(w[i] - limit[i]);        /* +0x0335..+0x035E */
            if (carries) carries[i]++;
        } else {
            /* +0x0363..+0x0392: node = [0x5D96]; while node != nil: inc es:[node+0x30]; node = es:[node+0x104] */
            if (walks) (*walks)++;
            for (n = list_head; n; n = n->next) {
                pr_u7d_store_word(n->record + PR_U7D_NODE_COUNTER_OFFSET,
                                  (uint16_t)(pr_u7d_load_word(n->record + PR_U7D_NODE_COUNTER_OFFSET) + 1u));
                if (bumped) (*bumped)++;
            }
        }
    }
    return PR_U7D_OK;
}

pr_u7d_status pr_u7d_entry1(pr_u7d_state *state, uint8_t n, uint8_t idx, pr_u7d_report *report)
{
    pr_u7d_report local;
    pr_u7d_report *r = report ? report : &local;
    uint16_t w[PR_U7D_WORDS];
    unsigned i, k;
    memset(r, 0, sizeof *r);
    if (!state || !state->window0 || state->window0_size < PR_U7D_WINDOW0_MIN_SIZE ||
        idx >= PR_U7D_WORDS || !pr_u7d_list_ok(state->list_head))
        return PR_U7D_ARGUMENT;   /* the original indexes its 14-byte local array by idx unchecked */

    /* +0x03DB..+0x0407: w[i] := es:[[0x49D2] + 0x18C + 2i], i = 0..6 */
    for (i = 0; i < PR_U7D_WORDS; i++)
        w[i] = pr_u7d_load_word(state->window0 + PR_U7D_WINDOW0_OFFSET + 2u * i);
    /* +0x0409..+0x0438: for k := 1 to n do begin inc(w[idx]); +0x02F2(w) end */
    for (k = 1; k <= n; k++) {
        w[idx] = (uint16_t)(w[idx] + 1u);
        (void)pr_u7d_carry_normalize(w, state->limit, state->list_head, r->carries, &r->list_walks,
                                     &r->nodes_bumped);
        r->iterations++;
    }
    /* +0x043A..+0x0466: window 0 words := w[0..6] */
    for (i = 0; i < PR_U7D_WORDS; i++) {
        pr_u7d_store_word(state->window0 + PR_U7D_WINDOW0_OFFSET + 2u * i, w[i]);
        r->words_out[i] = w[i];
    }
    /* +0x0468..+0x0471: push ax (AH = 0x93 from the loop's last shl), n; push ax, idx; push cs; call +0x0000 */
    r->update_n_word = (uint16_t)((PR_U7D_PUSHED_HIGH_BYTE << 8) | n);
    r->update_idx_word = (uint16_t)((PR_U7D_PUSHED_HIGH_BYTE << 8) | idx);
    if (state->update)
        state->update(state->ctx, r->update_n_word, r->update_idx_word);
    return PR_U7D_OK;
}
