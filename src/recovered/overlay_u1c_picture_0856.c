/* Bounded model of GAME.OVR unit 0x1c +0x0856 (ECL opcode 0x0E PICTURE); see
 * docs/re/U1C_PICTURE_0856.md. */
#include "poolrad/recovered/overlay_u1c_picture_0856.h"

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

static void log_call(pr_u1c_picture_report *r, pr_u1c_picture_call c)
{
    if (r->calls_n < sizeof r->calls / sizeof r->calls[0])
        r->calls[r->calls_n] = c;
    r->calls_n++;
}

#define CALL(rv, callid) do { \
        int rc_ = (rv); \
        if (rc_ != 0) { r->callback_status = rc_; r->failed_call = (callid); return PR_U1C_PICTURE_CALLBACK; } \
    } while (0)

pr_u1c_picture_status pr_u1c_op_picture(pr_rec_ecl_loader_state *st, pr_u1c_picture_env *env,
                                        pr_u1c_picture_report *report)
{
    pr_u1c_picture_report local;
    pr_u1c_picture_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status vs;
    uint16_t value = 0u;
    uint8_t id;
    const pr_u1c_picture_hooks *h;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status = PR_REC_ECL_WINDOW_OK;
    if (!st || !env)
        return PR_U1C_PICTURE_ARGUMENT;
    h = &env->hooks;

    ls = pr_rec_ecl_load_operands(st, PR_U1C_PICTURE_OPERANDS);        /* +0x085C..+0x085F: loader(1) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = loader_to_handler(ls);
        return PR_U1C_PICTURE_CORE;
    }
    vs = pr_rec_ecl_slot_value(st, 1u, &value);                        /* +0x0864..+0x086C: slot value(1) -> AL */
    r->slot_read = 1;
    r->slot_status = vs;
    if (vs == PR_REC_ECL_WINDOW_BOUNDS || vs == PR_REC_ECL_WINDOW_UNRESOLVED || vs == PR_REC_ECL_WINDOW_ARGUMENT) {
        r->core_status = window_to_handler(vs);
        return PR_U1C_PICTURE_CORE;
    }
    id = (uint8_t)(value & 0xFFu);
    r->id = id;

    /* +0x086F..+0x08A2: window-0 byte offset 0x1CC (word 0xE6) = 0 and id <> 0xFF: the resident box call */
    if (st->window0[PR_U1C_PICTURE_W0_WORD] == 0u && id != PR_U1C_PICTURE_NONE) {
        static const uint8_t box_args[6] = { 3, 3, 0x0D, 0x0D, 0, 0x0F };   /* +0x0881..+0x0890 */
        if (!env->str_0851 || env->str_0851[0] > PR_U1C_PICTURE_STR0_MAX)
            return PR_U1C_PICTURE_STRING;
        if (!h->box_3f1_02e8)
            return PR_U1C_PICTURE_HOOK;
        log_call(r, PR_U1C_PIC_CALL_BOX);
        r->boxed = 1;
        CALL(h->box_3f1_02e8(env->ctx, env, box_args, env->str_0851), PR_U1C_PIC_CALL_BOX);
    }

    if (id != PR_U1C_PICTURE_NONE) {                                   /* +0x08A7..+0x0911 */
        r->shown = 1;
        st->b84c9 = 1;                                                 /* +0x08AD */
        st->b84e0 = 1;                                                 /* +0x08B2 */
        if (st->window1[PR_U1C_PICTURE_W1_WORD] == PR_U1C_PICTURE_W1_MATCH) {   /* +0x08B7..+0x08C2 */
            if (!env->str_0852 || env->str_0852[0] > PR_U1C_PICTURE_STR1_MAX)
                return PR_U1C_PICTURE_STRING;
            if (!h->pic_3d0_2a)                                        /* +0x08DF: checked at its own call site */
                return PR_U1C_PICTURE_HOOK;
            log_call(r, PR_U1C_PIC_CALL_2A);
            CALL(h->pic_3d0_2a(env->ctx, env, env->str_0852, id, 0, PR_U1C_PICTURE_BUFFER_DS), PR_U1C_PIC_CALL_2A);
            if (!h->pic_3d0_25)                                        /* +0x08F7: the 2A call above has already run */
                return PR_U1C_PICTURE_HOOK;
            log_call(r, PR_U1C_PIC_CALL_25);
            CALL(h->pic_3d0_25(env->ctx, env, 3, 3, 1, env->w6ac4, env->w6ac6), PR_U1C_PIC_CALL_25);
        } else {
            if (!h->unit25_43)
                return PR_U1C_PICTURE_HOOK;
            log_call(r, PR_U1C_PIC_CALL_UNIT25_43);
            CALL(h->unit25_43(env->ctx, env, id, (uint8_t)(st->window1[PR_U1C_PICTURE_W1_WORD] & 0xFFu)), PR_U1C_PIC_CALL_UNIT25_43);
        }
    } else {                                                           /* +0x0913..+0x0950 */
        r->cleared = 1;
        if ((env->b49f9 > 1u || env->b49fa == 1u) && (st->b84e0 != 0u || env->b84e3 != 0u)) {
            if (!h->view_3ca_25)
                return PR_U1C_PICTURE_HOOK;
            log_call(r, PR_U1C_PIC_CALL_VIEW);
            CALL(h->view_3ca_25(env->ctx, env), PR_U1C_PIC_CALL_VIEW);
            r->view_refreshed = 1;
            st->b84e0 = 0;                                             /* +0x0934 */
            env->b84e3 = 0;                                            /* +0x0939 */
            env->b84e1 = 1;                                            /* +0x093E */
        }
        st->b84c8 = 0;                                                 /* +0x0943 */
        st->b84c9 = 0;                                                 /* +0x0948 */
    }
    if (vs == PR_REC_ECL_WINDOW_HOOK) {
        r->core_status = PR_REC_ECL_HANDLER_HOOK;                      /* the class-1 read ran the engine's load hook */
        return PR_U1C_PICTURE_CORE;
    }
    return PR_U1C_PICTURE_OK;
}
