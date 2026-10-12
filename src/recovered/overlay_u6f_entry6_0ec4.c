/* Bounded model of unit 0x6f entry 6 (+0x0EC4); see docs/re/U6F_ENTRY6_0EC4.md. */
#include "poolrad/recovered/overlay_u6f_entry6_0ec4.h"

#include <string.h>

typedef struct {
    const pr_u6f_e6_env *env;
    pr_u6f_e6_report *r;
    pr_u6f_e6_status st;
} Cx;

/* the object whose far pointer is exactly `p`, with `n` bytes accessible from p.off + disp (each byte address wraps at 16 bits) */
static pr_u25_e5_object *obj_at(Cx *c, pr_far16 p, uint16_t disp, size_t n)
{
    size_t i, k;
    for (i = 0; i < c->env->object_count; i++) {
        pr_u25_e5_object *o = &c->env->objects[i];
        if (o->offset != p.off || o->segment_id != p.seg)
            continue;
        if (o->segment == NULL) { c->st = PR_U6F_E6_VIEW; c->r->bad_ptr = p; return NULL; }
        for (k = 0; k < n; k++)
            if ((size_t)(uint16_t)(p.off + disp + k) >= o->segment_size) { c->st = PR_U6F_E6_VIEW; c->r->bad_ptr = p; return NULL; }
        return o;
    }
    c->st = PR_U6F_E6_UNMAPPED; c->r->bad_ptr = p;
    return NULL;
}

static int rd8(Cx *c, pr_far16 p, uint16_t disp, uint8_t *v)
{
    pr_u25_e5_object *o = obj_at(c, p, disp, 1);
    if (!o) return 0;
    *v = o->segment[(uint16_t)(p.off + disp)];
    return 1;
}

static int wr8(Cx *c, pr_far16 p, uint16_t disp, uint8_t v)
{
    pr_u25_e5_object *o = obj_at(c, p, disp, 1);
    if (!o) return 0;
    o->segment[(uint16_t)(p.off + disp)] = v;
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

static int rd16(Cx *c, pr_far16 p, uint16_t disp, uint16_t *v)
{
    pr_u25_e5_object *o = obj_at(c, p, disp, 2);
    if (!o) return 0;
    *v = (uint16_t)(o->segment[(uint16_t)(p.off + disp)] | (o->segment[(uint16_t)(p.off + disp + 1u)] << 8));
    return 1;
}

static int rdp(Cx *c, pr_far16 p, uint16_t disp, pr_far16 *out)
{
    uint16_t a, b;
    if (!rd16(c, p, disp, &a) || !rd16(c, p, (uint16_t)(disp + 2u), &b)) return 0;
    out->off = a; out->seg = b;
    return 1;
}

/* 0xAF8:0x1685: overlap-safe move. The offsets decide the direction (backward when the source offset is below the destination offset). Both
   ranges are checked before the first byte moves. */
static int move_far(Cx *c, pr_far16 src, uint16_t sdisp, pr_far16 dst, uint16_t ddisp, size_t n)
{
    pr_u25_e5_object *s = obj_at(c, src, sdisp, n), *d;
    uint16_t so, dd;
    size_t k;
    if (!s) return 0;
    d = obj_at(c, dst, ddisp, n);
    if (!d) return 0;
    so = (uint16_t)(src.off + sdisp); dd = (uint16_t)(dst.off + ddisp);
    if (so < dd) {
        for (k = n; k > 0; k--) d->segment[(uint16_t)(dd + k - 1u)] = s->segment[(uint16_t)(so + k - 1u)];
    } else {
        for (k = 0; k < n; k++) d->segment[(uint16_t)(dd + k)] = s->segment[(uint16_t)(so + k)];
    }
    return 1;
}

#define NEED(hook, kind) do { if (!(hook)) { c->st = PR_U6F_E6_HOOK; c->r->failed_call = (kind); return 0; } } while (0)
#define CHECKRC(rc, kind) do { if ((rc) != 0) { c->st = PR_U6F_E6_CALLBACK; c->r->callback_status = (rc); c->r->failed_call = (kind); return 0; } } while (0)

static int getmem(Cx *c, uint16_t size, pr_far16 *res)
{
    int rc;
    NEED(c->env->hooks.getmem_af8_023f, PR_U6F_E6_CALL_GETMEM);
    rc = c->env->hooks.getmem_af8_023f(c->env->ctx, c->env, size, res);
    CHECKRC(rc, PR_U6F_E6_CALL_GETMEM);
    return 1;
}

static int freemem(Cx *c, pr_far16 p, uint16_t size)
{
    int rc;
    NEED(c->env->hooks.free_af8_0254, PR_U6F_E6_CALL_FREE);
    rc = c->env->hooks.free_af8_0254(c->env->ctx, c->env, p, size);
    CHECKRC(rc, PR_U6F_E6_CALL_FREE);
    return 1;
}

/* name := StrLoad(c0e98) + Str([0x5376]) + suffix (StrCat caps at 255); ext := StrLoad(c0ea0); then 0x802:0x0716 */
static int load_block(Cx *c, const uint8_t *suffix, uint8_t id, uint16_t *size, pr_far16 *ptr)
{
    const pr_u6f_e6_env *e = c->env;
    uint8_t name[1 + PR_U6F_E6_STR_MAX], digit[2] = { 0, 0 };
    unsigned room, n;
    int rc;
    NEED(e->hooks.str_af8_1366, PR_U6F_E6_CALL_STR);
    rc = e->hooks.str_af8_1366(e->ctx, e, (uint32_t)e->b5376, digit);
    CHECKRC(rc, PR_U6F_E6_CALL_STR);
    if (digit[0] > 1u) digit[0] = 1u;                                            /* string[1] */
    memcpy(name, e->c0e98, 1u + e->c0e98[0]);
    room = PR_U6F_E6_STR_MAX - name[0]; n = digit[0] < room ? digit[0] : room;
    memcpy(name + 1 + name[0], digit + 1, n); name[0] = (uint8_t)(name[0] + n);
    room = PR_U6F_E6_STR_MAX - name[0]; n = suffix[0] < room ? suffix[0] : room;
    memcpy(name + 1 + name[0], suffix + 1, n); name[0] = (uint8_t)(name[0] + n);
    NEED(e->hooks.block_802_0716, PR_U6F_E6_CALL_BLOCK);
    *size = 0u; ptr->off = 0u; ptr->seg = 0u;
    rc = e->hooks.block_802_0716(e->ctx, e, name, e->c0ea0, id, size, ptr);
    CHECKRC(rc, PR_U6F_E6_CALL_BLOCK);
    return 1;
}

/* blocks 2 and 3: do-while chain of `unit`-byte units from the block, the head at obj[chain_disp], links at link_disp */
static int split_block(Cx *c, pr_far16 obj, pr_far16 blk, uint16_t size, uint16_t unit, uint16_t chain_disp, uint16_t link_disp, unsigned *count)
{
    pr_far16 head, cur;
    uint16_t off = 0u;
    if (!getmem(c, unit, &head) || !wrp(c, obj, chain_disp, head) || !rdp(c, obj, chain_disp, &cur)) return 0;
    for (;;) {
        if (!move_far(c, blk, off, cur, 0u, unit)) return 0;
        if (!wr16(c, cur, link_disp, 0u) || !wr16(c, cur, (uint16_t)(link_disp + 2u), 0u)) return 0;
        (*count)++;
        if ((uint16_t)(off + unit) < off) { c->st = PR_U6F_E6_RANGE; return 0; }   /* the 16-bit offset would wrap: the original loop would not terminate */
        off = (uint16_t)(off + unit);
        if (off >= size) break;
        {
            pr_far16 nxt;
            if (!getmem(c, unit, &nxt) || !wrp(c, cur, link_disp, nxt) || !rdp(c, cur, link_disp, &cur)) return 0;
        }
    }
    return freemem(c, blk, size);
}

pr_u6f_e6_status pr_u6f_entry6(pr_u6f_e6_ds *ds, const pr_u6f_e6_env *env, pr_far16 obj, uint8_t id, pr_u6f_e6_report *report)
{
    pr_u6f_e6_report local;
    pr_u6f_e6_report *r = report ? report : &local;
    Cx cx;
    Cx *c = &cx;
    pr_far16 blk;
    uint16_t size;
    uint8_t b, i;
    memset(r, 0, sizeof *r);
    if (!ds || !env || (env->objects == NULL && env->object_count != 0u) || !env->c0e98 || !env->c0e9c || !env->c0ea0 || !env->c0ebc || !env->c0ec0)
        return PR_U6F_E6_ARGUMENT;
    c->env = env; c->r = r; c->st = PR_U6F_E6_OK;

    if (!load_block(c, env->c0e9c, id, &size, &blk)) return c->st;               /* +0x0ECB..+0x0F27: block 1 */
    r->size[0] = size;
    if (size == 0u) return PR_U6F_E6_ERROR_EXIT;                                  /* +0x0F2C..+0x0F49: message, then 0x899:0 (terminate) */
    r->blocks_loaded++;
    if (!move_far(c, blk, 0u, obj, 0u, PR_U6F_E6_RECORD)) return c->st;          /* +0x0F4E..+0x0F5C */
    if (!wr16(c, obj, 0xC8u, 0u) || !wr16(c, obj, 0xCAu, 0u) || !wr16(c, obj, 0x7Fu, 0u) || !wr16(c, obj, 0x81u, 0u)
        || !wr16(c, obj, 0x104u, 0u) || !wr16(c, obj, 0x106u, 0u) || !wr16(c, obj, 0x108u, 0u) || !wr16(c, obj, 0x10Au, 0u))
        return c->st;                                                              /* +0x0F6A..+0x0FA0 */
    if (!rd8(c, obj, 0x2Eu, &b)) return c->st;                                    /* +0x0FA5..+0x0FD6 */
    if (b == 1u || b == 3u || b == 5u) { if (!wr8(c, obj, 0xC0u, 1u)) return c->st; }
    else if (b == 2u || b == 4u || b == 6u || b == 7u) { if (!wr8(c, obj, 0xC0u, 2u)) return c->st; }
    if (!rd8(c, obj, 0xC0u, &r->class_byte)) return c->st;

    for (i = 1u; i <= 6u; i++) {                                                  /* +0x0FDC..+0x106B: i = 1..3, [+0xC4] := 0xC4, i = 5..6 */
        uint8_t al = 0u;
        uint16_t w;
        int rc;
        if (i == 4u) { if (!wr8(c, obj, 0xC4u, 0xC4u)) return c->st; continue; }
        if (!env->hooks.roll_b0_0048) { r->failed_call = PR_U6F_E6_CALL_ROLL; return PR_U6F_E6_HOOK; }
        rc = env->hooks.roll_b0_0048(env->ctx, env, 1u, 6u, &al);
        if (rc != 0) { r->failed_call = PR_U6F_E6_CALL_ROLL; r->callback_status = rc; return PR_U6F_E6_CALLBACK; }
        r->rolls[r->n_rolls++] = al;
        w = (uint16_t)(((uint16_t)(al + 8u) << 4) + al);
        if (!wr8(c, obj, (uint16_t)(0xC0u + i), (uint8_t)(w & 0xFFu))) return c->st;
    }
    if (!freemem(c, blk, size)) return c->st;                                     /* +0x106D..+0x1076 */
    ds->b557c = 1u;                                                               /* +0x107B */

    if (!load_block(c, env->c0ebc, id, &size, &blk)) return c->st;               /* +0x1080..+0x10C2: block 2 */
    r->size[1] = size;
    if (size != 0u) {
        r->blocks_loaded++;
        if (!split_block(c, obj, blk, size, PR_U6F_E6_UNIT9, 0x7Fu, 5u, &r->units9)) return c->st;
    }
    if (!load_block(c, env->c0ec0, id, &size, &blk)) return c->st;               /* +0x1168..+0x11AA: block 3 */
    r->size[2] = size;
    if (size != 0u) {
        r->blocks_loaded++;
        if (!split_block(c, obj, blk, size, PR_U6F_E6_UNIT3F, 0xC8u, 0x2Au, &r->units3f)) return c->st;
    }
    ds->b557c = 0u;                                                               /* +0x1252 */
    {
        int rc;
        if (!env->hooks.flush_899_0104) { r->failed_call = PR_U6F_E6_CALL_FLUSH; return PR_U6F_E6_HOOK; }
        rc = env->hooks.flush_899_0104(env->ctx, env);                            /* +0x1257 */
        if (rc != 0) { r->failed_call = PR_U6F_E6_CALL_FLUSH; r->callback_status = rc; return PR_U6F_E6_CALLBACK; }
    }
    return PR_U6F_E6_OK;
}
