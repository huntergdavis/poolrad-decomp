/* Synthetic parity test for the unit 0x1c +0x0856 PICTURE handler model. Script,
 * windows, DS bytes, string constants and every hook are fabricated. A
 * restatement of the branch structure (a different shape: an expected-call
 * list built from the conditions) is compared with the model over every
 * picture id, both window words, and all DS flag combinations; the failure,
 * missing-hook and core-status paths are checked individually. */
#include "poolrad/recovered/overlay_u1c_picture_0856.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;
static const uint8_t S851[] = { 0 };                   /* cs:0x0851: length 0 */
static const uint8_t S852[] = { 3, 'a', 'b', 'c' };    /* cs:0x0852: length 3 (synthetic text) */

typedef struct {
    int log[16]; unsigned n;
    uint8_t box_args[6]; const uint8_t *box_str;
    const uint8_t *s2a; uint8_t id2a, zero2a; uint16_t off2a;
    uint8_t a25, b25, c25; uint16_t w4_25, w6_25;
    uint8_t id43, byte43;
    int fail_call;                      /* hook number (PR_U1C_PIC_CALL_*) that returns 9; 0 = none */
} Rec;

static int note(Rec *r, int which) { r->log[r->n++] = which; return r->fail_call == which ? 9 : 0; }
static int h_box(void *c, pr_u1c_picture_env *e, const uint8_t a[6], const uint8_t *s)
{ Rec *r = c; (void)e; memcpy(r->box_args, a, 6); r->box_str = s; return note(r, PR_U1C_PIC_CALL_BOX); }
static int h_2a(void *c, pr_u1c_picture_env *e, const uint8_t *s, uint8_t id, uint8_t z, uint16_t off)
{ Rec *r = c; (void)e; r->s2a = s; r->id2a = id; r->zero2a = z; r->off2a = off; return note(r, PR_U1C_PIC_CALL_2A); }
static int h_25(void *c, pr_u1c_picture_env *e, uint8_t a, uint8_t b, uint8_t cc, uint16_t w4, uint16_t w6)
{ Rec *r = c; (void)e; r->a25 = a; r->b25 = b; r->c25 = cc; r->w4_25 = w4; r->w6_25 = w6; return note(r, PR_U1C_PIC_CALL_25); }
static int h_43(void *c, pr_u1c_picture_env *e, uint8_t id, uint8_t b)
{ Rec *r = c; (void)e; r->id43 = id; r->byte43 = b; return note(r, PR_U1C_PIC_CALL_UNIT25_43); }
static int h_view(void *c, pr_u1c_picture_env *e) { Rec *r = c; (void)e; return note(r, PR_U1C_PIC_CALL_VIEW); }

static void setup(uint8_t id, uint16_t w0, uint16_t w1)
{
    static uint8_t script[3];
    unsigned i;
    script[0] = 0x0E; script[1] = 0x00; script[2] = id;
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = sizeof script; st.pc = 0x9900u; st.latch = 0x0E;
    for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) st.window0[i] = 0xAAAAu;
    for (i = 0; i < PR_REC_ECL_WINDOW1_WORDS; ++i) st.window1[i] = 0xAAAAu;
    st.window0[0xE6] = w0; st.window1[0x2E1] = w1;
    st.b84c8 = 7; st.b84c9 = 7; st.b84e0 = 7;
}

static void env_init(pr_u1c_picture_env *e, Rec *r)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    e->str_0851 = S851; e->str_0852 = S852; e->w6ac4 = 0x1357; e->w6ac6 = 0x2468;
    e->hooks.box_3f1_02e8 = h_box; e->hooks.pic_3d0_2a = h_2a; e->hooks.pic_3d0_25 = h_25;
    e->hooks.unit25_43 = h_43; e->hooks.view_3ca_25 = h_view; e->ctx = r;
}

int main(void)
{
    pr_u1c_picture_env env; Rec rec; pr_u1c_picture_report rep;
    unsigned id, w0i, w1i, f9, fa, e0, e3;
    static const uint16_t w0s[] = { 0, 1, 0x100 };
    static const uint16_t w1s[] = { 0x00FF, 0x01FF, 0x0000, 0x0100, 0xFFFF, 0x00FE };

    for (id = 0; id < 256; id++)
        for (w0i = 0; w0i < 3; w0i++)
            for (w1i = 0; w1i < 6; w1i++)
                for (f9 = 0; f9 < 3; f9++)
                    for (fa = 0; fa < 3; fa++)
                        for (e0 = 0; e0 < 2; e0++)
                            for (e3 = 0; e3 < 2; e3++) {
                                int want[8], nw = 0;
                                unsigned exp84c8 = 7, exp84c9 = 7, exp84e0 = 7, exp84e1 = 0, exp84e3 = e3;
                                /* every id sees every window-word value, but only the representative ids (and one id in 16)
                                   see all the DS flag combinations; the others one combination chosen from the id */
                                if (!(id == 0 || id == 1 || id == 0x7F || id == 0xFE || id == 0xFF || (id & 15u) == 5u) &&
                                    !(f9 == id % 3u && fa == (id / 3u) % 3u && e0 == (id & 1u) && e3 == ((id >> 1) & 1u))) continue;
                                setup((uint8_t)id, w0s[w0i], w1s[w1i]);
                                env_init(&env, &rec);
                                env.b49f9 = (uint8_t)f9; env.b49fa = (uint8_t)fa; env.b84e3 = (uint8_t)e3; env.b84e1 = 0;
                                st.b84e0 = (uint8_t)e0; exp84e0 = e0;
                                /* the expected sequence from the conditions */
                                if (w0s[w0i] == 0 && id != 0xFF) want[nw++] = PR_U1C_PIC_CALL_BOX;
                                if (id != 0xFF) {
                                    exp84c9 = 1; exp84e0 = 1;
                                    if (w1s[w1i] == 0x00FF) { want[nw++] = PR_U1C_PIC_CALL_2A; want[nw++] = PR_U1C_PIC_CALL_25; }
                                    else want[nw++] = PR_U1C_PIC_CALL_UNIT25_43;
                                } else {
                                    if ((f9 > 1 || fa == 1) && (e0 != 0 || e3 != 0)) { want[nw++] = PR_U1C_PIC_CALL_VIEW; exp84e0 = 0; exp84e3 = 0; exp84e1 = 1; }
                                    exp84c8 = 0; exp84c9 = 0;
                                }
                                CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_OK);
                                CHECK(rec.n == (unsigned)nw && memcmp(rec.log, want, sizeof(int) * (size_t)nw) == 0);
                                CHECK(rep.id == id && rep.calls_n == (unsigned)nw);
                                CHECK(st.b84c8 == exp84c8 && st.b84c9 == exp84c9 && st.b84e0 == exp84e0 && env.b84e1 == exp84e1 && env.b84e3 == exp84e3);
                                CHECK(st.pc == 0x9903u);                      /* the loader consumed opcode + operand */
                                if (nw && want[0] == PR_U1C_PIC_CALL_BOX) {
                                    static const uint8_t ba[6] = { 3, 3, 0x0D, 0x0D, 0, 0x0F };
                                    CHECK(memcmp(rec.box_args, ba, 6) == 0 && rec.box_str == S851);
                                }
                                if (id != 0xFF && w1s[w1i] == 0x00FF)
                                    CHECK(rec.s2a == S852 && rec.id2a == id && rec.zero2a == 0 && rec.off2a == 0x6ABE &&
                                          rec.a25 == 3 && rec.b25 == 3 && rec.c25 == 1 && rec.w4_25 == 0x1357 && rec.w6_25 == 0x2468);
                                if (id != 0xFF && w1s[w1i] != 0x00FF)
                                    CHECK(rec.id43 == id && rec.byte43 == (uint8_t)(w1s[w1i] & 0xFF));
                            }

    /* a failing hook stops the handler there: earlier effects retained, later ones not made */
    setup(0x05, 0, 0x00FF); env_init(&env, &rec); rec.fail_call = PR_U1C_PIC_CALL_BOX;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_CALLBACK && rep.callback_status == 9 && rep.failed_call == PR_U1C_PIC_CALL_BOX);
    CHECK(rec.n == 1 && st.b84c9 == 7 && st.b84e0 == 7);                       /* the show-path writes are after the box call */
    setup(0x05, 0, 0x00FF); env_init(&env, &rec); rec.fail_call = PR_U1C_PIC_CALL_2A;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_CALLBACK && rep.failed_call == PR_U1C_PIC_CALL_2A && rec.n == 2);
    CHECK(st.b84c9 == 1 && st.b84e0 == 1);                                      /* retained */
    setup(0x05, 1, 0x00FF); env_init(&env, &rec); rec.fail_call = PR_U1C_PIC_CALL_25;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_CALLBACK && rep.failed_call == PR_U1C_PIC_CALL_25 && rec.n == 2);
    setup(0x05, 1, 0x0001); env_init(&env, &rec); rec.fail_call = PR_U1C_PIC_CALL_UNIT25_43;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_CALLBACK && rep.failed_call == PR_U1C_PIC_CALL_UNIT25_43 && st.b84c9 == 1);
    setup(0xFF, 1, 0); env_init(&env, &rec); env.b49fa = 1; st.b84e0 = 1; st.b84c8 = 5; rec.fail_call = PR_U1C_PIC_CALL_VIEW;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_CALLBACK && rep.failed_call == PR_U1C_PIC_CALL_VIEW);
    CHECK(st.b84e0 == 1 && st.b84c8 == 5 && env.b84e1 == 0);                    /* nothing after the view call was made */

    /* a NULL reached hook: stops before its call; unreached hooks may be NULL */
    setup(0x05, 0, 0x00FF); env_init(&env, &rec); env.hooks.box_3f1_02e8 = NULL;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_HOOK && rec.n == 0 && st.b84c9 == 7);
    setup(0x05, 1, 0x00FF); env_init(&env, &rec); env.hooks.box_3f1_02e8 = NULL; env.hooks.view_3ca_25 = NULL; env.hooks.unit25_43 = NULL;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_OK && rec.n == 2);   /* the box, view and entry-7 hooks are not reached */
    /* hooks are checked at their own call sites: with 0x3D0:0x2A present and 0x3D0:0x25 NULL, the 2A call runs and stays logged */
    setup(0x05, 1, 0x00FF); env_init(&env, &rec); env.hooks.pic_3d0_25 = NULL;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_HOOK && rec.n == 1 && rec.log[0] == PR_U1C_PIC_CALL_2A);
    CHECK(rep.calls_n == 1 && rep.calls[0] == PR_U1C_PIC_CALL_2A && rec.s2a == S852 && rec.id2a == 0x05 && st.b84c9 == 1 && st.b84e0 == 1);
    setup(0x05, 1, 0x00FF); env_init(&env, &rec); env.hooks.pic_3d0_2a = NULL;       /* 2A missing: stops before it; 25 never runs */
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_HOOK && rec.n == 0 && rep.calls_n == 0);
    setup(0x05, 1, 0x00FF); env_init(&env, &rec); env.hooks.pic_3d0_25 = NULL; rec.fail_call = PR_U1C_PIC_CALL_2A;   /* 2A fails first */
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_CALLBACK && rep.failed_call == PR_U1C_PIC_CALL_2A && rec.n == 1);
    setup(0xFF, 1, 0); env_init(&env, &rec); env.b49f9 = 2; st.b84e0 = 1; env.hooks.view_3ca_25 = NULL;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_HOOK && st.b84c8 == 7);

    /* the string constants are checked only where they are copied */
    setup(0x05, 0, 0x0001); env_init(&env, &rec); env.str_0852 = NULL;           /* not reached: the 0x2E1 compare fails */
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_OK);
    setup(0x05, 0, 0x0001); env_init(&env, &rec); env.str_0851 = NULL;
    CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_STRING && rec.n == 0 && st.b84c9 == 7);
    { static const uint8_t long1[] = { 1, 'x' };
      setup(0x05, 0, 0x0001); env_init(&env, &rec); env.str_0851 = long1;       /* would overflow the 1-byte slot onto the id byte */
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_STRING); }
    { static const uint8_t ok3[] = { 3, 'a', 'b', 'c' }, long4[] = { 4, 'a', 'b', 'c', 'd' };
      setup(0x05, 1, 0x00FF); env_init(&env, &rec); env.str_0852 = ok3;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_OK);
      setup(0x05, 1, 0x00FF); env_init(&env, &rec); env.str_0852 = long4;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_STRING && st.b84c9 == 1 && rec.n == 0);    /* the writes before the copy stand */
      setup(0x05, 1, 0x00FF); env_init(&env, &rec); env.str_0852 = NULL;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_STRING); }

    /* core statuses: truncated operand (BOUNDS), a class-4 source (UNRESOLVED): no effect at all; class-1 source: HOOK after completing */
    { static const uint8_t trunc[] = { 0x0E, 0x00 };
      setup(0x05, 0, 0x00FF); env_init(&env, &rec); st.script = trunc; st.script_size = sizeof trunc;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && rec.n == 0 && st.b84c9 == 7); }
    { static const uint8_t c4[] = { 0x0E, 0x01, 0xB2, 0x00 };                    /* 0x00B2: an unsupported class-4 address */
      setup(0x05, 0, 0x00FF); env_init(&env, &rec); st.script = c4; st.script_size = sizeof c4;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && rec.n == 0 && st.b84c9 == 7 && st.b84e0 == 7); }
    { static const uint8_t c4ok[] = { 0x0E, 0x01, 0x4F, 0xC0 };                  /* 0xC04F = [0x6AB1], resolved by the class-4 table */
      setup(0x05, 1, 0x0001); env_init(&env, &rec); st.script = c4ok; st.script_size = sizeof c4ok; st.b6ab1 = 0x42;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_OK && rep.id == 0x42 && rec.id43 == 0x42); }
    { static const uint8_t c1[] = { 0x0E, 0x01, 0x40, 0x6B };                    /* window-1 variable: the load hook model */
      setup(0x05, 1, 0x0001); env_init(&env, &rec); st.script = c1; st.script_size = sizeof c1; st.window1[0x40] = 0x00FF;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status == PR_REC_ECL_WINDOW_HOOK);
      CHECK(rep.id == 0xFF && rep.cleared && st.b84c8 == 0 && st.b84c9 == 0); }   /* the handler ran to completion first */
    /* a word value: only its low byte is the id */
    { static const uint8_t w[] = { 0x0E, 0x02, 0x05, 0x01 };                     /* immediate word 0x0105 */
      setup(0x05, 1, 0x0001); env_init(&env, &rec); st.script = w; st.script_size = sizeof w;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_OK && rep.id == 0x05 && rec.id43 == 0x05); }
    { static const uint8_t w[] = { 0x0E, 0x02, 0xFF, 0x00 };                     /* 0x00FF: none */
      setup(0x05, 1, 0x0001); env_init(&env, &rec); st.script = w; st.script_size = sizeof w;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_OK && rep.cleared); }
    { static const uint8_t w[] = { 0x0E, 0x02, 0xFF, 0x01 };                     /* 0x01FF: low byte 0xFF is still none */
      setup(0x05, 1, 0x0001); env_init(&env, &rec); st.script = w; st.script_size = sizeof w;
      CHECK(pr_u1c_op_picture(&st, &env, &rep) == PR_U1C_PICTURE_OK && rep.cleared && rep.id == 0xFF); }

    CHECK(pr_u1c_op_picture(NULL, &env, &rep) == PR_U1C_PICTURE_ARGUMENT && pr_u1c_op_picture(&st, NULL, &rep) == PR_U1C_PICTURE_ARGUMENT);
    puts("overlay_u1c_picture_0856: ok");
    return 0;
}
