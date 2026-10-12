/* Bounded model of GAME.OVR unit 0x1c +0x0A09 (ECL opcodes 0x11 PRINT and 0x12 PRINTCLEAR); see
 * docs/re/U1C_PRINT_0A09.md. */
#include "poolrad/recovered/overlay_u1c_print_0a09.h"

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

static void log_call(pr_u1c_print_report *r, pr_u1c_print_call c)
{
    if (r->calls_n < sizeof r->calls / sizeof r->calls[0])
        r->calls[r->calls_n] = c;
    r->calls_n++;
}

#define CALL(rv, callid) do { \
        int rc_ = (rv); \
        if (rc_ != 0) { r->callback_status = rc_; r->failed_call = (callid); return PR_U1C_PRINT_CALLBACK; } \
    } while (0)

pr_u1c_print_status pr_u1c_op_print(pr_rec_ecl_loader_state *st, pr_u1c_print_env *env,
                                    pr_u1c_print_report *report)
{
    pr_u1c_print_report local_report;
    pr_u1c_print_report *r = report ? report : &local_report;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status vs = PR_REC_ECL_WINDOW_OK;
    uint16_t value = 0u;
    uint8_t local[PR_U1C_PRINT_LOCAL_SIZE];
    uint8_t args[6];
    const pr_u1c_print_hooks *h;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status = PR_REC_ECL_WINDOW_OK;
    if (!st || !env)
        return PR_U1C_PRINT_ARGUMENT;
    h = &env->hooks;

    ls = pr_rec_ecl_load_operands(st, PR_U1C_PRINT_OPERANDS);          /* +0x0A10..+0x0A13: loader(1) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = loader_to_handler(ls);
        return PR_U1C_PRINT_CORE;
    }
    env->b84e4 = 0u;                                                   /* +0x0A18 */
    env->b4a00 = 1u;                                                   /* +0x0A1D */
    if (st->tag[1] < 0x80u) {                                          /* +0x0A22: cmp [0x7006], 0x80 ; jae */
        r->numeric_path = 1;
        vs = pr_rec_ecl_slot_value(st, 1u, &value);                    /* +0x0A2F..+0x0A32: slot value(1) */
        r->slot_read = 1;
        r->slot_status = vs;
        if (vs == PR_REC_ECL_WINDOW_BOUNDS || vs == PR_REC_ECL_WINDOW_UNRESOLVED || vs == PR_REC_ECL_WINDOW_ARGUMENT) {
            r->core_status = window_to_handler(vs);
            return PR_U1C_PRINT_CORE;
        }
        r->value = value;
        memset(local, 0, sizeof local);                                /* the original's local is uninitialised */
        if (!h->num_ba_1073)
            return PR_U1C_PRINT_HOOK;
        log_call(r, PR_U1C_PRINT_CALL_NUM);
        CALL(h->num_ba_1073(env->ctx, env, local, value), PR_U1C_PRINT_CALL_NUM);   /* +0x0A38 */
        if (!h->store_af8_0b25)
            return PR_U1C_PRINT_HOOK;
        log_call(r, PR_U1C_PRINT_CALL_STORE);
        CALL(h->store_af8_0b25(env->ctx, env, st, (uint8_t)PR_U1C_PRINT_STORE_MAX, local), PR_U1C_PRINT_CALL_STORE); /* +0x0A46 */
    }
    r->print_arm = st->latch == PR_U1C_PRINT_LATCH_PRINT;              /* +0x0A4B: cmp [0x6F7F], 0x11 ; jne */
    if (!r->print_arm) {
        st->b5e25 = PR_U1C_PRINT_STATE_5E25;                           /* +0x0A70 */
        st->b5e24 = PR_U1C_PRINT_STATE_5E24;                           /* +0x0A75 */
    }
    args[0] = 1u; args[1] = 0x11u; args[2] = 0x26u; args[3] = 0x16u; args[4] = 0x0Au;   /* +0x0A52..+0x0A5E / +0x0A7A..+0x0A86 */
    args[5] = r->print_arm ? 0u : 1u;                                  /* +0x0A61 / +0x0A89 */
    if (!h->print_709_0768)
        return PR_U1C_PRINT_HOOK;
    log_call(r, PR_U1C_PRINT_CALL_PRINT);
    CALL(h->print_709_0768(env->ctx, env, args, st->string_len[1], st->string_text[1]), PR_U1C_PRINT_CALL_PRINT);   /* +0x0A69 / +0x0A91 */
    env->b4a00 = 0u;                                                   /* +0x0A96 */
    if (vs == PR_REC_ECL_WINDOW_HOOK) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;                      /* the class-1 read ran the engine's load hook */
        return PR_U1C_PRINT_CORE;
    }
    return PR_U1C_PRINT_OK;
}
