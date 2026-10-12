/* Bounded model of GAME.OVR unit 0x1c +0x1629 (ECL opcode 0x22); see docs/re/U1C_PARTY_SURPRISE_1629.md. */
#include "poolrad/recovered/overlay_u1c_party_surprise_1629.h"

#include <string.h>

static int field(const pr_u68_0236_node *n, unsigned off, uint8_t *out)
{
    size_t at = (size_t)(uint16_t)(n->offset + off);          /* es:[di + off]: 16-bit wrap */
    if (n->segment == NULL || at >= n->segment_size)
        return 0;
    *out = n->segment[at];
    return 1;
}

static pr_rec_ecl_handler_status window_to_handler(pr_rec_ecl_window_status ws)
{
    switch (ws) {
    case PR_REC_ECL_WINDOW_OK: return PR_REC_ECL_HANDLER_OK;
    case PR_REC_ECL_WINDOW_HOOK: return PR_REC_ECL_HANDLER_HOOK;
    case PR_REC_ECL_WINDOW_BOUNDS: return PR_REC_ECL_HANDLER_BOUNDS;
    case PR_REC_ECL_WINDOW_UNRESOLVED: return PR_REC_ECL_HANDLER_UNRESOLVED;
    default: return PR_REC_ECL_HANDLER_ARGUMENT;
    }
}

pr_u1c_psurp_status pr_u1c_op_party_surprise(pr_rec_ecl_loader_state *st, const pr_u68_0236_node *head,
                                             pr_u1c_psurp_report *report)
{
    pr_u1c_psurp_report local;
    pr_u1c_psurp_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    const pr_u68_0236_node *n;
    uint8_t a = 0u, b = 0u;                                    /* [bp-5], [bp-6]; [bp-7] is never read */
    unsigned visited = 0u;
    int pending_hook = 0;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->store1_status = r->store2_status = PR_REC_ECL_WINDOW_OK;
    if (!st)
        return PR_U1C_PSURP_ARGUMENT;

    ls = pr_rec_ecl_load_operands(st, PR_U1C_PSURP_OPERANDS);          /* +0x162F..+0x1632: loader(2) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_PSURP_CORE;
    }
    for (n = head; n != NULL; n = n->next) {                           /* +0x1650..+0x1687 */
        uint8_t t, link[4];
        unsigned k;
        if (visited >= PR_U1C_PSURP_LIST_CAP || !field(n, PR_U1C_PSURP_FIELD, &t)) {
            r->visited = visited; r->flag = a;
            return PR_U1C_PSURP_LIST;
        }
        for (k = 0; k < PR_U68_0236_NEXT_BYTES; k++) {                 /* the four link bytes at +0x104..+0x107 must exist */
            if (!field(n, PR_U68_0236_NEXT_OFFSET + k, &link[k])) {
                r->visited = visited; r->flag = a;
                return PR_U1C_PSURP_LIST;
            }
        }
        if (t == PR_U1C_PSURP_MATCH_A || t == PR_U1C_PSURP_MATCH_B)    /* +0x165B..+0x166A */
            a = 1u;                                                    /* +0x166C; [bp-7] := 1 too, never read */
        ++visited;
    }
    r->visited = visited;
    r->flag = a;

    r->dest1 = pr_rec_ecl_operand_word(st->low[1], st->high[1]);       /* +0x1689..+0x1696: 0x2B:0x4D, slot 1 */
    r->dest2 = pr_rec_ecl_operand_word(st->low[2], st->high[2]);       /* +0x1699..+0x16A6: 0x2B:0x4D, slot 2 */

    r->stored1 = 1;
    r->store1_status = pr_rec_ecl_store_word(st, r->dest1, a);         /* +0x16A9..+0x16B2: 0x2B:0x52 (d1, a) */
    if (r->store1_status == PR_REC_ECL_WINDOW_HOOK)
        pending_hook = 1;
    else if (r->store1_status != PR_REC_ECL_WINDOW_OK) {
        r->core_status = window_to_handler(r->store1_status);
        return PR_U1C_PSURP_CORE;
    }
    r->stored2 = 1;
    r->store2_status = pr_rec_ecl_store_word(st, r->dest2, b);         /* +0x16B7..+0x16C0: 0x2B:0x52 (d2, b = 0) */
    if (r->store2_status == PR_REC_ECL_WINDOW_HOOK)
        pending_hook = 1;
    else if (r->store2_status != PR_REC_ECL_WINDOW_OK) {
        r->core_status = window_to_handler(r->store2_status);
        return PR_U1C_PSURP_CORE;
    }
    if (pending_hook) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_PSURP_CORE;
    }
    return PR_U1C_PSURP_OK;
}
