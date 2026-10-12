/* Bounded model of GAME.OVR unit 0x1c +0x125D (ECL opcode 0x1C); see docs/re/U1C_CLEARMONSTERS_125D.md. */
#include "poolrad/recovered/overlay_u1c_clearmonsters_125d.h"

#include <string.h>

static const pr_u1c_cmon_node *lookup(const pr_u1c_cmon_env *env, uint16_t off, uint16_t seg)
{
    size_t i;
    for (i = 0; i < env->node_count; i++)
        if (env->nodes[i].offset == off && env->nodes[i].segment_id == seg)
            return &env->nodes[i];
    return NULL;
}

/* the little-endian word at es:[di + k]: each byte address wraps at 16 bits; 0 when a byte is outside the view */
static int read_word(const pr_u1c_cmon_node *n, unsigned k, uint16_t *out)
{
    size_t lo = (size_t)(uint16_t)(n->offset + k), hi = (size_t)(uint16_t)(n->offset + k + 1u);
    if (n->segment == NULL || lo >= n->segment_size || hi >= n->segment_size)
        return 0;
    *out = (uint16_t)(n->segment[lo] | (n->segment[hi] << 8));
    return 1;
}

pr_u1c_cmon_status pr_u1c_op_clearmonsters(pr_rec_ecl_loader_state *st, pr_u1c_cmon_env *env, pr_u1c_cmon_report *report)
{
    pr_u1c_cmon_report local;
    pr_u1c_cmon_report *r = report ? report : &local;
    memset(r, 0, sizeof *r);
    if (!st || !env || (env->nodes == NULL && env->node_count != 0u))   /* a malformed host table is rejected before any effect */
        return PR_U1C_CMON_ARGUMENT;
    st->pc = (uint16_t)(st->pc + 1u);                                  /* +0x1263: inc word [0x49ED] */
    env->b4435 = 0u;                                                   /* +0x1267 */
    env->b84e7 = 0u;                                                   /* +0x126C */
    env->b6f83 = PR_U1C_CMON_B6F83;                                    /* +0x1271 */
    memset(env->block_67f4, 0, sizeof env->block_67f4);                /* +0x1276..+0x1282: FillChar(DS:0x67F4, 0x1C, 0) */
    while ((env->head_off | env->head_seg) != 0u) {                    /* +0x1287..+0x128E: or of the two head words */
        const pr_u1c_cmon_node *node;
        uint16_t link_off, link_seg;
        int rc;
        if (r->freed >= PR_U1C_CMON_LIST_CAP)
            return PR_U1C_CMON_LIST;
        node = lookup(env, env->head_off, env->head_seg);              /* the host's mapping of the far pointer to its bytes */
        if (node == NULL)
            return PR_U1C_CMON_UNMAPPED;
        if (!read_word(node, PR_U1C_CMON_LINK_OFFSET, &link_off) ||    /* +0x1294..+0x129F: the raw link words, read before the free */
            !read_word(node, PR_U1C_CMON_LINK_OFFSET + 2u, &link_seg))
            return PR_U1C_CMON_LIST;
        if (!env->hooks.free_af8_0254)
            return PR_U1C_CMON_HOOK;
        rc = env->hooks.free_af8_0254(env->ctx, env, node, PR_U1C_CMON_NODE_SIZE);   /* +0x12A2..+0x12AE: 0xAF8:0x0254 */
        if (rc != 0) {
            r->callback_status = rc;
            return PR_U1C_CMON_CALLBACK;
        }
        ++r->freed;
        env->head_off = link_off;                                      /* +0x12B3..+0x12BC: head := link */
        env->head_seg = link_seg;
    }
    return PR_U1C_CMON_OK;
}
