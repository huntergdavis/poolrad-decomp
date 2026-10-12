/* Bounded model of GAME.OVR unit 0x1c +0x16C9 (ECL opcode 0x23); see docs/re/U1C_SURPRISE_16C9.md. */
#include "poolrad/recovered/overlay_u1c_surprise_16c9.h"

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

static int sext(uint8_t v) { return v < 0x80u ? (int)v : (int)v - 256; }       /* cwde on AL */

pr_u1c_surprise_status pr_u1c_op_surprise(pr_rec_ecl_loader_state *st, pr_u1c_surprise_env *env,
                                          pr_u1c_surprise_report *report)
{
    pr_u1c_surprise_report local;
    pr_u1c_surprise_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    uint8_t res = 0u;
    int pending_hook = 0, rc;
    unsigned i;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->store_status = PR_REC_ECL_WINDOW_OK;
    for (i = 0; i < 4; i++)
        r->slot_status[i] = PR_REC_ECL_WINDOW_OK;
    if (!st || !env)
        return PR_U1C_SURPRISE_ARGUMENT;

    ls = pr_rec_ecl_load_operands(st, PR_U1C_SURPRISE_OPERANDS);       /* +0x16CF..+0x16D2: loader(4) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_SURPRISE_CORE;
    }
    for (i = 0; i < 4; i++) {                                          /* +0x16DB..+0x16FD: i = 1..4 */
        uint16_t v = 0u;
        pr_rec_ecl_window_status ws = pr_rec_ecl_slot_value(st, (uint8_t)(i + 1u), &v);
        r->slot_status[i] = ws;
        if (ws == PR_REC_ECL_WINDOW_BOUNDS || ws == PR_REC_ECL_WINDOW_UNRESOLVED || ws == PR_REC_ECL_WINDOW_ARGUMENT) {
            r->core_status = window_to_handler(ws);
            return PR_U1C_SURPRISE_CORE;
        }
        if (ws == PR_REC_ECL_WINDOW_HOOK)
            pending_hook = 1;
        r->s[i] = (uint8_t)(v & 0xFFu);                                /* mov byte [bp + di - 9], dl */
    }
    r->a = (uint8_t)(r->s[3] + 2u - r->s[0]);                          /* +0x16FF..+0x170F: [bp-5] + 2 - [bp-8], AL */
    r->b = (uint8_t)(r->s[1] + 2u - r->s[2]);                          /* +0x1712..+0x1722: [bp-7] + 2 - [bp-6], AL */

    if (!env->hooks.roll_b0_0048)
        return PR_U1C_SURPRISE_HOOK;
    r->rolls = 1;
    rc = env->hooks.roll_b0_0048(env->ctx, env, PR_U1C_SURPRISE_ROLL_COUNT, PR_U1C_SURPRISE_ROLL_RANGE, &r->x);   /* +0x1725..+0x1730 */
    if (rc != 0) { r->callback_status = rc; return PR_U1C_SURPRISE_CALLBACK; }
    r->rolls = 2;
    rc = env->hooks.roll_b0_0048(env->ctx, env, PR_U1C_SURPRISE_ROLL_COUNT, PR_U1C_SURPRISE_ROLL_RANGE, &r->y);   /* +0x1733..+0x173E */
    if (rc != 0) { r->callback_status = rc; return PR_U1C_SURPRISE_CALLBACK; }

    if (!((int)r->x > sext(r->a))) {                                   /* +0x1741..+0x174E: jg skips */
        if ((int)r->y > sext(r->b))                                    /* +0x1750..+0x175D */
            res = 1u;                                                  /* +0x1765 */
        else
            res = 3u;                                                  /* +0x175F; overwritten below, as the second test cannot be skipped here */
    }
    if (!((int)r->y > sext(r->b)))                                     /* +0x1769..+0x1776: jg skips */
        res = 2u;                                                      /* +0x1778 */
    r->result = res;

    r->stored = 1;
    r->store_status = pr_rec_ecl_store_word(st, PR_U1C_SURPRISE_DEST, res);   /* +0x177C..+0x1786: 0x2B:0x52 (0x6DCB, r) */
    if (r->store_status != PR_REC_ECL_WINDOW_OK) {
        r->core_status = window_to_handler(r->store_status);
        return PR_U1C_SURPRISE_CORE;
    }
    if (pending_hook) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_SURPRISE_CORE;
    }
    return PR_U1C_SURPRISE_OK;
}
