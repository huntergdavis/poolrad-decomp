/* Bounded model of unit 0x1c +0x0537..+0x0816 of ECL opcode 0x0B; see docs/re/U1C_LOAD_MONSTER_TAIL_0537.md. */
#include "poolrad/recovered/overlay_u1c_load_monster_tail_0537.h"

#include <string.h>

typedef struct {
    const pr_u1c_lmt_env *env;
    pr_u1c_lmt_report *r;
    pr_u1c_lmt_status st;
} Cx;

static int is_nil(pr_far16 p) { return p.off == 0u && p.seg == 0u; }

/* the object whose far pointer is exactly `p`, with `n` bytes readable from `p.off + disp` (each byte address wraps at 16 bits) */
static pr_u25_e5_object *obj_at(Cx *c, pr_far16 p, uint16_t disp, size_t n)
{
    size_t i, k;
    for (i = 0; i < c->env->object_count; i++) {
        pr_u25_e5_object *o = &c->env->objects[i];
        if (o->offset != p.off || o->segment_id != p.seg)
            continue;
        if (o->segment == NULL) { c->st = PR_U1C_LMT_VIEW; c->r->bad_ptr = p; return NULL; }
        for (k = 0; k < n; k++)
            if ((size_t)(uint16_t)(p.off + disp + k) >= o->segment_size) { c->st = PR_U1C_LMT_VIEW; c->r->bad_ptr = p; return NULL; }
        return o;
    }
    c->st = PR_U1C_LMT_UNMAPPED; c->r->bad_ptr = p;
    return NULL;
}

static int rd16(Cx *c, pr_far16 p, uint16_t disp, uint16_t *v)
{
    pr_u25_e5_object *o = obj_at(c, p, disp, 2);
    if (!o) return 0;
    *v = (uint16_t)(o->segment[(uint16_t)(p.off + disp)] | (o->segment[(uint16_t)(p.off + disp + 1u)] << 8));
    return 1;
}

static int rdp(Cx *c, pr_far16 p, uint16_t disp, pr_far16 *out)       /* a far pointer word pair: offset at disp, segment at disp + 2 */
{
    uint16_t a, b;
    if (!rd16(c, p, disp, &a) || !rd16(c, p, (uint16_t)(disp + 2u), &b)) return 0;
    out->off = a; out->seg = b;
    return 1;
}

static int wr16(Cx *c, pr_far16 p, uint16_t disp, uint16_t v)
{
    pr_u25_e5_object *o = obj_at(c, p, disp, 2);
    if (!o) return 0;
    o->segment[(uint16_t)(p.off + disp)] = (uint8_t)(v & 0xFFu);
    o->segment[(uint16_t)(p.off + disp + 1u)] = (uint8_t)(v >> 8);
    return 1;
}

static int wrp(Cx *c, pr_far16 p, uint16_t disp, pr_far16 v)
{
    return wr16(c, p, disp, v.off) && wr16(c, p, (uint16_t)(disp + 2u), v.seg);
}

static int wr8(Cx *c, pr_far16 p, uint16_t disp, uint8_t v)
{
    pr_u25_e5_object *o = obj_at(c, p, disp, 1);
    if (!o) return 0;
    o->segment[(uint16_t)(p.off + disp)] = v;
    return 1;
}

/* 0xAF8:0x0A3F Move(src, dst, n) from a host buffer */
static int move_in(Cx *c, const uint8_t *src, pr_far16 dst, size_t n)
{
    pr_u25_e5_object *d = obj_at(c, dst, 0, n);
    size_t k;
    if (!d) return 0;
    for (k = 0; k < n; k++) d->segment[(uint16_t)(dst.off + k)] = src[k];
    return 1;
}

/* Move between two table objects: both ranges are checked before the first byte moves */
static int move_far(Cx *c, pr_far16 src, pr_far16 dst, size_t n)
{
    pr_u25_e5_object *s = obj_at(c, src, 0, n), *d;
    size_t k;
    if (!s) return 0;
    d = obj_at(c, dst, 0, n);
    if (!d) return 0;
    for (k = 0; k < n; k++) d->segment[(uint16_t)(dst.off + k)] = s->segment[(uint16_t)(src.off + k)];
    return 1;
}

static int getmem(Cx *c, uint16_t size, pr_far16 *res)
{
    int rc;
    if (!c->env->hooks.getmem_af8_023f) { c->st = PR_U1C_LMT_HOOK; return 0; }
    c->r->getmem_calls++;
    rc = c->env->hooks.getmem_af8_023f(c->env->ctx, c->env, size, res);
    if (rc != 0) { c->st = PR_U1C_LMT_CALLBACK; c->r->callback_status = rc; return 0; }
    return 1;
}

/* One sidecar chain copy (+0x063E..+0x070D for B, +0x0710..+0x07EA for C): for each source node, prepend a GetMem(size) copy to the chain
   at cur[chain_disp]; the node's link is at link_disp. Returns 0 on a stop (c->st set). */
static int copy_chain(Cx *c, pr_far16 cur, pr_far16 head, uint16_t size, uint16_t chain_disp, uint16_t link_disp, unsigned *count)
{
    pr_far16 it = head;
    unsigned steps = 0;
    while (!is_nil(it)) {
        pr_far16 old, m, chain;
        if (++steps > PR_U1C_LMT_WALK_MAX) { c->st = PR_U1C_LMT_CYCLE; return 0; }
        if (!rdp(c, cur, chain_disp, &chain)) return 0;
        if (is_nil(chain)) {                                       /* first node: the chain word pair is 0:0 */
            if (!getmem(c, size, &m) || !wrp(c, cur, chain_disp, m) || !move_far(c, it, m, size)) return 0;
            if (!wr16(c, m, link_disp, 0u) || !wr16(c, m, (uint16_t)(link_disp + 2u), 0u)) return 0;
        } else {                                                   /* later nodes: the old pair is kept as the new node's link (prepend) */
            old = chain;
            if (!getmem(c, size, &m) || !wrp(c, cur, chain_disp, m) || !move_far(c, it, m, size)) return 0;
            if (!wrp(c, m, link_disp, old)) return 0;
        }
        (*count)++;
        if (!rdp(c, it, link_disp, &it)) return 0;
    }
    return 1;
}

pr_u1c_lmt_status pr_u1c_load_monster_tail(pr_u1c_lmt_ds *ds, const pr_u1c_lmt_env *env, const pr_u1c_lmt_in *in, pr_u1c_lmt_report *report)
{
    pr_u1c_lmt_report local;
    pr_u1c_lmt_report *r = report ? report : &local;
    Cx c;
    pr_far16 cur, link, a;
    uint8_t n;
    unsigned steps = 0;
    memset(r, 0, sizeof *r);
    if (!ds || !env || !in || (env->objects == NULL && env->object_count != 0u))
        return PR_U1C_LMT_ARGUMENT;
    c.env = env; c.r = r; c.st = PR_U1C_LMT_OK;
    n = in->n;
    a = in->var_a;

    cur = env->party_head;                                                       /* +0x0537..+0x0559: walk to the tail */
    for (;;) {
        if (!rdp(&c, cur, 0x104u, &link)) return c.st;
        if (is_nil(link)) break;
        if (++steps > PR_U1C_LMT_WALK_MAX) return PR_U1C_LMT_CYCLE;
        cur = link;
        r->walked++;
    }
    r->tail = cur;

    if (!wrp(&c, cur, 0x104u, a)) return c.st;                                   /* +0x055B..+0x059F: the first monster is A itself */
    if (!rdp(&c, cur, 0x104u, &cur)) return c.st;                                /* cur := tail.link (reads back A) */
    if (!wr8(&c, cur, 0xBFu, ds->b6f83) || !wr16(&c, cur, 0x104u, 0u) || !wr16(&c, cur, 0x106u, 0u)) return c.st;
    ds->b4435 = (uint8_t)(ds->b4435 + 1u); n = (uint8_t)(n + 1u); r->monsters++;

    for (;;) {
        pr_far16 p;
        if (n > in->count) break;                                                /* +0x05A2..+0x05B4 */
        if (ds->b4435 >= PR_U1C_LMT_CAP) { r->cap_stop = 1; break; }
        if (!getmem(&c, PR_U1C_LMT_RECORD, &p)) return c.st;                     /* +0x05B7..+0x05E9 */
        r->copies++;
        if (!wrp(&c, cur, 0x104u, p)) return c.st;
        if (!rdp(&c, cur, 0x104u, &cur)) return c.st;
        if (!move_in(&c, in->record_copy, cur, PR_U1C_LMT_RECORD)) return c.st;  /* +0x05EC..+0x05FB */
        if (!wr8(&c, cur, 0xBFu, ds->b6f83) || !wr16(&c, cur, 0x104u, 0u) || !wr16(&c, cur, 0x106u, 0u)) return c.st;   /* +0x0600..+0x0618 */
        if (!wr16(&c, cur, 0x7Fu, 0u) || !wr16(&c, cur, 0x81u, 0u) || !wr16(&c, cur, 0xC8u, 0u) || !wr16(&c, cur, 0xCAu, 0u)) return c.st;
        n = (uint8_t)(n + 1u); ds->b4435 = (uint8_t)(ds->b4435 + 1u); r->monsters++;
        if (!is_nil(in->head_b) && !copy_chain(&c, cur, in->head_b, 0x3Fu, 0xC8u, 0x2Au, &r->b_nodes)) return c.st;   /* +0x063E..+0x070D */
        if (!is_nil(in->head_c) && !copy_chain(&c, cur, in->head_c, 9u, 0x7Fu, 5u, &r->c_nodes)) return c.st;        /* +0x0710..+0x07EA */
    }

    ds->b6f83 = (uint8_t)(ds->b6f83 + 1u);                                       /* +0x07FD..+0x0816 */
    ds->b84e7 = 1u;
    ds->w5d92 = in->saved_w5d92; ds->w5d94 = in->saved_w5d94;
    return PR_U1C_LMT_OK;
}
