/* Portable C model of GAME.OVR unit 0x9e entry 2 (+0x0026) and entry 1
 * (+0x0000); see docs/re/U9E_ENTRY2.md. Bounded model: the DS handler table
 * and the runtime FreeMem are host hooks. */
#include "poolrad/recovered/overlay_u9e_entry2.h"

#include <string.h>

void pr_u9e_dispatch(const pr_u9e_state *state, uint8_t type, pr_u7d0_member *member, pr_u7d0_item *item,
                     uint8_t mode)
{
    /* +0x0003..+0x001E: push member, item, mode; di = type * 4; lcall [di + 0x6828] */
    if (state && state->handlers[type])
        state->handlers[type](state->ctx, member, item, mode);
}

pr_u9e_status pr_u9e_entry2(pr_u9e_state *state, pr_u7d0_member *member, uint8_t type, pr_u7d0_item *item,
                            pr_u9e_report *report)
{
    pr_u9e_report local;
    pr_u9e_report *r = report ? report : &local;
    pr_u7d0_item *p = item;           /* [bp-8] */
    memset(r, 0, sizeof *r);
    if (!state || !member)
        return PR_U9E_ARGUMENT;

    /* +0x002C..+0x0078: item null -> first record whose byte +0 equals the type byte */
    if (!p) {
        r->searched = 1;
        for (p = member->head; p && p->type != type; p = p->next)
            ;
    }
    /* +0x0078..+0x0080: nothing selected -> epilogue */
    if (!p)
        return PR_U9E_OK;
    r->found = 1;

    /* +0x0083..+0x00A1: flag4 != 0 -> near call +0x0000 (type, member, item, 1) */
    if (p->flag4 != 0) {
        r->handler_called = 1;
        if (!state->handlers[type])
            r->handler_was_null = 1;
        pr_u9e_dispatch(state, type, member, p, PR_U9E_REMOVE_MODE);
    }

    /* +0x00A4..+0x00D1: head == item -> member +0x7F := item +5 */
    if (member->head == p) {
        member->head = p->next;
        r->was_head = 1;
    } else {
        /* +0x00D3..+0x0127: q = head; while q != nil and q.next != item do q := q.next; q.next := item.next */
        pr_u7d0_item *q = member->head;
        while (q && q->next != p)
            q = q->next;
        if (!q)
            return PR_U9E_UNLINK_FAILED;
        q->next = p->next;
    }

    /* +0x012B..+0x0135: FreeMem(item, 9) */
    r->released = 1;
    if (state->release)
        state->release(state->ctx, p, (uint16_t)PR_U9E_ITEM_SIZE);
    return PR_U9E_OK;
}

void pr_u9e_as_u7d0_remove_hook(void *ctx, pr_u7d0_member *member, uint8_t type, pr_u7d0_item *item)
{
    (void)pr_u9e_entry2((pr_u9e_state *)ctx, member, type, item, NULL);
}
