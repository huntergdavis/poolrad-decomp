/* Bounded model of GAME.OVR unit 0x1c +0x01A3 (ECL opcodes 0x04..0x07); see docs/re/U1C_ARITH_01A3.md. */
#include "poolrad/recovered/overlay_u1c_arith_01a3.h"

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

pr_u1c_arith_status pr_u1c_op_arith(pr_rec_ecl_loader_state *st, uint16_t stack_residue, pr_u1c_arith_report *report)
{
    pr_u1c_arith_report local;
    pr_u1c_arith_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status ws[2], ss;
    uint16_t v[2] = { 0u, 0u };
    int pending_hook = 0, residue = 0;
    unsigned i;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status[0] = r->slot_status[1] = PR_REC_ECL_WINDOW_OK;
    r->store_status = PR_REC_ECL_WINDOW_OK;
    if (!st)
        return PR_U1C_ARITH_ARGUMENT;

    ls = pr_rec_ecl_load_operands(st, PR_U1C_ARITH_OPERANDS);          /* +0x01A9..+0x01AC: loader(3) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_ARITH_CORE;
    }
    for (i = 0; i < 2; i++) {                                          /* +0x01B1..+0x01D4: i = 1, 2 */
        ws[i] = pr_rec_ecl_slot_value(st, (uint8_t)(i + 1u), &v[i]);
        r->slot_status[i] = ws[i];
        if (fatal(ws[i])) {
            r->core_status = window_to_handler(ws[i]);
            r->v1 = v[0]; r->v2 = v[1];
            return PR_U1C_ARITH_CORE;
        }
        if (ws[i] == PR_REC_ECL_WINDOW_HOOK)
            pending_hook = 1;
    }
    r->v1 = v[0];
    r->v2 = v[1];
    r->dest = pr_rec_ecl_operand_word(st->low[3], st->high[3]);        /* +0x01D6..+0x01E3: 0x2B:0x4D */

    switch (st->latch) {                                               /* +0x01E6..+0x0222 */
    case PR_U1C_ARITH_ADD:
        r->result = (uint16_t)(v[0] + v[1]);
        break;
    case PR_U1C_ARITH_SUBTRACT:
        r->result = (uint16_t)(v[1] - v[0]);                           /* [bp-4] - [bp-6]: the second minus the first */
        break;
    case PR_U1C_ARITH_DIVIDE:
        if (v[1] == 0u) {                                              /* div word [bp-4] with a zero divisor: INT 0 */
            r->runtime_error = PR_U1C_ARITH_RUNTIME_ERROR_DIVIDE;
            return PR_U1C_ARITH_DIVIDE_ERROR;
        }
        r->result = (uint16_t)(v[0] / v[1]);                           /* dx:ax = 0:v1 */
        break;
    case PR_U1C_ARITH_MULTIPLY:
        r->result = (uint16_t)((uint32_t)v[0] * v[1]);                 /* mul: only AX is stored */
        break;
    default:
        r->result = stack_residue;                                     /* [bp-9] never written */
        residue = 1;
        break;
    }

    r->stored = 1;
    ss = pr_rec_ecl_store_word(st, r->dest, r->result);                /* +0x0225..+0x022B: 0x2B:0x52 (dest, r) */
    r->store_status = ss;
    if (ss != PR_REC_ECL_WINDOW_OK) {
        r->core_status = window_to_handler(ss);
        return PR_U1C_ARITH_CORE;
    }
    if (pending_hook) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;                      /* a class-1 slot read ran the engine's load hook */
        return PR_U1C_ARITH_CORE;
    }
    return residue ? PR_U1C_ARITH_RESIDUE : PR_U1C_ARITH_OK;
}
