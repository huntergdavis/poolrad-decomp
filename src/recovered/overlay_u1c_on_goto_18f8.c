/* Bounded model of GAME.OVR unit 0x1c +0x18F8 (ECL opcodes 0x25 ON GOTO and 0x26 ON GOSUB); see
 * docs/re/U1C_ON_GOTO_18F8.md. */
#include "poolrad/recovered/overlay_u1c_on_goto_18f8.h"

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

static int slot_fatal(pr_rec_ecl_window_status ws)
{
    return ws == PR_REC_ECL_WINDOW_BOUNDS || ws == PR_REC_ECL_WINDOW_UNRESOLVED || ws == PR_REC_ECL_WINDOW_ARGUMENT;
}

pr_u1c_on_goto_status pr_u1c_op_on_goto(pr_rec_ecl_loader_state *st, pr_u1c_on_goto_report *report)
{
    pr_u1c_on_goto_report local;
    pr_u1c_on_goto_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status v1, v2;
    uint16_t w1 = 0u, w2 = 0u;
    unsigned i;
    uint8_t sel, n;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    if (!st)
        return PR_U1C_ON_GOTO_ARGUMENT;

    ls = pr_rec_ecl_load_operands(st, 2u);                             /* +0x1902..+0x1905: loader(2) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = loader_to_handler(ls);
        return PR_U1C_ON_GOTO_CORE;
    }
    v1 = pr_rec_ecl_slot_value(st, 1u, &w1);                           /* +0x190A..+0x1912 */
    r->slot_status[0] = v1;
    if (slot_fatal(v1)) {
        r->core_status = window_to_handler(v1);
        return PR_U1C_ON_GOTO_CORE;
    }
    if (v1 == PR_REC_ECL_WINDOW_HOOK) {                                /* the class-1 load hook may override the selector: stop before
                                                                          slot value(2) is read or anything depends on the selector */
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_ON_GOTO_CORE;
    }
    v2 = pr_rec_ecl_slot_value(st, 2u, &w2);                           /* +0x1915..+0x191D */
    r->slot_status[1] = v2;
    if (slot_fatal(v2)) {
        r->core_status = window_to_handler(v2);
        return PR_U1C_ON_GOTO_CORE;
    }
    if (v2 == PR_REC_ECL_WINDOW_HOOK) {                                /* the count may be overridden: stop before the PC decrement, loader(n),
                                                                          the selection and any call-frame effect */
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_ON_GOTO_CORE;
    }
    sel = (uint8_t)(w1 & 0xFFu);
    n = (uint8_t)(w2 & 0xFFu);
    r->selector = sel;
    r->count = n;

    st->pc = (uint16_t)(st->pc - 1u);                                  /* +0x1920: dec word [0x49ED] */
    ls = pr_rec_ecl_load_operands(st, n);                              /* +0x1924..+0x1928: loader(n) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = loader_to_handler(ls);
        return PR_U1C_ON_GOTO_CORE;
    }
    for (i = 0u; i < n; ++i) {                                         /* +0x192D..+0x1987 */
        if (sel != (uint8_t)i)                                         /* +0x193F..+0x1945 */
            continue;
        r->selected = 1;
        if (st->latch == PR_U1C_ON_GOTO_LATCH_GOTO) {                  /* +0x1947: cmp [0x6F7F], 0x25 */
            r->went_goto = 1;
            st->pc = pr_rec_ecl_operand_word(st->low[i + 1u], st->high[i + 1u]);   /* +0x194E..+0x196D */
        } else {
            pr_rec_ecl_handler_status fs = pr_rec_ecl_push_call_frame(st, i + 1u); /* +0x1972..+0x1979 */
            if (fs != PR_REC_ECL_HANDLER_OK) {
                r->core_status = fs;
                return PR_U1C_ON_GOTO_CORE;
            }
        }
        break;                                                         /* +0x197E: found := 1 ends the loop */
    }
    return PR_U1C_ON_GOTO_OK;
}
