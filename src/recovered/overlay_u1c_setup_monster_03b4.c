/* Bounded model of GAME.OVR unit 0x1c +0x03B4 (ECL opcode 0x0C); see docs/re/U1C_SETUP_MONSTER_03B4.md. */
#include "poolrad/recovered/overlay_u1c_setup_monster_03b4.h"

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

static void log_call(pr_u1c_smon_report *r, pr_u1c_smon_call c)
{
    if (r->calls_n < 2)
        r->calls[r->calls_n] = c;
    r->calls_n++;
}

pr_u1c_smon_status pr_u1c_op_setup_monster(pr_rec_ecl_loader_state *st, pr_u1c_setup_monster_env *env,
                                           pr_u1c_smon_report *report)
{
    pr_u1c_smon_report local;
    pr_u1c_smon_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status ws;
    uint16_t v = 0u;
    uint8_t cell = 0u;
    int pending_hook = 0, rc;
    unsigned i;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    for (i = 0; i < 3; i++)
        r->slot_status[i] = PR_REC_ECL_WINDOW_OK;
    if (!st || !env)
        return PR_U1C_SMON_ARGUMENT;

    ls = pr_rec_ecl_load_operands(st, PR_U1C_SETUP_MONSTER_OPERANDS);  /* +0x03BA..+0x03BD: loader(3) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_SMON_CORE;
    }
    ws = pr_rec_ecl_slot_value(st, 1u, &v);                            /* +0x03C2..+0x03C5 */
    r->slot_status[0] = ws;
    if (fatal(ws)) { r->core_status = window_to_handler(ws); return PR_U1C_SMON_CORE; }
    pending_hook |= ws == PR_REC_ECL_WINDOW_HOOK;
    env->b6f81 = (uint8_t)(v & 0xFFu);                                 /* +0x03CA */
    r->s1 = env->b6f81;

    ws = pr_rec_ecl_slot_value(st, 2u, &v);                            /* +0x03CD..+0x03D0 */
    r->slot_status[1] = ws;
    if (fatal(ws)) { r->core_status = window_to_handler(ws); return PR_U1C_SMON_CORE; }
    pending_hook |= ws == PR_REC_ECL_WINDOW_HOOK;
    st->window1[PR_U1C_SETUP_MONSTER_W580] = v;                        /* +0x03D5..+0x03D9: es:[di + 0x580] := AX, direct */
    r->s2 = v;

    ws = pr_rec_ecl_slot_value(st, 3u, &v);                            /* +0x03DE..+0x03E1 */
    r->slot_status[2] = ws;
    if (fatal(ws)) { r->core_status = window_to_handler(ws); return PR_U1C_SMON_CORE; }
    pending_hook |= ws == PR_REC_ECL_WINDOW_HOOK;
    env->b6f82 = (uint8_t)(v & 0xFFu);                                 /* +0x03E6 */
    r->s3 = env->b6f82;

    if (!env->hooks.unit25_0482) {
        r->w580 = st->window1[PR_U1C_SETUP_MONSTER_W580]; r->w582 = st->window1[PR_U1C_SETUP_MONSTER_W582];
        return PR_U1C_SMON_HOOK;
    }
    log_call(r, PR_U1C_SMON_CALL_0482);
    rc = env->hooks.unit25_0482(env->ctx, env, st->b6aad, st->b6aae, st->b6aaf, &cell);   /* +0x03E9..+0x03F5: 0x2B:0x3E */
    r->w580 = st->window1[PR_U1C_SETUP_MONSTER_W580]; r->w582 = st->window1[PR_U1C_SETUP_MONSTER_W582];
    if (rc != 0) { r->callback_status = rc; r->failed_call = PR_U1C_SMON_CALL_0482; return PR_U1C_SMON_CALLBACK; }
    r->cell = cell;
    st->window1[PR_U1C_SETUP_MONSTER_W582] = (uint16_t)cell;           /* +0x03FA..+0x0400: xor ah,ah; es:[di + 0x582] := AX */
    if (st->window1[PR_U1C_SETUP_MONSTER_W580] < st->window1[PR_U1C_SETUP_MONSTER_W582]) {   /* +0x0409..+0x0417: jae skips */
        st->window1[PR_U1C_SETUP_MONSTER_W582] = st->window1[PR_U1C_SETUP_MONSTER_W580];     /* +0x041D..+0x0426 */
        r->lowered = 1;
    }
    r->w580 = st->window1[PR_U1C_SETUP_MONSTER_W580];
    r->w582 = st->window1[PR_U1C_SETUP_MONSTER_W582];

    if (!env->hooks.unit25_054e)
        return PR_U1C_SMON_HOOK;
    log_call(r, PR_U1C_SMON_CALL_054E);
    rc = env->hooks.unit25_054e(env->ctx, env, st, env->b6f81, env->b6f82,
                                (uint8_t)(st->window1[PR_U1C_SETUP_MONSTER_W582] & 0xFFu));   /* +0x042B..+0x0442: 0x2B:0x48 */
    if (rc != 0) { r->callback_status = rc; r->failed_call = PR_U1C_SMON_CALL_054E; return PR_U1C_SMON_CALLBACK; }
    if (pending_hook) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;                      /* a class-1 slot read ran the engine's load hook model */
        return PR_U1C_SMON_CORE;
    }
    return PR_U1C_SMON_OK;
}
