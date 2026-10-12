/* Bounded model of the entry prefix of GAME.OVR unit 0x1c +0x0450 (ECL opcode 0x0B); see docs/re/U1C_LOAD_MONSTER_PREFIX_0450.md. */
#include "poolrad/recovered/overlay_u1c_load_monster_prefix_0450.h"

#include <string.h>

pr_u1c_lmon_status pr_u1c_op_load_monster_prefix(pr_rec_ecl_loader_state *st, pr_u1c_lmon_env *env,
                                                 pr_u1c_lmon_report *report)
{
    pr_u1c_lmon_report local;
    pr_u1c_lmon_report *r = report ? report : &local;
    pr_rec_ecl_loader_status ls;
    pr_rec_ecl_window_status vs;
    uint16_t v = 0u;
    memset(r, 0, sizeof *r);
    r->core_status = PR_REC_ECL_HANDLER_OK;
    r->slot_status = PR_REC_ECL_WINDOW_OK;
    if (!st || !env || !env->wrapper)
        return PR_U1C_LMON_ARGUMENT;
    r->var_a = env->stack_a; r->var_b = env->stack_b; r->var_c = env->stack_c;
    r->saved_w5d92 = env->w5d92; r->saved_w5d94 = env->w5d94;          /* +0x0457..+0x0461: saved := far [0x5D92] */

    ls = pr_rec_ecl_load_operands(st, PR_U1C_LMON_OPERANDS);           /* +0x0468..+0x046B: loader(3) */
    if (ls != PR_REC_ECL_LOADER_OK) {
        r->core_status = ls == PR_REC_ECL_LOADER_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_LMON_CORE;
    }
    if (env->b4435 >= PR_U1C_LMON_CAP)                                 /* +0x0470..+0x0477: cmp [0x4435], 0x3F ; jb ; else jmp +0x0813 */
        return PR_U1C_LMON_CAP_RETURN;

    vs = pr_rec_ecl_slot_value(st, 1u, &v);                            /* +0x048A..+0x0492: slot value(1) -> AL */
    r->slot_status = vs;
    if (vs == PR_REC_ECL_WINDOW_BOUNDS || vs == PR_REC_ECL_WINDOW_UNRESOLVED || vs == PR_REC_ECL_WINDOW_ARGUMENT) {
        r->core_status = vs == PR_REC_ECL_WINDOW_BOUNDS ? PR_REC_ECL_HANDLER_BOUNDS :
                         vs == PR_REC_ECL_WINDOW_UNRESOLVED ? PR_REC_ECL_HANDLER_UNRESOLVED : PR_REC_ECL_HANDLER_ARGUMENT;
        return PR_U1C_LMON_CORE;
    }
    if (vs == PR_REC_ECL_WINDOW_HOOK) {                                /* the class-1 hook may override the id: stop before the wrapper */
        r->core_status = PR_REC_ECL_HANDLER_HOOK;
        return PR_U1C_LMON_CORE;
    }
    r->id = (uint8_t)(v & 0xFFu);

    r->wrapper_called = 1;
    r->wrapper_status = pr_u25_entry5_0411(env->wrapper, r->id, &r->var_a, &r->var_b, &r->var_c, &r->wrapper_report);   /* +0x0495..+0x04AA */
    if (r->wrapper_status != PR_U25_E5_OK)
        return PR_U1C_LMON_WRAPPER;
    return PR_U1C_LMON_PREFIX_DONE;                                    /* +0x04AF: the model stops here */
}
