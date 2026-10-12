/* Bounded model of GAME.OVR unit 0x1c +0x0001 (ECL opcode 0x00 EXIT); see
 * docs/re/U1C_EXIT_0001.md. */
#include "poolrad/recovered/overlay_u1c_exit_0001.h"

#include <string.h>

static const uint8_t empty_pascal_string[1] = { 0 };       /* the byte at +0x0000: length 0 */

pr_u1c_exit_status pr_u1c_op_exit(pr_rec_ecl_loader_state *st, pr_u1c_exit_env *env, pr_u1c_exit_report *report)
{
    pr_u1c_exit_report local;
    pr_u1c_exit_report *r = report ? report : &local;
    memset(r, 0, sizeof *r);
    if (!st || !env || st->call_depth > PR_REC_ECL_CALL_STACK_MAX)
        return PR_U1C_EXIT_ARGUMENT;

    if (st->b84e2 != 0u) {                                           /* +0x0007..+0x000C */
        static const uint8_t box1[6] = { 1, 1, 0x0F, 0x0F, 0, 0x0F };   /* +0x000E..+0x001D */
        static const uint8_t box2[6] = { 3, 3, 0x0D, 0x0D, 0, 0x0F };   /* +0x0040..+0x004F */
        int rc;
        if (!env->hooks.box_3f1_02e8)
            return PR_U1C_EXIT_HOOK;
        rc = env->hooks.box_3f1_02e8(env->ctx, env, box1, empty_pascal_string);     /* +0x002A, +0x002F */
        r->boxes++;
        if (rc != 0) {
            r->callback_status = rc;
            r->failed_box = 1;
            return PR_U1C_EXIT_CALLBACK;
        }
        if (st->window0[PR_U1C_EXIT_W0_WORD] != 0u) {                /* +0x0034..+0x003E */
            if (!env->hooks.box_3f1_02e8)                            /* the first call may have changed the hooks */
                return PR_U1C_EXIT_HOOK;                             /* first-call effects retained */
            rc = env->hooks.box_3f1_02e8(env->ctx, env, box2, empty_pascal_string); /* +0x005C, +0x0061 */
            r->boxes++;
            if (rc != 0) {
                r->callback_status = rc;
                r->failed_box = 2;
                return PR_U1C_EXIT_CALLBACK;
            }
        }
    }
    if (env->b4430 != 0u) {                                          /* +0x0066..+0x007B */
        env->w5d92 = env->w4436;
        env->w5d94 = env->w4438;
        env->b4430 = 0u;
        r->restored_character = 1;
    }
    st->b84c8 = 0u;                                                  /* +0x0080..+0x008C: FillChar(DS:0x84C8, 2, 0) */
    st->b84c9 = 0u;
    st->b84e0 = 0u;                                                  /* +0x0091 */
    st->b84e2 = 0u;                                                  /* +0x0096 */
    st->b442e = 1u;                                                  /* +0x009B: the run loop's stop byte */
    st->pc = (uint16_t)(st->pc + 1u);                                /* +0x00A0: inc word [0x49ED] */
    r->freed_nodes = st->call_depth;                                 /* +0x00A4..+0x00DF: FreeMem(node, 6) until nil */
    st->call_depth = 0u;
    st->b5e25 = PR_U1C_EXIT_STATE_5E25;                              /* +0x00E1 */
    st->b5e24 = PR_U1C_EXIT_STATE_5E24;                              /* +0x00E6 */
    env->b84de = 0u;                                                 /* +0x00EB */
    return PR_U1C_EXIT_OK;                                           /* +0x00F0..+0x00F3 */
}
