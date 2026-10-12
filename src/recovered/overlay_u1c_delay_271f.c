/* Bounded model of GAME.OVR unit 0x1c +0x271F (ECL opcode 0x3A); see docs/re/U1C_DELAY_271F.md. */
#include "poolrad/recovered/overlay_u1c_delay_271f.h"

#include <string.h>

uint16_t pr_res_0709_0dbe_arg(uint8_t b49e2)
{
    return (uint16_t)((uint32_t)b49e2 * PR_U1C_DELAY_UNIT_MS);          /* +0x0DC1..+0x0DCB: mul dx (dx:ax), push ax; at most 57,375 */
}

pr_u1c_delay_status pr_u1c_op_delay(pr_rec_ecl_loader_state *st, pr_u1c_delay_env *env, pr_u1c_delay_report *report)
{
    pr_u1c_delay_report local;
    pr_u1c_delay_report *r = report ? report : &local;
    int rc;
    memset(r, 0, sizeof *r);
    if (!st || !env)
        return PR_U1C_DELAY_ARGUMENT;
    st->pc = (uint16_t)(st->pc + 1u);                                   /* +0x2722: inc word [0x49ED], before the call */
    r->ms = pr_res_0709_0dbe_arg(env->b49e2);
    if (!env->hooks.delay_a40_029c)
        return PR_U1C_DELAY_HOOK;
    r->called = 1;
    rc = env->hooks.delay_a40_029c(env->ctx, env, r->ms);               /* 0x709:0xDBE -> 0xA40:0x029C (ms) */
    if (rc != 0) {
        r->callback_status = rc;
        return PR_U1C_DELAY_CALLBACK;
    }
    return PR_U1C_DELAY_OK;
}
