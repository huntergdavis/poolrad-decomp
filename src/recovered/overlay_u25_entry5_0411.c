/* Bounded model of GAME.OVR unit 0x25 +0x0411 (entry 5, stub 0x2B:0x39); see docs/re/U25_ENTRY5_0411.md. */
#include "poolrad/recovered/overlay_u25_entry5_0411.h"

#include <string.h>

static const pr_u25_e5_object *lookup(const pr_u25_e5_env *env, pr_far16 p)
{
    size_t i;
    for (i = 0; i < env->object_count; i++)
        if (env->objects[i].offset == p.off && env->objects[i].segment_id == p.seg)
            return &env->objects[i];
    return NULL;
}

/* the little-endian word at es:[di + k]: each byte address wraps at 16 bits; 0 when a byte is outside the view */
static int read_word(const pr_u25_e5_object *o, unsigned k, uint16_t *out)
{
    size_t lo = (size_t)(uint16_t)(o->offset + k), hi = (size_t)(uint16_t)(o->offset + k + 1u);
    if (o->segment == NULL || lo >= o->segment_size || hi >= o->segment_size)
        return 0;
    *out = (uint16_t)(o->segment[lo] | (o->segment[hi] << 8));
    return 1;
}

pr_u25_e5_status pr_u25_entry5_0411(pr_u25_e5_env *env, uint8_t id, pr_far16 *var_a, pr_far16 *var_b, pr_far16 *var_c,
                                    pr_u25_e5_report *report)
{
    pr_u25_e5_report local;
    pr_u25_e5_report *r = report ? report : &local;
    pr_far16 p;
    const pr_u25_e5_object *obj;
    uint16_t w0, w1;
    int rc;
    memset(r, 0, sizeof *r);
    if (!env || !var_a || !var_b || !var_c || (env->objects == NULL && env->object_count != 0u))
        return PR_U25_E5_ARGUMENT;

    var_a->off = 0u; var_a->seg = 0u;                                   /* +0x0417..+0x0422: A := nil */
    if (!env->hooks.getmem_af8_023f) {
        r->failed_call = PR_U25_E5_CALL_GETMEM;
        return PR_U25_E5_HOOK;
    }
    r->getmem_called = 1;
    p.off = 0u; p.seg = 0u;
    rc = env->hooks.getmem_af8_023f(env->ctx, env, PR_U25_E5_OBJECT_SIZE, &p);   /* +0x0423..+0x042B */
    if (rc != 0) {
        r->failed_call = PR_U25_E5_CALL_GETMEM; r->callback_status = rc;
        return PR_U25_E5_CALLBACK;                                      /* A stays nil: it is written only after the call returns */
    }
    *var_a = p;                                                         /* +0x042C..+0x0435: A := P */

    if (!env->hooks.init_u6f_0ec4) {
        r->failed_call = PR_U25_E5_CALL_INIT;
        return PR_U25_E5_HOOK;
    }
    r->init_called = 1;
    rc = env->hooks.init_u6f_0ec4(env->ctx, env, (uint16_t)((p.off & 0xFF00u) | id), p);   /* +0x0436..+0x0444: 0x7C:0x3E; AH is the GetMem offset's high byte */
    if (rc != 0) {
        r->failed_call = PR_U25_E5_CALL_INIT; r->callback_status = rc;
        return PR_U25_E5_CALLBACK;                                      /* A holds P; B and C are untouched */
    }

    obj = lookup(env, *var_a);                                          /* +0x0449..+0x044C: les di,[bp+0xE]; les di,es:[di] reads A NOW */
    if (obj == NULL)
        return PR_U25_E5_UNMAPPED;
    if (!read_word(obj, PR_U25_E5_OFF_B, &w0) || !read_word(obj, PR_U25_E5_OFF_B + 2u, &w1)) {
        r->view_field = PR_U25_E5_OFF_B;
        return PR_U25_E5_VIEW;
    }
    var_b->off = w0; var_b->seg = w1;                                   /* +0x0459..+0x0463: B := far [P + 0xC8] */
    obj = lookup(env, *var_a);                                          /* +0x0466..+0x0469: both LES operations repeat: A is read again (B may alias A) */
    if (obj == NULL)
        return PR_U25_E5_UNMAPPED;                                      /* B already written */
    if (!read_word(obj, PR_U25_E5_OFF_C, &w0) || !read_word(obj, PR_U25_E5_OFF_C + 2u, &w1)) {
        r->view_field = PR_U25_E5_OFF_C;
        return PR_U25_E5_VIEW;                                          /* B already written */
    }
    var_c->off = w0; var_c->seg = w1;                                   /* +0x0466..+0x0478: C := far [P + 0x7F] */
    return PR_U25_E5_OK;
}
