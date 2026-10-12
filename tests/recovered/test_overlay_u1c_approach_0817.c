/* Synthetic test for the unit 0x1c +0x0817 APPROACH handler model. State and the hook are fabricated. For every
 * boundary value of the window-1 word w582 the decrement, the hook call (arguments, only when w582 <> 0), the PC
 * advance in both branches, and the retained decrement on a missing or failing hook are checked, including the
 * composition with the SETUP MONSTER model that produces w582. */
#include "poolrad/recovered/overlay_u1c_approach_0817.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;

typedef struct { unsigned n; uint8_t a, b, c; pr_rec_ecl_loader_state *state; int fail; } Rec;

static int h_054e(void *c, pr_u1c_setup_monster_env *e, pr_rec_ecl_loader_state *s, uint8_t a, uint8_t b, uint8_t cc)
{ Rec *r = c; (void)e; r->n++; r->a = a; r->b = b; r->c = cc; r->state = s; return r->fail ? 9 : 0; }

static void env_init(pr_u1c_setup_monster_env *e, Rec *r, uint8_t b81, uint8_t b82)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    e->b6f81 = b81; e->b6f82 = b82; e->hooks.unit25_054e = h_054e; e->ctx = r;
}

int main(void)
{
    static const uint16_t ws[] = { 0, 1, 2, 3, 0x7F, 0x80, 0xFF, 0x100, 0x101, 0x7FFF, 0x8000, 0xFFFF };
    unsigned i;
    pr_u1c_setup_monster_env env; Rec rec; pr_u1c_approach_report rep;

    for (i = 0; i < sizeof ws / sizeof ws[0]; i++) {
        uint16_t w = ws[i];
        memset(&st, 0, sizeof st); st.pc = 0x9C27u; st.window1[0x2C1] = w; st.window1[0x2C0] = 0x1234; env_init(&env, &rec, 0x5A, 0xA5);
        CHECK(pr_u1c_op_approach(&st, &env, &rep) == PR_U1C_APPROACH_OK);
        CHECK(st.pc == 0x9C28u && st.window1[0x2C0] == 0x1234 && rep.before == w);
        if (w == 0) {
            CHECK(rec.n == 0 && !rep.called && st.window1[0x2C1] == 0 && rep.after == 0);          /* no decrement, no hook, only the PC */
        } else {
            CHECK(rec.n == 1 && rep.called && st.window1[0x2C1] == (uint16_t)(w - 1) && rep.after == (uint16_t)(w - 1));
            CHECK(rec.a == 0x5A && rec.b == 0xA5 && rec.c == (uint8_t)((w - 1) & 0xFF) && rec.state == &st);   /* the low byte of the decremented word */
        }
    }
    /* the PC wraps at 16 bits */
    memset(&st, 0, sizeof st); st.pc = 0xFFFFu; env_init(&env, &rec, 0, 0);
    CHECK(pr_u1c_op_approach(&st, &env, NULL) == PR_U1C_APPROACH_OK && st.pc == 0x0000u);
    /* repeated calls count w582 down to zero, one hook call each, then stop calling */
    memset(&st, 0, sizeof st); st.pc = 0x9900u; st.window1[0x2C1] = 3; env_init(&env, &rec, 1, 2);
    for (i = 0; i < 5; i++) CHECK(pr_u1c_op_approach(&st, &env, &rep) == PR_U1C_APPROACH_OK);
    CHECK(rec.n == 3 && st.window1[0x2C1] == 0 && st.pc == 0x9905u && rec.c == 0);
    /* failing hook: the decrement stands, the PC is not advanced */
    memset(&st, 0, sizeof st); st.pc = 0x9C27u; st.window1[0x2C1] = 5; env_init(&env, &rec, 1, 2); rec.fail = 1;
    CHECK(pr_u1c_op_approach(&st, &env, &rep) == PR_U1C_APPROACH_CALLBACK && rep.callback_status == 9 && rep.called);
    CHECK(st.window1[0x2C1] == 4 && st.pc == 0x9C27u);
    /* missing hook: HOOK status, same retained decrement; with w582 = 0 the hook is not needed */
    memset(&st, 0, sizeof st); st.pc = 0x9C27u; st.window1[0x2C1] = 5; env_init(&env, &rec, 1, 2); env.hooks.unit25_054e = NULL;
    CHECK(pr_u1c_op_approach(&st, &env, &rep) == PR_U1C_APPROACH_HOOK && !rep.called && st.window1[0x2C1] == 4 && st.pc == 0x9C27u);
    memset(&st, 0, sizeof st); st.pc = 0x9C27u; env_init(&env, &rec, 1, 2); env.hooks.unit25_054e = NULL;
    CHECK(pr_u1c_op_approach(&st, &env, &rep) == PR_U1C_APPROACH_OK && st.pc == 0x9C28u);

    /* composition: SETUP MONSTER leaves w582 = min(w580, cell) and the DS bytes; APPROACH then counts it down with the same hook */
    {
        static const uint8_t script[] = { 0x0C, 0x00, 0x07, 0x02, 0x04, 0x00, 0x00, 0x09, 0x00, 0x0D };
        pr_u1c_smon_report srep;
        memset(&st, 0, sizeof st); st.script = script; st.script_size = sizeof script; st.pc = 0x9900u; st.latch = 0x0C;
        env_init(&env, &rec, 0, 0);
        env.hooks.unit25_0482 = NULL;                                        /* a missing 0x3E hook stops SETUP MONSTER before w582 is set */
        CHECK(pr_u1c_op_setup_monster(&st, &env, &srep) == PR_U1C_SMON_HOOK && st.window1[0x2C0] == 4 && env.b6f81 == 7 && env.b6f82 == 9);
        st.window1[0x2C1] = 4;                                               /* as if min(4, cell = 4) were left by the 0x3E hook */
        st.pc = 0x9909u;
        CHECK(pr_u1c_op_approach(&st, &env, &rep) == PR_U1C_APPROACH_OK && rec.n == 1 && rec.a == 7 && rec.b == 9 && rec.c == 3 && st.pc == 0x990Au);
    }

    CHECK(pr_u1c_op_approach(NULL, &env, &rep) == PR_U1C_APPROACH_ARGUMENT && pr_u1c_op_approach(&st, NULL, &rep) == PR_U1C_APPROACH_ARGUMENT);
    puts("overlay_u1c_approach_0817: ok");
    return 0;
}
