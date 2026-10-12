/* Path-complete model of GAME.OVR unit 0x1c +0x0DEF (opcodes 0x2F AND and 0x30
 * OR), composed from lane A's merged unit 0x25 cores; see
 * docs/re/U1C_AND_OR_0DEF.md. */
#include "poolrad/recovered/overlay_u1c_and_or_0def.h"

#include <string.h>

static pr_rec_ecl_handler_status loader_to_handler(pr_rec_ecl_loader_status st)
{
    switch (st) {
    case PR_REC_ECL_LOADER_OK: return PR_REC_ECL_HANDLER_OK;
    case PR_REC_ECL_LOADER_BOUNDS: return PR_REC_ECL_HANDLER_BOUNDS;
    default: return PR_REC_ECL_HANDLER_ARGUMENT;
    }
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

static int slot_value_failed(pr_rec_ecl_window_status vs)
{
    return vs == PR_REC_ECL_WINDOW_BOUNDS || vs == PR_REC_ECL_WINDOW_UNRESOLVED || vs == PR_REC_ECL_WINDOW_ARGUMENT;
}

pr_rec_ecl_handler_status pr_u1c_op_and_or(pr_rec_ecl_loader_state *state, pr_rec_ecl_window_status *store_status,
                                           pr_u1c_and_or_report *report)
{
    pr_u1c_and_or_report local;
    pr_u1c_and_or_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status ws, v1s, v2s;
    pr_rec_ecl_handler_status cs;
    uint16_t v1 = 0u, v2 = 0u;
    memset(r, 0, sizeof *r);
    r->slot1_status = r->slot2_status = PR_REC_ECL_WINDOW_OK;
    if (store_status)
        *store_status = PR_REC_ECL_WINDOW_OK;
    if (!state)
        return PR_REC_ECL_HANDLER_ARGUMENT;

    ls = pr_rec_ecl_load_operands(state, PR_U1C_AND_OR_OPERANDS);     /* +0x0DF5..+0x0DF8: loader(3) */
    if (ls != PR_REC_ECL_LOADER_OK)
        return loader_to_handler(ls);
    v1s = pr_rec_ecl_slot_value(state, 1u, &v1);                      /* +0x0DFD..+0x0E05 */
    r->slot1_read = 1;
    r->slot1_status = v1s;
    if (slot_value_failed(v1s))
        return window_to_handler(v1s);                                /* the engine would use a value this model lacks */
    v2s = pr_rec_ecl_slot_value(state, 2u, &v2);                      /* +0x0E08..+0x0E10 */
    r->slot2_read = 1;
    r->slot2_status = v2s;
    if (slot_value_failed(v2s))
        return window_to_handler(v2s);
    r->v1 = v1;
    r->v2 = v2;
    r->dest = pr_rec_ecl_operand_word(state->low[3], state->high[3]); /* +0x0E13..+0x0E20: [0x7088], [0x7048] -> 0x2B:0x4D */
    /* +0x0E23..+0x0E3B: only the low byte is kept; any latch but 0x2F is OR */
    r->result = (uint8_t)((state->latch == PR_U1C_AND_OR_LATCH_AND) ? (v1 & v2) : (v1 | v2));
    cs = pr_rec_ecl_compare_words(state, 0u, (uint16_t)r->result);    /* +0x0E3E..+0x0E47: push 0, push result; 0x2B:0x70 */
    r->compared = 1;
    if (cs != PR_REC_ECL_HANDLER_OK)
        return cs;
    ws = pr_rec_ecl_store_word(state, r->dest, (uint16_t)r->result);  /* +0x0E4C..+0x0E55: push dest, push result; 0x2B:0x52 */
    r->stored = 1;
    if (store_status)
        *store_status = ws;
    if (ws == PR_REC_ECL_WINDOW_OK && (v1s == PR_REC_ECL_WINDOW_HOOK || v2s == PR_REC_ECL_WINDOW_HOOK))
        return PR_REC_ECL_HANDLER_HOOK;                               /* a value read ran the load hook */
    return window_to_handler(ws);                                     /* +0x0E5A..+0x0E5D */
}
