/* Synthetic test for the unit 0x1c +0x16C9 SURPRISE handler model. Scripts, state and the roll hook are fabricated;
 * the hook returns supplied values, it is not a dice model. The stored value is compared with an independent restatement
 * (r = 2 if y <= b; else 1 if x <= a; else 0, in signed arithmetic) over boundary bytes for the four slot values and the
 * two rolls, including the 8-bit wrap of the two derived bytes and the signed compare of the sign-extended byte against
 * the zero-extended roll. Hook order and arguments, failure and missing hooks, slot and store paths are checked individually. */
#include "poolrad/recovered/overlay_u1c_surprise_16c9.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;

typedef struct {
    uint8_t out[2]; unsigned n;
    uint8_t count[2], range[2];
    int fail_at;                         /* 1-based call that returns 9; 0 = none */
} Rec;

static int h_roll(void *c, pr_u1c_surprise_env *e, uint8_t count, uint8_t range, uint8_t *al)
{
    Rec *r = c; (void)e;
    if (r->n < 2) { r->count[r->n] = count; r->range[r->n] = range; *al = r->out[r->n]; }
    ++r->n;
    return (int)r->n == r->fail_at ? 9 : 0;
}

/* opcode, four literal byte operands (tag 0) */
static void setup(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4)
{
    static uint8_t script[9];
    script[0] = 0x23;
    script[1] = 0x00; script[2] = s1; script[3] = 0x00; script[4] = s2;
    script[5] = 0x00; script[6] = s3; script[7] = 0x00; script[8] = s4;
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = sizeof script; st.pc = 0x9900u; st.latch = 0x23;
    st.window1[0x2CB] = 0xAAAA;
}

static void env_init(pr_u1c_surprise_env *e, Rec *r, uint8_t x, uint8_t y)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    r->out[0] = x; r->out[1] = y;
    e->hooks.roll_b0_0048 = h_roll; e->ctx = r;
}

static int sb(unsigned v) { v &= 0xFF; return v < 0x80 ? (int)v : (int)v - 256; }

int main(void)
{
    static const unsigned sv[] = { 0, 1, 2, 3, 0x7E, 0x7F, 0x80, 0x81, 0xFD, 0xFE, 0xFF };
    static const unsigned rv[] = { 0, 1, 2, 5, 6, 7, 0x7F, 0x80, 0xFE, 0xFF };
    const unsigned nsv = sizeof sv / sizeof sv[0], nrv = sizeof rv / sizeof rv[0];
    unsigned i1, i2, i3, i4, ix, iy;
    pr_u1c_surprise_env env; Rec rec; pr_u1c_surprise_report rep;
    unsigned seen[4] = { 0, 0, 0, 0 };

    for (i1 = 0; i1 < nsv; i1++) for (i2 = 0; i2 < nsv; i2++) for (i3 = 0; i3 < nsv; i3++) for (i4 = 0; i4 < nsv; i4++)
    for (ix = 0; ix < nrv; ix++) for (iy = 0; iy < nrv; iy++) {
        unsigned s1 = sv[i1], s2 = sv[i2], s3 = sv[i3], s4 = sv[i4], x = rv[ix], y = rv[iy];
        int a = sb(s4 + 2 - s1), b = sb(s2 + 2 - s3);                      /* low byte of the 16-bit sum, sign-extended */
        unsigned want = (int)y <= b ? 2u : ((int)x <= a ? 1u : 0u);
        setup((uint8_t)s1, (uint8_t)s2, (uint8_t)s3, (uint8_t)s4); env_init(&env, &rec, (uint8_t)x, (uint8_t)y);
        CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_OK);
        CHECK(rep.result == want && st.window1[0x2CB] == want && rep.a == (uint8_t)a && rep.b == (uint8_t)b && rep.x == x && rep.y == y);
        CHECK(rec.n == 2 && rec.count[0] == 1 && rec.count[1] == 1 && rec.range[0] == 6 && rec.range[1] == 6);
        CHECK(rep.s[0] == s1 && rep.s[1] == s2 && rep.s[2] == s3 && rep.s[3] == s4 && rep.stored && st.pc == 0x9909u);
        seen[want]++;
    }
    CHECK(seen[0] > 0 && seen[1] > 0 && seen[2] > 0 && seen[3] == 0);       /* 3 is never stored */

    /* named cases: the derived bytes wrap at 8 bits and compare as signed */
    setup(0xFF, 0, 0, 0); env_init(&env, &rec, 0, 0);                         /* a = 0 + 2 - 255 = -253 -> low byte 3; b = 0 + 2 - 0 = 2 */
    CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_OK && rep.a == 3 && rep.b == 2 && rep.result == 2);
    setup(0, 0, 0, 0xFE); env_init(&env, &rec, 0, 3);                         /* a = 0xFE + 2 = 0x100 -> 0; b = 2; y = 3 > 2, x = 0 <= 0 -> 1 */
    CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_OK && rep.a == 0 && rep.b == 2 && rep.result == 1);
    setup(0, 0, 0, 0x7F); env_init(&env, &rec, 0xFF, 3);                      /* a = 0x81 -> -127 signed; x = 255 > -127: first block skipped; y = 3 > 2 -> 0 */
    CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_OK && rep.a == 0x81 && rep.result == 0);

    /* hook order and failure: the failing call stops, nothing is stored; a missing hook reports HOOK */
    setup(1, 2, 3, 4); env_init(&env, &rec, 1, 1); rec.fail_at = 1;
    CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_CALLBACK && rep.callback_status == 9 && rec.n == 1 && !rep.stored && st.window1[0x2CB] == 0xAAAA);
    setup(1, 2, 3, 4); env_init(&env, &rec, 1, 1); rec.fail_at = 2;
    CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_CALLBACK && rec.n == 2 && rep.rolls == 2 && !rep.stored);
    setup(1, 2, 3, 4); env_init(&env, &rec, 1, 1); env.hooks.roll_b0_0048 = NULL;
    CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_HOOK && rep.rolls == 0 && !rep.stored);

    /* the store goes to 0x6DCB, a quiet class-1 address: OK, and the word is written */
    setup(1, 2, 3, 4); env_init(&env, &rec, 0, 0);
    CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_OK && rep.store_status == PR_REC_ECL_WINDOW_OK && st.window1[0x2CB] == rep.result);

    /* loader failure and slot value failures: before any roll; a class-1 slot read completes then HOOKs; word and variable operands */
    { static const uint8_t trunc[] = { 0x23, 0x00, 0x01, 0x00 };
      memset(&st, 0, sizeof st); st.script = trunc; st.script_size = sizeof trunc; st.pc = 0x9900u; st.latch = 0x23; env_init(&env, &rec, 0, 0);
      CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && rec.n == 0 && !rep.stored); }
    { static const uint8_t c4[] = { 0x23, 0x00, 0x01, 0x00, 0x02, 0x01, 0xB2, 0x00, 0x00, 0x04 };
      memset(&st, 0, sizeof st); st.script = c4; st.script_size = sizeof c4; st.pc = 0x9900u; st.latch = 0x23; env_init(&env, &rec, 0, 0);
      CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && rec.n == 0 && !rep.stored); }
    { static const uint8_t c1[] = { 0x23, 0x01, 0x18, 0x6B, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04 };
      memset(&st, 0, sizeof st); st.script = c1; st.script_size = sizeof c1; st.pc = 0x9900u; st.latch = 0x23; st.window1[0x18] = 0x0105; env_init(&env, &rec, 9, 9);
      CHECK(pr_u1c_op_surprise(&st, &env, &rep) == PR_U1C_SURPRISE_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status[0] == PR_REC_ECL_WINDOW_HOOK);
      CHECK(rep.s[0] == 5 && rec.n == 2 && rep.stored); }                    /* the word value's low byte only */

    CHECK(pr_u1c_op_surprise(NULL, &env, &rep) == PR_U1C_SURPRISE_ARGUMENT && pr_u1c_op_surprise(&st, NULL, &rep) == PR_U1C_SURPRISE_ARGUMENT);
    setup(1, 2, 3, 4); env_init(&env, &rec, 0, 0);
    CHECK(pr_u1c_op_surprise(&st, &env, NULL) == PR_U1C_SURPRISE_OK);
    puts("overlay_u1c_surprise_16c9: ok");
    return 0;
}
