/* Portable C model of GAME.OVR unit 0x7d +0x0000; see
 * docs/re/U7D_ZERO_CALLEE.md. Bounded model, not strict full C: the unit
 * 0x9e entry 2 call is a host hook. */
#include "poolrad/recovered/overlay_u7d_zero.h"

#include <string.h>

/* One member's item walk (+0x00E2..+0x02BD). `i` is the member's flag index. */
static pr_u7d0_status pr_u7d0_member_pass(pr_u7d0_state *st, pr_u7d0_member *node, uint8_t i,
                                          uint8_t chunk, pr_u7d0_report *r)
{
    pr_u7d0_item *head = node->head;
    pr_u7d0_item *tail = head;       /* +0x0106..+0x0155 */
    pr_u7d0_item *cur = head;        /* [bp-0x1B] */
    pr_u7d0_item *snap = head;       /* [bp-0x13] */
    int done = 0;                    /* [bp-0xA] */
    /* (+0x0118: when head is null the original reads es:[0:5] into [bp-0x17]; that value is dead.) */
    while (tail && tail->next)
        tail = tail->next;
    for (;;) {
        pr_u7d0_item *saved_next;
        if (!cur || done)            /* +0x015B..+0x016C */
            break;
        if (++r->steps > PR_U7D0_STEP_CAP)
            return PR_U7D0_STEP_LIMIT;
        if (cur == tail)             /* +0x016F..+0x017F */
            done = 1;
        if (cur->quantity == 0) {    /* +0x0183..+0x019E */
            cur = cur->next;
        } else if (chunk < cur->quantity) {   /* +0x01A1..+0x01D8 */
            cur->quantity = (uint16_t)(cur->quantity - chunk);
            st->flags[i] = 1;
            cur = cur->next;
        } else {                     /* +0x01DA..+0x022D */
            uint8_t type = cur->type;
            saved_next = cur->next;
            r->hook_calls++;
            if (st->remove)
                st->remove(st->ctx, node, type, cur);
            cur = (snap == node->head) ? node->head : saved_next;
        }
        /* +0x0239..+0x028B: snap = node->head, then the first node that is cur or whose next is cur */
        snap = node->head;
        while (snap && snap != cur && snap->next != cur)
            snap = snap->next;
    }
    /* +0x028E..+0x02BD: re-flag when any remaining item has a nonzero quantity */
    for (; cur; cur = cur->next)
        if (cur->quantity != 0)
            st->flags[i] = 1;
    return PR_U7D0_OK;
}

pr_u7d0_status pr_u7d0_run(pr_u7d0_state *st, uint8_t n, uint8_t idx, pr_u7d0_report *report)
{
    pr_u7d0_report local;
    pr_u7d0_report *r = report ? report : &local;
    uint16_t remaining;
    int b;
    memset(r, 0, sizeof *r);
    if (!st)
        return PR_U7D0_ARGUMENT;
    if (st->mode == PR_U7D0_MODE_SCAN) {
        /* +0x0020..+0x0059: found = any flags[i] for i = 1 .. max(w, 1); a byte counter, so w >= 255 wraps */
        uint16_t w;
        unsigned i, found = 0;
        if (!st->window1 || st->window1_size < PR_U7D0_WINDOW1_MIN_SIZE)
            return PR_U7D0_ARGUMENT;
        w = (uint16_t)(st->window1[PR_U7D0_WINDOW1_SCAN_WORD] | (st->window1[PR_U7D0_WINDOW1_SCAN_WORD + 1] << 8));
        unsigned count = 0;
        i = 1;
        for (;;) {                  /* do { found |= flags[i]; i++ } while (!found && i <= w), i a byte */
            if (st->flags[i])
                found = 1;
            i = (i + 1u) & 0xFFu;
            count++;
            if (found || i > w || count > PR_U7D0_FLAG_COUNT)
                break;
        }
        if (!found) {
            if (w >= 255u)
                return PR_U7D0_NONTERMINATING;
            r->early_return = 1;
            return PR_U7D0_OK;
        }
    }
    if ((int8_t)idx > 1 && (!st->limit || st->limit_count < idx))
        return PR_U7D0_ARGUMENT;      /* the original reads DS:0x363A + 2 * (idx - 1) unchecked */

    if (st->mode != PR_U7D0_MODE_SCAN) {        /* +0x000D..+0x001E */
        memset(st->flags + PR_U7D0_FILL_FIRST, 1, PR_U7D0_FILL_COUNT);
        r->filled = 1;
    }
    /* +0x005C..+0x008C: [bp-5] = n; for b := idx downto 2: [bp-5] := limit[b-1] * [bp-5] (low word) */
    remaining = n;
    for (b = (int8_t)idx; b > 1; b--)
        remaining = (uint16_t)(st->limit[b - 1] * remaining);
    r->remaining_start = remaining;
    /* +0x008E..+0x02E9 */
    while (remaining != 0) {
        uint8_t chunk = (uint8_t)(remaining > PR_U7D0_CHUNK_MAX ? PR_U7D0_CHUNK_MAX : remaining);
        uint8_t i = 1;
        pr_u7d0_member *node;
        r->passes++;
        for (node = st->list_head; node; node = node->next, i++) {   /* +0x00AD..+0x00C2, +0x02BF..+0x02D5 */
            if (st->flags[i] == 0)
                continue;
            st->flags[i] = 0;
            if (pr_u7d0_member_pass(st, node, i, chunk, r) != PR_U7D0_OK)
                return PR_U7D0_STEP_LIMIT;
        }
        remaining = (uint16_t)(remaining > PR_U7D0_CHUNK_MAX ? remaining - PR_U7D0_CHUNK_MAX : 0);   /* +0x02D8..+0x02E6 */
    }
    return PR_U7D0_OK;
}
