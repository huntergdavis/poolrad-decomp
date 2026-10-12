/* Bounded model of GAME.OVR unit 0x1c +0x0234 (ECL opcode 0x08); see docs/re/U1C_RANDOM_0234.md. */
#include "poolrad/recovered/overlay_u1c_random_0234.h"

#include <string.h>

static void log_call(pr_u1c_random_report *r, pr_u1c_random_call c)
{
    if (r->calls_n < sizeof r->calls / sizeof r->calls[0])
        r->calls[r->calls_n] = c;
    r->calls_n++;
}

#define CALL(rv, callid) do { \
        int rc_ = (rv); \
        if (rc_ != 0) { r->callback_status = rc_; r->failed_call = (callid); return PR_U1C_RANDOM_CALLBACK; } \
    } while (0)

pr_u1c_random_status pr_u1c_op_random(pr_rec_ecl_loader_state *st, pr_u1c_random_env *env,
                                      pr_u1c_random_report *report)
{
    pr_u1c_random_report local;
    pr_u1c_random_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status vs;
    uint16_t value = 0u;
    uint8_t v, al = 0u;
    const pr_u1c_random_hooks *h;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status = PR_REC_ECL_WINDOW_OK;
    r->store_status = PR_REC_ECL_WINDOW_OK;
    if (!st || !env)
        return PR_U1C_RANDOM_ARGUMENT;
    h = &env->hooks;

    ls = pr_rec_ecl_load_operands(st, PR_U1C_RANDOM_OPERANDS);         /* +0x023A..+0x023D: loader(2) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_RANDOM_CORE;
    }
    vs = pr_rec_ecl_slot_value(st, 1u, &value);                        /* +0x0242..+0x024A: slot value(1) -> AL */
    r->slot_status = vs;
    if (vs == PR_REC_ECL_WINDOW_BOUNDS || vs == PR_REC_ECL_WINDOW_UNRESOLVED || vs == PR_REC_ECL_WINDOW_ARGUMENT) {
        r->core_status = vs == PR_REC_ECL_WINDOW_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS :
                         vs == PR_REC_ECL_WINDOW_UNRESOLVED ? PR_REC_ECL_HANDLER_UNRESOLVED : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_RANDOM_CORE;
    }
    v = (uint8_t)(value & 0xFFu);
    r->slot_byte = v;
    if (v < 0xFFu)                                                     /* +0x024D..+0x0253: cmp 0xFF; jae; inc */
        v = (uint8_t)(v + 1u);
    r->argument = v;
    r->dest = pr_rec_ecl_operand_word(st->low[2], st->high[2]);        /* +0x0256..+0x0263: 0x2B:0x4D */

    if (!h->rt_af8_11c0)
        return PR_U1C_RANDOM_HOOK;
    log_call(r, PR_U1C_RANDOM_CALL_11C0);
    CALL(h->rt_af8_11c0(env->ctx, env, v), PR_U1C_RANDOM_CALL_11C0);   /* +0x0266..+0x026C */
    if (!h->rt_af8_119a)
        return PR_U1C_RANDOM_HOOK;
    log_call(r, PR_U1C_RANDOM_CALL_119A);
    CALL(h->rt_af8_119a(env->ctx, env, 0u), PR_U1C_RANDOM_CALL_119A);  /* +0x0271..+0x0273: xor dx, dx */
    if (!h->rt_af8_119e)
        return PR_U1C_RANDOM_HOOK;
    log_call(r, PR_U1C_RANDOM_CALL_119E);
    CALL(h->rt_af8_119e(env->ctx, env, &al), PR_U1C_RANDOM_CALL_119E); /* +0x0278..+0x027D */
    r->result = al;

    r->stored = 1;
    r->store_status = pr_rec_ecl_store_word(st, r->dest, al);          /* +0x0280..+0x0289: 0x2B:0x52 (dest, AL) */
    switch (r->store_status) {
    case PR_REC_ECL_WINDOW_OK:
        break;
    case PR_REC_ECL_WINDOW_HOOK:
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_RANDOM_CORE;
    case PR_REC_ECL_WINDOW_BOUNDS:
        r->core_status = PR_REC_ECL_HANDLER_BOUNDS;
        return PR_U1C_RANDOM_CORE;
    case PR_REC_ECL_WINDOW_UNRESOLVED:
        r->core_status = PR_REC_ECL_HANDLER_UNRESOLVED;
        return PR_U1C_RANDOM_CORE;
    default:
        r->core_status = PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_RANDOM_CORE;
    }
    if (vs == PR_REC_ECL_WINDOW_HOOK) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;                      /* the class-1 slot read ran the engine's load hook */
        return PR_U1C_RANDOM_CORE;
    }
    return PR_U1C_RANDOM_OK;
}
