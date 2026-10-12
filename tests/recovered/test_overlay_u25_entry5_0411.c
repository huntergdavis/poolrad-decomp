/* Synthetic test for the unit 0x25 +0x0411 wrapper model (stub 0x2B:0x39). The heap, the object bytes and both hooks are fabricated;
 * the model never writes object bytes, so everything the wrapper returns comes from what the fake initializer wrote. Checked:
 * A zeroed before GetMem and written only after it returns, the GetMem size (0x11D), the initializer's arguments and the state it sees
 * (A = P, B and C not yet written), B/C decoded little-endian from +0xC8/+0x7F, the low byte of id only, the unwritten results at every
 * failure, the unmapped and short-view paths, the 16-bit wrap, and the argument guards. */
#include "poolrad/recovered/overlay_u25_entry5_0411.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t obj_bytes[0x200];
static uint8_t big[0x10000];
static pr_u25_e5_object table[2];
static pr_far16 *g_a, *g_b, *g_c;

typedef struct {
    int log[4]; unsigned n;
    uint16_t size; pr_far16 give;              /* GetMem: the size seen, the pointer to return */
    pr_far16 a_in_getmem, a_in_init, b_in_init, c_in_init;
    uint16_t id; pr_far16 obj;                  /* the id WORD the initializer received */
    int fail_at;                               /* 1 = GetMem, 2 = initializer */
    int write_links;                           /* the initializer writes B and C into the object */
} Rec;

static int h_getmem(void *c, pr_u25_e5_env *e, uint16_t size, pr_far16 *res)
{ Rec *r = c; (void)e; r->size = size; r->a_in_getmem = *g_a; *res = r->give; r->log[r->n++] = 1; return r->fail_at == 1 ? 9 : 0; }
static int h_init(void *c, pr_u25_e5_env *e, uint16_t id, pr_far16 obj)
{
    Rec *r = c; (void)e;
    r->id = id; r->obj = obj; r->a_in_init = *g_a; r->b_in_init = *g_b; r->c_in_init = *g_c; r->log[r->n++] = 2;
    if (r->write_links) {                                     /* what the real initializer would leave: the two list heads */
        obj_bytes[0x10 + 0xC8] = 0x34; obj_bytes[0x10 + 0xC9] = 0x12; obj_bytes[0x10 + 0xCA] = 0x78; obj_bytes[0x10 + 0xCB] = 0x56;
        obj_bytes[0x10 + 0x7F] = 0xBC; obj_bytes[0x10 + 0x80] = 0x9A; obj_bytes[0x10 + 0x81] = 0xF0; obj_bytes[0x10 + 0x82] = 0xDE;
    }
    return r->fail_at == 2 ? 9 : 0;
}

static pr_far16 swap_to;
static int h_init_swap(void *c, pr_u25_e5_env *e, uint16_t id, pr_far16 obj)      /* an initializer that changes variable A */
{ (void)c; (void)e; (void)id; (void)obj; *g_a = swap_to; return 0; }

static void env_init(pr_u25_e5_env *e, Rec *r)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    memset(obj_bytes, 0x5A, sizeof obj_bytes);
    table[0].segment = obj_bytes; table[0].segment_size = sizeof obj_bytes; table[0].segment_id = 0x3000; table[0].offset = 0x0010;
    e->objects = table; e->object_count = 1;
    r->give.off = 0x0010; r->give.seg = 0x3000; r->write_links = 1;
    e->hooks.getmem_af8_023f = h_getmem; e->hooks.init_u6f_0ec4 = h_init; e->ctx = r;
}

int main(void)
{
    pr_u25_e5_env env; Rec rec; pr_u25_e5_report rep;
    pr_far16 a, b, c;
    uint8_t before[sizeof obj_bytes];
#define SENT(v) do { (v).off = 0xAAAA; (v).seg = 0xBBBB; } while (0)
    g_a = &a; g_b = &b; g_c = &c;

    /* the normal order: A zeroed before GetMem; GetMem(0x11D); A := P; initializer(id low byte, P) sees A = P and B, C unwritten; then B, C */
    env_init(&env, &rec); SENT(a); SENT(b); SENT(c);
    CHECK(pr_u25_entry5_0411(&env, 0x5A, &a, &b, &c, &rep) == PR_U25_E5_OK && rep.getmem_called && rep.init_called);
    CHECK(rec.n == 2 && rec.log[0] == 1 && rec.log[1] == 2 && rec.size == 0x11D);
    CHECK(rec.a_in_getmem.off == 0 && rec.a_in_getmem.seg == 0);                                   /* A is nil when GetMem runs */
    CHECK(rec.id == 0x005A && rec.obj.off == 0x0010 && rec.obj.seg == 0x3000);                      /* object offset high byte 0x00: the word is 0x005A */
    CHECK(rec.a_in_init.off == 0x0010 && rec.a_in_init.seg == 0x3000);                             /* A already holds P */
    CHECK(rec.b_in_init.off == 0xAAAA && rec.c_in_init.off == 0xAAAA);                             /* B and C are not yet written */
    CHECK(a.off == 0x0010 && a.seg == 0x3000);
    CHECK(b.off == 0x1234 && b.seg == 0x5678 && c.off == 0x9ABC && c.seg == 0xDEF0);               /* +0xC8 -> B, +0x7F -> C, little-endian */

    /* the wrapper never writes the object: with an initializer that writes nothing, B and C are the object's own 0x5A bytes */
    env_init(&env, &rec); rec.write_links = 0; SENT(a); SENT(b); SENT(c); memcpy(before, obj_bytes, sizeof before);
    CHECK(pr_u25_entry5_0411(&env, 1, &a, &b, &c, NULL) == PR_U25_E5_OK);
    CHECK(b.off == 0x5A5A && b.seg == 0x5A5A && c.off == 0x5A5A && c.seg == 0x5A5A && memcmp(before, obj_bytes, sizeof before) == 0);

    /* the initializer's id word is (GetMem offset & 0xFF00) | id low byte: AL is loaded over the AX the GetMem result left, so the offset's high byte
       propagates (0xAB10 with id 0x5A -> 0xAB5A); all 256 low bytes for several offset high bytes */
    env_init(&env, &rec); rec.give.off = 0xAB10; table[0].offset = 0xAB10; SENT(a); SENT(b); SENT(c);
    (void)pr_u25_entry5_0411(&env, 0x5A, &a, &b, &c, NULL);                                        /* the view at 0xAB10 is too small for +0xC8: VIEW, but the initializer ran */
    CHECK(rec.id == 0xAB5A && rec.obj.off == 0xAB10);
    { static const uint16_t offs[] = { 0x0010, 0x0110, 0xAB10, 0xFFF0 };
      unsigned v, k;
      for (k = 0; k < sizeof offs / sizeof offs[0]; k++) for (v = 0; v < 256; v++) {
          env_init(&env, &rec); rec.give.off = offs[k]; table[0].offset = offs[k]; rec.write_links = 0; SENT(a); SENT(b); SENT(c);
          table[0].segment_size = sizeof obj_bytes;
          (void)pr_u25_entry5_0411(&env, (uint8_t)v, &a, &b, &c, NULL);
          CHECK(rec.id == (uint16_t)((offs[k] & 0xFF00u) | v));                /* the initializer ran for every offset, mapped or not */
      } }

    /* GetMem fails: A was zeroed and stays nil (written only after the call returns); B, C untouched; the initializer is not called */
    env_init(&env, &rec); rec.fail_at = 1; SENT(a); SENT(b); SENT(c);
    CHECK(pr_u25_entry5_0411(&env, 3, &a, &b, &c, &rep) == PR_U25_E5_CALLBACK && rep.failed_call == PR_U25_E5_CALL_GETMEM && rep.callback_status == 9);
    CHECK(a.off == 0 && a.seg == 0 && b.off == 0xAAAA && c.off == 0xAAAA && rec.n == 1 && !rep.init_called);
    /* GetMem missing */
    env_init(&env, &rec); env.hooks.getmem_af8_023f = NULL; SENT(a); SENT(b); SENT(c);
    CHECK(pr_u25_entry5_0411(&env, 3, &a, &b, &c, &rep) == PR_U25_E5_HOOK && rep.failed_call == PR_U25_E5_CALL_GETMEM && a.off == 0 && !rep.getmem_called && b.off == 0xAAAA);
    /* initializer fails: A holds P, B and C are untouched */
    env_init(&env, &rec); rec.fail_at = 2; SENT(a); SENT(b); SENT(c);
    CHECK(pr_u25_entry5_0411(&env, 3, &a, &b, &c, &rep) == PR_U25_E5_CALLBACK && rep.failed_call == PR_U25_E5_CALL_INIT && a.off == 0x0010 && b.off == 0xAAAA && c.off == 0xAAAA);
    /* initializer missing */
    env_init(&env, &rec); env.hooks.init_u6f_0ec4 = NULL; SENT(a); SENT(b); SENT(c);
    CHECK(pr_u25_entry5_0411(&env, 3, &a, &b, &c, &rep) == PR_U25_E5_HOOK && rep.failed_call == PR_U25_E5_CALL_INIT && a.off == 0x0010 && b.off == 0xAAAA);

    /* a pointer GetMem returned that the host table does not hold: the initializer still ran; B and C untouched */
    env_init(&env, &rec); rec.give.off = 0x0020; SENT(a); SENT(b); SENT(c);
    CHECK(pr_u25_entry5_0411(&env, 3, &a, &b, &c, &rep) == PR_U25_E5_UNMAPPED && rep.init_called && a.off == 0x0020 && b.off == 0xAAAA);
    env_init(&env, &rec); rec.give.seg = 0x3001; SENT(a); SENT(b); SENT(c);
    CHECK(pr_u25_entry5_0411(&env, 3, &a, &b, &c, &rep) == PR_U25_E5_UNMAPPED);

    /* short views: +0xC8..+0xCB outside -> B and C untouched; +0xC8 inside but +0x7F..+0x82 outside is impossible (0x7F < 0xC8), so test +0xCB just outside */
    env_init(&env, &rec); table[0].segment_size = 0x10 + 0xCB; SENT(a); SENT(b); SENT(c);               /* +0xCB is the 4th byte of B: outside */
    CHECK(pr_u25_entry5_0411(&env, 3, &a, &b, &c, &rep) == PR_U25_E5_VIEW && rep.view_field == 0xC8 && b.off == 0xAAAA && c.off == 0xAAAA);
    /* B readable but C's bytes outside: only possible when the object's offset wraps so that +0x7F lands past the view */
    {
        pr_u25_e5_object w; pr_far16 wa, wb, wc;
        memset(big, 0, sizeof big);
        w.segment = big; w.segment_size = 0x00C0; w.segment_id = 0x3100; w.offset = 0xFF80;        /* +0x7F -> 0xFFFF (outside 0xC0); +0xC8 -> 0x0048 (inside) */
        env_init(&env, &rec); rec.write_links = 0; rec.give.off = 0xFF80; rec.give.seg = 0x3100; env.objects = &w; env.object_count = 1; SENT(wa); SENT(wb); SENT(wc);
        g_a = &wa; g_b = &wb; g_c = &wc;
        big[0x0048] = 0x11; big[0x0049] = 0x22; big[0x004A] = 0x33; big[0x004B] = 0x44;
        CHECK(pr_u25_entry5_0411(&env, 3, &wa, &wb, &wc, &rep) == PR_U25_E5_VIEW && rep.view_field == 0x7F);
        CHECK(wb.off == 0x2211 && wb.seg == 0x4433 && wc.off == 0xAAAA);                              /* B was written, C was not; +0xC8 wrapped */
        g_a = &a; g_b = &b; g_c = &c;
    }

    /* A is re-read from memory at each field read (original: les di,[bp+0xE]; les di,es:[di] before B and again before C) */
    {
        static uint8_t obj2[0x200];
        pr_u25_e5_object two[2];
        memset(obj2, 0x77, sizeof obj2);
        obj2[0x10 + 0xC8] = 0xA1; obj2[0x10 + 0xC9] = 0xA2; obj2[0x10 + 0xCA] = 0xA3; obj2[0x10 + 0xCB] = 0xA4;
        obj2[0x10 + 0x7F] = 0xB1; obj2[0x10 + 0x80] = 0xB2; obj2[0x10 + 0x81] = 0xB3; obj2[0x10 + 0x82] = 0xB4;
        /* (1) the initializer replaces A: B and C both come from the object A now names, not from the GetMem result */
        env_init(&env, &rec); rec.write_links = 0;
        two[0] = table[0];
        two[1].segment = obj2; two[1].segment_size = sizeof obj2; two[1].segment_id = 0x3200; two[1].offset = 0x0010;
        env.objects = two; env.object_count = 2;
        env.hooks.init_u6f_0ec4 = h_init_swap; swap_to.off = 0x0010; swap_to.seg = 0x3200;
        SENT(a); SENT(b); SENT(c);
        CHECK(pr_u25_entry5_0411(&env, 3, &a, &b, &c, NULL) == PR_U25_E5_OK && a.seg == 0x3200);
        CHECK(b.off == 0xA2A1 && b.seg == 0xA4A3 && c.off == 0xB2B1 && c.seg == 0xB4B3);
        /* (2) B aliases A: after B is written, A holds B's value, and C is read through THAT pointer (the second object) */
        env_init(&env, &rec); rec.write_links = 0;
        memset(obj_bytes, 0x5A, sizeof obj_bytes);
        obj_bytes[0x10 + 0xC8] = 0x10; obj_bytes[0x10 + 0xC9] = 0x00; obj_bytes[0x10 + 0xCA] = 0x00; obj_bytes[0x10 + 0xCB] = 0x32;    /* +0xC8 -> 3200:0010 */
        env.objects = two; env.object_count = 2;
        SENT(c); a.off = 0; a.seg = 0;
        CHECK(pr_u25_entry5_0411(&env, 3, &a, &a, &c, NULL) == PR_U25_E5_OK && a.off == 0x0010 && a.seg == 0x3200);
        CHECK(c.off == 0xB2B1 && c.seg == 0xB4B3);                                    /* C came from the object B/A now names, not the first object (0x5A bytes) */
        /* (3) the aliased B value names no object: UNMAPPED for C with B (= A) already written */
        env_init(&env, &rec); rec.write_links = 0;
        memset(obj_bytes, 0x5A, sizeof obj_bytes);
        obj_bytes[0x10 + 0xC8] = 0x99; obj_bytes[0x10 + 0xC9] = 0x99; obj_bytes[0x10 + 0xCA] = 0x99; obj_bytes[0x10 + 0xCB] = 0x99;
        SENT(c);
        CHECK(pr_u25_entry5_0411(&env, 3, &a, &a, &c, &rep) == PR_U25_E5_UNMAPPED && a.off == 0x9999 && a.seg == 0x9999 && c.off == 0xAAAA);
        /* without aliasing, a plain B leaves A alone and both reads use the same object */
        env_init(&env, &rec); rec.write_links = 1; SENT(a); SENT(b); SENT(c);
        CHECK(pr_u25_entry5_0411(&env, 3, &a, &b, &c, NULL) == PR_U25_E5_OK && a.off == 0x0010);
    }

    /* argument guards: nothing is touched, A is not zeroed */
    env_init(&env, &rec); SENT(a); SENT(b); SENT(c);
    CHECK(pr_u25_entry5_0411(NULL, 1, &a, &b, &c, &rep) == PR_U25_E5_ARGUMENT && a.off == 0xAAAA);
    CHECK(pr_u25_entry5_0411(&env, 1, NULL, &b, &c, &rep) == PR_U25_E5_ARGUMENT && pr_u25_entry5_0411(&env, 1, &a, NULL, &c, &rep) == PR_U25_E5_ARGUMENT);
    CHECK(pr_u25_entry5_0411(&env, 1, &a, &b, NULL, &rep) == PR_U25_E5_ARGUMENT && rec.n == 0 && a.off == 0xAAAA);
    env.objects = NULL; env.object_count = 2;
    CHECK(pr_u25_entry5_0411(&env, 1, &a, &b, &c, &rep) == PR_U25_E5_ARGUMENT && rec.n == 0 && a.off == 0xAAAA);

    puts("overlay_u25_entry5_0411: ok");
    return 0;
}
