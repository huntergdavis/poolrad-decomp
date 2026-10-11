/* Portable C for GAME.OVR unit 0x38 public entries 81 and 110
 * (docs/re/OVERLAY_UNIT38_WALK_ENTRIES.md). */
#include "poolrad/recovered/overlay_unit38_walk.h"

static pr_u38_far_ptr read_far_ptr(const uint8_t *bytes)
{
    pr_u38_far_ptr p;
    p.offset = (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
    p.segment = (uint16_t)(bytes[2] | ((uint16_t)bytes[3] << 8));
    return p;
}

static void put(unsigned *out, unsigned value)
{
    if (out) *out = value;
}

/* +0x268A: cmp byte [0x682A],4; je body; cmp byte [0x682A],2; jne done;
 * body: les di,[bp+0xC]; mov al,es:[di+0x14]; five jb/ja range tests each
 * storing 1..5 into [bp-1] and jumping to the add (no store otherwise);
 * add: mov al,[bp-1]; xor ah,ah; mov dx,ax; mov al,[0x6816]; xor ah,ah;
 * add ax,dx; mov [0x6816],al. */
pr_u38_status pr_u38_entry81(pr_u38w_state *state, const uint8_t *record,
                             uint8_t stack_residue, unsigned *paths, unsigned *bracket_out)
{
    uint8_t value, bracket;
    unsigned taken = PR_U38_PATH_EFFECT;
    put(paths, 0);
    put(bracket_out, 0);
    if (!state || !record) return PR_U38_ARGUMENT;
    if (state->b682a != 4u && state->b682a != 2u) return PR_U38_OK;
    value = record[0x14u];
    if (value >= 4u && value <= 6u) bracket = 1;
    else if (value >= 7u && value <= 10u) bracket = 2;
    else if (value >= 11u && value <= 13u) bracket = 3;
    else if (value >= 14u && value <= 17u) bracket = 4;
    else if (value >= 18u && value <= 20u) bracket = 5;
    else { bracket = stack_residue; taken |= PR_U38_PATH_SECOND; }   /* [bp-1] never written */
    state->b6816 = (uint8_t)(state->b6816 + bracket);
    put(paths, taken);
    put(bracket_out, bracket);
    return PR_U38_OK;
}

/* +0x2E37, see the header for the instruction-level description. The
 * access order below follows the original: P, Q (scratch store), head
 * pointer from Q[0xC8]; per node: null test, flag test, node[0x34], then
 * node[0x2E+i] / node[0x31] for i = 1..3, then node[0x2A] link. */
pr_u38_walk_status pr_u38_entry110(pr_u38w_state *state, const uint8_t *record,
                                   pr_u38_resolve_fn resolve, void *context,
                                   unsigned node_limit, unsigned *nodes_out, unsigned *matched_out)
{
    const uint8_t *object;
    const uint8_t *target;
    pr_u38_far_ptr node;
    unsigned nodes = 0;
    put(nodes_out, 0);
    put(matched_out, 0);
    if (!state || !record || !resolve || node_limit == 0) return PR_U38_WALK_ARGUMENT;
    object = resolve(context, read_far_ptr(record + PR_U38_RECORD_SLOT_108), 0x0Eu);
    if (!object) return PR_U38_WALK_ARGUMENT;
    state->scratch_6826 = read_far_ptr(object + 0x0Au);
    target = resolve(context, state->scratch_6826, PR_U38_RECORD_SLOT_C8 + 4u);
    if (!target) return PR_U38_WALK_ARGUMENT;
    node = read_far_ptr(target + PR_U38_RECORD_SLOT_C8);
    for (;;) {
        const uint8_t *n;
        if (node.offset == 0 && node.segment == 0) { put(nodes_out, nodes); return PR_U38_WALK_END; }
        if (state->b681e != 0) { put(nodes_out, nodes); return PR_U38_WALK_FLAG; }
        if (nodes == node_limit) { put(nodes_out, nodes); return PR_U38_WALK_LIMIT; }
        n = resolve(context, node, PR_U38_NODE_MIN_SIZE);
        if (!n) { put(nodes_out, nodes); return PR_U38_WALK_ARGUMENT; }
        nodes++;
        if (n[0x34u] != 0) {
            unsigned i;
            for (i = 1; i <= 3u; ++i) {
                uint8_t probe = n[0x2Eu + i];
                if ((probe == 0x98u && n[0x31u] == 0xFCu) || probe == 0x76u) {
                    state->b681e = 1;
                    put(matched_out, 1);
                }
            }
        }
        node = read_far_ptr(n + PR_U38_NODE_LINK);
    }
}
