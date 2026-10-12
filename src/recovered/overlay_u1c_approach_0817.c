/* Bounded model of GAME.OVR unit 0x1c +0x0817 (ECL opcode 0x0D); see docs/re/U1C_APPROACH_0817.md. */
#include "poolrad/recovered/overlay_u1c_approach_0817.h"

#include <string.h>

pr_u1c_approach_status pr_u1c_op_approach(pr_rec_ecl_loader_state *st, pr_u1c_setup_monster_env *env,
                                          pr_u1c_approach_report *report)
{
    pr_u1c_approach_report local;
    pr_u1c_approach_report *r = report ? report : &local;
    memset(r, 0, sizeof *r);
    if (!st || !env)
        return PR_U1C_APPROACH_ARGUMENT;
    r->before = r->after = st->window1[PR_U1C_SETUP_MONSTER_W582];
    if (st->window1[PR_U1C_SETUP_MONSTER_W582] != 0u) {                /* +0x081E..+0x0824: cmp word, 0 ; jbe skips */
        int rc;
        st->window1[PR_U1C_SETUP_MONSTER_W582] = (uint16_t)(st->window1[PR_U1C_SETUP_MONSTER_W582] - 1u);   /* +0x082A: dec word */
        r->after = st->window1[PR_U1C_SETUP_MONSTER_W582];
        if (!env->hooks.unit25_054e)
            return PR_U1C_APPROACH_HOOK;
        r->called = 1;
        rc = env->hooks.unit25_054e(env->ctx, env, st, env->b6f81, env->b6f82, (uint8_t)(r->after & 0xFFu));   /* +0x082F..+0x0846: 0x2B:0x48 */
        if (rc != 0) {
            r->callback_status = rc;
            return PR_U1C_APPROACH_CALLBACK;
        }
    }
    st->pc = (uint16_t)(st->pc + 1u);                                  /* +0x084B: inc word [0x49ED] */
    return PR_U1C_APPROACH_OK;
}
