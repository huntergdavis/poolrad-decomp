/* Bounded model of unit 0x1c +0x04AF..+0x0536 of ECL opcode 0x0B; see docs/re/U1C_LOAD_MONSTER_04AF.md. */
#include "poolrad/recovered/overlay_u1c_load_monster_04af.h"

#include <string.h>

static const pr_u25_e5_object *lookup(const pr_u1c_lm04af_env *env, pr_far16 p)
{
    size_t i;
    for (i = 0; i < env->object_count; i++)
        if (env->objects[i].offset == p.off && env->objects[i].segment_id == p.seg)
            return &env->objects[i];
    return NULL;
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

pr_u1c_lm04af_status pr_u1c_load_monster_04af(pr_rec_ecl_loader_state *st, const pr_u1c_lm04af_env *env,
                                              pr_far16 var_a, pr_far16 var_b, pr_far16 var_c, pr_u1c_lm04af_report *report)
{
    pr_u1c_lm04af_report local;
    pr_u1c_lm04af_report *r = report ? report : &local;
    const pr_u25_e5_object *obj;
    pr_rec_ecl_window_status ws;
    uint16_t v = 0u;
    size_t k;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status[0] = r->slot_status[1] = PR_REC_ECL_WINDOW_OK;
    if (!st || !env || (env->objects == NULL && env->object_count != 0u))
        return PR_U1C_LM04AF_ARGUMENT;

    obj = lookup(env, var_a);                                          /* +0x04AF..+0x04BE: Move(far A, local copy, 0x11D) */
    if (obj == NULL)
        return PR_U1C_LM04AF_UNMAPPED;
    if (obj->segment == NULL)
        return PR_U1C_LM04AF_VIEW;
    for (k = 0; k < PR_U1C_LM04AF_RECORD_SIZE; k++) {
        size_t at = (size_t)(uint16_t)(obj->offset + k);               /* each source byte address wraps at 16 bits */
        if (at >= obj->segment_size)
            return PR_U1C_LM04AF_VIEW;
    }
    for (k = 0; k < PR_U1C_LM04AF_RECORD_SIZE; k++)
        r->record_copy[k] = obj->segment[(uint16_t)(obj->offset + k)];
    r->moved = 1;
    r->saved_b = var_b; r->saved_c = var_c;                            /* +0x04C3..+0x04DF */

    ws = pr_rec_ecl_slot_value(st, 2u, &v);                            /* +0x04E3..+0x04EB: slot value(2) -> AL */
    r->slot_status[0] = ws;
    if (ws != PR_REC_ECL_WINDOW_OK) {                                  /* BOUNDS/UNRESOLVED/ARGUMENT and the class-1 HOOK (the count steers the loops) stop here */
        r->core_status = window_to_handler(ws);
        return PR_U1C_LM04AF_CORE;
    }
    r->count_raw = (uint8_t)(v & 0xFFu);
    r->count = r->count_raw > 0u ? r->count_raw : 1u;                  /* +0x04EE..+0x04F8: cmp [bp-2],0 ; ja ; else := 1 */

    ws = pr_rec_ecl_slot_value(st, 3u, &v);                            /* +0x04F8..+0x0500: slot value(3) -> AL */
    r->slot_status[1] = ws;
    if (ws != PR_REC_ECL_WINDOW_OK) {                                  /* a HOOK may override the byte passed to the callee: stop */
        r->core_status = window_to_handler(ws);
        return PR_U1C_LM04AF_CORE;
    }
    r->p3 = (uint8_t)(v & 0xFFu);

    r->party_head.off = env->w5d96; r->party_head.seg = env->w5d98;    /* +0x0503..+0x050D: cur := far [0x5D96] (read) */

    if (env->str_044b == NULL)
        return PR_U1C_LM04AF_STRING;
    {
        unsigned n = env->str_044b[0] <= PR_U1C_LM04AF_STR_MAX ? env->str_044b[0] : PR_U1C_LM04AF_STR_MAX;   /* 0xAF8:0x0B25 StrStore, maximum 12 */
        r->local_string[0] = (uint8_t)n;
        memcpy(r->local_string + 1, env->str_044b + 1, n);
    }
    if (!env->hooks.unit3ce_01c8)
        return PR_U1C_LM04AF_HOOK;
    r->hook_called = 1;
    k = (size_t)env->hooks.unit3ce_01c8(env->ctx, env, r->local_string, r->p3, env->b6f83);   /* +0x0524..+0x0532: 0x3E9:0x34 */
    if (k != 0) {
        r->callback_status = (int)k;
        return PR_U1C_LM04AF_CALLBACK;
    }
    return PR_U1C_LM04AF_OK;                                           /* +0x0537: the model stops here */
}
