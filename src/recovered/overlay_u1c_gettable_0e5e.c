/* Bounded model of GAME.OVR unit 0x1c +0x0E5E (ECL opcode 0x2A); see docs/re/U1C_GETTABLE_0E5E.md. */
#include "poolrad/recovered/overlay_u1c_gettable_0e5e.h"

#include <string.h>

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

static int fatal(pr_rec_ecl_window_status ws)
{
    return ws == PR_REC_ECL_WINDOW_BOUNDS || ws == PR_REC_ECL_WINDOW_UNRESOLVED || ws == PR_REC_ECL_WINDOW_ARGUMENT;
}

pr_u1c_gettable_status pr_u1c_op_gettable(pr_rec_ecl_loader_state *st, pr_u1c_gettable_report *report)
{
    pr_u1c_gettable_report local;
    pr_u1c_gettable_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status vs, ws, ss;
    uint16_t sv = 0u, value = 0u;
    int pending_hook = 0;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status = r->load_status = r->store_status = PR_REC_ECL_WINDOW_OK;
    if (!st)
        return PR_U1C_GETTABLE_ARGUMENT;

    ls = pr_rec_ecl_load_operands(st, PR_U1C_GETTABLE_OPERANDS);       /* +0x0E64..+0x0E67: loader(3) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_GETTABLE_CORE;
    }
    r->base = pr_rec_ecl_operand_word(st->low[1], st->high[1]);        /* +0x0E6C..+0x0E79: 0x2B:0x4D, slot 1 */
    vs = pr_rec_ecl_slot_value(st, 2u, &sv);                           /* +0x0E7C..+0x0E84: slot value(2) -> AL */
    r->slot_status = vs;
    if (fatal(vs)) {
        r->core_status = window_to_handler(vs);
        return PR_U1C_GETTABLE_CORE;
    }
    pending_hook = vs == PR_REC_ECL_WINDOW_HOOK;
    r->index = (uint8_t)(sv & 0xFFu);
    r->dest = pr_rec_ecl_operand_word(st->low[3], st->high[3]);        /* +0x0E87..+0x0E94: 0x2B:0x4D, slot 3 */
    r->addr = (uint16_t)(r->base + r->index);                          /* +0x0E97..+0x0E9F */

    r->loaded = 1;
    ws = pr_rec_ecl_load_word(st, r->addr, &value);                    /* +0x0EA2..+0x0EAA: 0x2B:0x57 */
    r->load_status = ws;
    if (fatal(ws)) {
        r->core_status = window_to_handler(ws);                        /* the engine would store an uninitialized local */
        return PR_U1C_GETTABLE_CORE;
    }
    if (ws == PR_REC_ECL_WINDOW_HOOK)
        pending_hook = 1;
    r->value = value;

    r->stored = 1;
    ss = pr_rec_ecl_store_word(st, r->dest, value);                    /* +0x0EAD..+0x0EB3: 0x2B:0x52 (dest, value) */
    r->store_status = ss;
    if (ss != PR_REC_ECL_WINDOW_OK) {
        r->core_status = window_to_handler(ss);
        return PR_U1C_GETTABLE_CORE;
    }
    if (pending_hook) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_GETTABLE_CORE;
    }
    return PR_U1C_GETTABLE_OK;
}
