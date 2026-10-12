/* Bounded model of GAME.OVR unit 0x1c +0x125D (ECL opcode 0x1C); see docs/re/U1C_CLEARMONSTERS_125D.md. */
#include "poolrad/recovered/overlay_u1c_clearmonsters_125d.h"

#include <string.h>

static int link_present(const pr_u1c_cmon_node *n)
{
    unsigned k;
    if (n->segment == NULL)
        return 0;
    for (k = 0; k < 4u; k++)
        if ((size_t)(uint16_t)(n->offset + PR_U1C_CMON_LINK_OFFSET + k) >= n->segment_size)
            return 0;
    return 1;
}

pr_u1c_cmon_status pr_u1c_op_clearmonsters(pr_rec_ecl_loader_state *st, pr_u1c_cmon_env *env, pr_u1c_cmon_report *report)
{
    pr_u1c_cmon_report local;
    pr_u1c_cmon_report *r = report ? report : &local;
    memset(r, 0, sizeof *r);
    if (!st || !env)
        return PR_U1C_CMON_ARGUMENT;
    st->pc = (uint16_t)(st->pc + 1u);                                  /* +0x1263: inc word [0x49ED] */
    env->b4435 = 0u;                                                   /* +0x1267 */
    env->b84e7 = 0u;                                                   /* +0x126C */
    env->b6f83 = PR_U1C_CMON_B6F83;                                    /* +0x1271 */
    memset(env->block_67f4, 0, sizeof env->block_67f4);                /* +0x1276..+0x1282: FillChar(DS:0x67F4, 0x1C, 0) */
    while (env->head != NULL) {                                        /* +0x1287..+0x128E: or of the two head words */
        const pr_u1c_cmon_node *node = env->head;
        const pr_u1c_cmon_node *link;
        int rc;
        if (r->freed >= PR_U1C_CMON_LIST_CAP || !link_present(node))
            return PR_U1C_CMON_LIST;
        link = node->next;                                             /* +0x1294..+0x129F: the link words are read before the free */
        if (!env->hooks.free_af8_0254)
            return PR_U1C_CMON_HOOK;
        rc = env->hooks.free_af8_0254(env->ctx, env, node, PR_U1C_CMON_NODE_SIZE);   /* +0x12A2..+0x12AE: 0xAF8:0x0254 */
        if (rc != 0) {
            r->callback_status = rc;
            return PR_U1C_CMON_CALLBACK;
        }
        ++r->freed;
        env->head = link;                                              /* +0x12B3..+0x12BC: head := link */
    }
    return PR_U1C_CMON_OK;
}
