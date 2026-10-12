/* Synthetic test for the LOAD MONSTER +0x04AF..+0x0536 slice model. Script, object bytes, constant string and the 0x3E9:0x34 hook are
 * fabricated. Checked: the 0x11D-byte record copy (incl. 16-bit source wrap and the UNMAPPED/VIEW stops before any other effect), the
 * saved list heads, count 0 -> 1 (unsigned compare: 0x80..0xFF kept), slot 3's low byte, the HOOK stops of a class-1 load-hook address
 * at slot 2/3 (the hook is not called), the StrStore truncation to 12, the exact hook arguments, callback failure, and argument guards. */
#include "poolrad/recovered/overlay_u1c_load_monster_04af.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;
static uint8_t seg[0x10000];
static pr_u25_e5_object table[1];
static pr_u1c_lm04af_env env;
static uint8_t script[16];
static const uint8_t cstr[] = { 4, 'a', 'b', 'c', 'd' };

typedef struct { unsigned n; uint8_t str[16]; uint8_t p3, b; int fail; } Rec;
static Rec rec;

static int h_call(void *c, const pr_u1c_lm04af_env *e, const uint8_t *s, uint8_t p3, uint8_t b)
{
    Rec *r = c; (void)e;
    r->n++; memcpy(r->str, s, 1u + s[0]); r->p3 = p3; r->b = b;
    return r->fail;
}

/* opcode, slot 1 literal 0, then slot 2 and slot 3 raw operands; the loader(3) of the prefix has already run */
static void setup(const uint8_t *s2, size_t n2, const uint8_t *s3, size_t n3)
{
    size_t o = 0;
    script[o++] = 0x0B; script[o++] = 0x00; script[o++] = 0x00;
    memcpy(script + o, s2, n2); o += n2;
    memcpy(script + o, s3, n3); o += n3;
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = o; st.pc = 0x9900u; st.latch = 0x0B;
    (void)pr_rec_ecl_load_operands(&st, 3u);
    memset(seg, 0, sizeof seg);
    table[0].segment = seg; table[0].segment_size = sizeof seg; table[0].segment_id = 0x3000; table[0].offset = 0x0010;
    memset(&rec, 0, sizeof rec);
    memset(&env, 0, sizeof env);
    env.objects = table; env.object_count = 1; env.str_044b = cstr;
    env.w5d96 = 0x4321; env.w5d98 = 0x8765; env.b6f83 = 0x5C;
    env.hooks.unit3ce_01c8 = h_call; env.ctx = &rec;
}

int main(void)
{
    static const uint8_t l5[] = { 0x00, 0x05 }, l0[] = { 0x00, 0x00 }, l9[] = { 0x00, 0x09 }, l80[] = { 0x00, 0x80 };
    static const uint8_t hook[] = { 0x01, 0x18, 0x6B };      /* class-1 load-hook EFFECT address */
    pr_far16 a = { 0x0010, 0x3000 }, b = { 0x1111, 0x2222 }, c = { 0x3333, 0x4444 };
    pr_u1c_lm04af_report rep;
    unsigned i;

    /* normal: record copy, saved heads, count, p3, hook arguments, stop at the return */
    setup(l5, sizeof l5, l9, sizeof l9);
    for (i = 0; i < 0x11D; i++) seg[0x10 + i] = (uint8_t)(i * 7 + 1);
    seg[0x10 + 0x11D] = 0xEE;                                  /* one past the record must not be copied */
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_OK);
    CHECK(rep.moved == 1 && memcmp(rep.record_copy, seg + 0x10, 0x11D) == 0);
    CHECK(rep.saved_b.off == 0x1111 && rep.saved_b.seg == 0x2222 && rep.saved_c.off == 0x3333 && rep.saved_c.seg == 0x4444);
    CHECK(rep.count == 5 && rep.count_raw == 5 && rep.p3 == 9);
    CHECK(rep.party_head.off == 0x4321 && rep.party_head.seg == 0x8765);
    CHECK(rec.n == 1 && rec.p3 == 9 && rec.b == 0x5C && rec.str[0] == 4 && memcmp(rec.str + 1, "abcd", 4) == 0 && rep.hook_called == 1);

    /* count 0 -> 1; 0x80 is kept (unsigned compare) */
    setup(l0, sizeof l0, l9, sizeof l9);
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_OK && rep.count == 1 && rep.count_raw == 0);
    setup(l80, sizeof l80, l9, sizeof l9);
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_OK && rep.count == 0x80);

    /* the Move source wraps at 16 bits */
    setup(l5, sizeof l5, l9, sizeof l9);
    table[0].offset = 0xFFF0; a.off = 0xFFF0;
    for (i = 0; i < 0x11D; i++) seg[(uint16_t)(0xFFF0 + i)] = (uint8_t)(i ^ 0x5A);
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_OK && rep.record_copy[0x10] == (uint8_t)(0x10 ^ 0x5A) && rep.record_copy[0x11] == (uint8_t)(0x11 ^ 0x5A));
    a.off = 0x0010;

    /* UNMAPPED / VIEW stop before any other effect */
    setup(l5, sizeof l5, l9, sizeof l9);
    a.off = 0x0020;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_UNMAPPED && !rep.moved && rec.n == 0 && rep.saved_b.off == 0 && rep.count == 0);
    a.off = 0x0010; table[0].segment_size = 0x10 + 0x11C;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_VIEW && !rep.moved && rec.n == 0);
    table[0].segment = NULL;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_VIEW);

    /* class-1 load-hook address at slot 2 / slot 3: HOOK stop, callee not called */
    setup(hook, sizeof hook, l9, sizeof l9); st.window1[0x18] = 4;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status[0] == PR_REC_ECL_WINDOW_HOOK && rec.n == 0 && rep.moved);
    setup(l5, sizeof l5, hook, sizeof hook); st.window1[0x18] = 4;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_CORE && rep.slot_status[1] == PR_REC_ECL_WINDOW_HOOK && rep.count == 5 && rec.n == 0);

    /* StrStore truncates to 12; NULL constant; NULL hook; callback failure */
    { static const uint8_t longs[] = { 20, 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T' };
      setup(l5, sizeof l5, l9, sizeof l9); env.str_044b = longs;
      CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_OK && rep.local_string[0] == 12 && memcmp(rep.local_string + 1, "ABCDEFGHIJKL", 12) == 0 && rec.str[0] == 12); }
    setup(l5, sizeof l5, l9, sizeof l9); env.str_044b = NULL;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_STRING && rec.n == 0);
    setup(l5, sizeof l5, l9, sizeof l9); env.hooks.unit3ce_01c8 = NULL;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_HOOK && !rep.hook_called);
    setup(l5, sizeof l5, l9, sizeof l9); rec.fail = 7;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_CALLBACK && rep.callback_status == 7 && rep.hook_called);
    setup(l5, sizeof l5, l9, sizeof l9); rec.fail = -3;                                     /* negative status is preserved */
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_CALLBACK && rep.callback_status == -3 && rep.hook_called);

    /* argument guards; NULL report is accepted */
    setup(l5, sizeof l5, l9, sizeof l9);
    CHECK(pr_u1c_load_monster_04af(NULL, &env, a, b, c, &rep) == PR_U1C_LM04AF_ARGUMENT && pr_u1c_load_monster_04af(&st, NULL, a, b, c, &rep) == PR_U1C_LM04AF_ARGUMENT);
    env.objects = NULL; env.object_count = 1;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, &rep) == PR_U1C_LM04AF_ARGUMENT);
    env.objects = table; env.object_count = 1;
    CHECK(pr_u1c_load_monster_04af(&st, &env, a, b, c, NULL) == PR_U1C_LM04AF_OK);
    puts("overlay_u1c_load_monster_04af: ok");
    return 0;
}
