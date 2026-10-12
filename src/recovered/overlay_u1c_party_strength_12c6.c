/* Bounded model of GAME.OVR unit 0x1c +0x12C6 (ECL opcode 0x1D); see docs/re/U1C_PARTY_STRENGTH_12C6.md. */
#include "poolrad/recovered/overlay_u1c_party_strength_12c6.h"

#include <string.h>

static int field(const pr_u68_0236_node *n, unsigned off, uint8_t *out)
{
    size_t at = (size_t)(uint16_t)(n->offset + off);          /* es:[di + off]: 16-bit wrap */
    if (n->segment == NULL || at >= n->segment_size)
        return 0;
    *out = n->segment[at];
    return 1;
}

pr_u1c_pstr_status pr_u1c_op_party_strength(pr_rec_ecl_loader_state *st, const pr_u68_0236_node *head,
                                            pr_u1c_pstr_report *report)
{
    pr_u1c_pstr_report local;
    pr_u1c_pstr_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    const pr_u68_0236_node *n;
    uint8_t sum = 0u;
    unsigned visited = 0u;
    uint16_t dest;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->store_status = PR_REC_ECL_WINDOW_OK;
    if (!st)
        return PR_U1C_PSTR_ARGUMENT;

    ls = pr_rec_ecl_load_operands(st, 1u);                            /* +0x12CC..+0x12CF: loader(1) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_PSTR_CORE;
    }
    for (n = head; n != NULL; n = n->next) {                          /* +0x12E5..+0x12ED, +0x139F..+0x13B2 */
        uint8_t a, b, c, d, e, link[4];
        uint16_t w;
        unsigned k;
        if (visited >= PR_U1C_PSTR_LIST_CAP) {
            r->visited = visited; r->sum = sum;
            return PR_U1C_PSTR_LIST;
        }
        if (!field(n, PR_U1C_PSTR_A, &a) || !field(n, PR_U1C_PSTR_B, &b) || !field(n, PR_U1C_PSTR_C, &c) ||
            !field(n, PR_U1C_PSTR_D, &d) || !field(n, PR_U1C_PSTR_E, &e)) {
            r->visited = visited; r->sum = sum;
            return PR_U1C_PSTR_LIST;
        }
        for (k = 0; k < PR_U68_0236_NEXT_BYTES; k++) {                /* the four link bytes at +0x104..+0x107 must exist */
            if (!field(n, PR_U68_0236_NEXT_OFFSET + k, &link[k])) {
                r->visited = visited; r->sum = sum;
                return PR_U1C_PSTR_LIST;
            }
        }
        b = b > PR_U1C_PSTR_B_BIAS ? (uint8_t)(b - PR_U1C_PSTR_B_BIAS) : 0u;       /* +0x1327..+0x133A */
        c = c > PR_U1C_PSTR_C_BIAS ? (uint8_t)(c - PR_U1C_PSTR_C_BIAS) : 0u;       /* +0x133E..+0x1351 */
        w = (uint16_t)((uint16_t)d << 3);                              /* +0x1355..+0x135C: shl ax, 3 */
        w = (uint16_t)(w + (uint16_t)(5u * c));                        /* +0x1360..+0x136B */
        w = (uint16_t)(w + (uint16_t)(5u * b));                        /* +0x136F..+0x137C */
        w = (uint16_t)(w + a);                                         /* +0x137E..+0x1383 */
        w = (uint16_t)(w + (uint16_t)((uint16_t)e << 2));              /* +0x138B..+0x1394 */
        sum = (uint8_t)(sum + (uint8_t)(w / PR_U1C_PSTR_DIVISOR));     /* +0x1396..+0x139C: cwd; div 10 (dx = 0); add byte */
        ++visited;
    }
    r->visited = visited;
    r->sum = sum;

    dest = pr_rec_ecl_operand_word(st->low[1], st->high[1]);          /* +0x13B5..+0x13C2: 0x2B:0x4D */
    r->dest = dest;
    r->stored = 1;
    r->store_status = pr_rec_ecl_store_word(st, dest, sum);            /* +0x13C5..+0x13CE: 0x2B:0x52 (dest, sum) */
    switch (r->store_status) {
    case PR_REC_ECL_WINDOW_OK:
        return PR_U1C_PSTR_OK;
    case PR_REC_ECL_WINDOW_HOOK:
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        break;
    case PR_REC_ECL_WINDOW_BOUNDS:
        r->core_status = PR_REC_ECL_HANDLER_BOUNDS;
        break;
    case PR_REC_ECL_WINDOW_UNRESOLVED:
        r->core_status = PR_REC_ECL_HANDLER_UNRESOLVED;
        break;
    default:
        r->core_status = PR_REC_ECL_HANDLER_ARGUMENT;
        break;
    }
    return PR_U1C_PSTR_CORE;
}
