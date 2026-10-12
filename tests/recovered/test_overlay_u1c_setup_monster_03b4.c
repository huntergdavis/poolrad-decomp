/* Synthetic test for the unit 0x1c +0x03B4 SETUP MONSTER handler model. Scripts, state and both hooks are fabricated.
 * The two window words are compared with an independent restatement (w582 = min(w580, cell), unsigned) over boundary
 * values; the DS bytes, the full-word slot 2, the direct (hook-free) window writes, hook order and arguments, failing and
 * missing hooks with retained partial effects, slot failures at each of the three reads, and loader failure are checked
 * individually. */
#include "poolrad/recovered/overlay_u1c_setup_monster_03b4.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;

typedef struct {
    int log[4]; unsigned n;
    uint8_t x, y, f; uint8_t cell;           /* 0x3E: arguments seen and the result to give */
    uint8_t a, b, c; pr_rec_ecl_loader_state *state;   /* 0x48 arguments seen */
    int fail_call;
} Rec;

static int h_0482(void *c, pr_u1c_setup_monster_env *e, uint8_t x, uint8_t y, uint8_t f, uint8_t *al)
{ Rec *r = c; (void)e; r->x = x; r->y = y; r->f = f; *al = r->cell; r->log[r->n++] = PR_U1C_SMON_CALL_0482; return r->fail_call == 1 ? 9 : 0; }
static int h_054e(void *c, pr_u1c_setup_monster_env *e, pr_rec_ecl_loader_state *s, uint8_t a, uint8_t b, uint8_t cc)
{ Rec *r = c; (void)e; r->state = s; r->a = a; r->b = b; r->c = cc; r->log[r->n++] = PR_U1C_SMON_CALL_054E; return r->fail_call == 2 ? 9 : 0; }

/* opcode, slot 1 = word immediate (tag 2), slot 2 = word immediate (tag 2), slot 3 = word immediate (tag 2) */
static size_t setup(uint16_t s1, uint16_t s2, uint16_t s3)
{
    static uint8_t script[10];
    script[0] = 0x0C;
    script[1] = 0x02; script[2] = (uint8_t)(s1 & 0xFF); script[3] = (uint8_t)(s1 >> 8);
    script[4] = 0x02; script[5] = (uint8_t)(s2 & 0xFF); script[6] = (uint8_t)(s2 >> 8);
    script[7] = 0x02; script[8] = (uint8_t)(s3 & 0xFF); script[9] = (uint8_t)(s3 >> 8);
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = sizeof script; st.pc = 0x9900u; st.latch = 0x0C;
    st.b6aad = 0x11; st.b6aae = 0x22; st.b6aaf = 0x33;
    st.window1[0x2C0] = 0xAAAA; st.window1[0x2C1] = 0xBBBB;
    return sizeof script;
}

static void env_init(pr_u1c_setup_monster_env *e, Rec *r, uint8_t cell)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    r->cell = cell;
    e->hooks.unit25_0482 = h_0482; e->hooks.unit25_054e = h_054e; e->ctx = r;
    e->b6f81 = 0xEE; e->b6f82 = 0xEE;
}

int main(void)
{
    static const uint16_t w2s[] = { 0, 1, 5, 0x7F, 0xFF, 0x100, 0x1234, 0x8000, 0xFFFF };
    static const unsigned cells[] = { 0, 1, 5, 0x7F, 0xFE, 0xFF };
    unsigned i, j;
    pr_u1c_setup_monster_env env; Rec rec; pr_u1c_smon_report rep;

    for (i = 0; i < sizeof w2s / sizeof w2s[0]; i++) for (j = 0; j < sizeof cells / sizeof cells[0]; j++) {
        uint16_t w = w2s[i]; unsigned cell = cells[j];
        unsigned want = w < cell ? w : cell;                              /* independent restatement of the unsigned minimum */
        setup(0x1234, w, 0xAB56); env_init(&env, &rec, (uint8_t)cell);
        CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_OK);
        CHECK(env.b6f81 == 0x34 && env.b6f82 == 0x56 && st.window1[0x2C0] == w && st.window1[0x2C1] == want);   /* DS low bytes; the full word */
        CHECK(rep.cell == cell && rep.w580 == w && rep.w582 == want && rep.lowered == (w < cell) && rep.s2 == w);
        CHECK(rec.n == 2 && rec.log[0] == PR_U1C_SMON_CALL_0482 && rec.log[1] == PR_U1C_SMON_CALL_054E);
        CHECK(rec.x == 0x11 && rec.y == 0x22 && rec.f == 0x33 && rec.a == 0x34 && rec.b == 0x56 && rec.c == (uint8_t)want && rec.state == &st);
        CHECK(rep.calls_n == 2 && st.pc == 0x990Au);
    }

    /* failing hooks keep the earlier effects: the DS bytes and w580 stand; a failing 0x3E leaves w582 as it was */
    setup(0x1234, 0x0007, 0xAB56); env_init(&env, &rec, 3); rec.fail_call = 1;
    CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_CALLBACK && rep.callback_status == 9 && rep.failed_call == PR_U1C_SMON_CALL_0482 && rec.n == 1);
    CHECK(env.b6f81 == 0x34 && env.b6f82 == 0x56 && st.window1[0x2C0] == 7 && st.window1[0x2C1] == 0xBBBB);
    setup(0x1234, 0x0007, 0xAB56); env_init(&env, &rec, 3); rec.fail_call = 2;
    CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_CALLBACK && rep.failed_call == PR_U1C_SMON_CALL_054E && rec.n == 2);
    CHECK(st.window1[0x2C0] == 7 && st.window1[0x2C1] == 3);                    /* w582 is already min(7, 3) */
    /* missing hooks: HOOK status, same retained effects */
    setup(0x1234, 0x0007, 0xAB56); env_init(&env, &rec, 3); env.hooks.unit25_0482 = NULL;
    CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_HOOK && rec.n == 0 && st.window1[0x2C0] == 7 && st.window1[0x2C1] == 0xBBBB && env.b6f82 == 0x56);
    setup(0x1234, 0x0007, 0xAB56); env_init(&env, &rec, 3); env.hooks.unit25_054e = NULL;
    CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_HOOK && rec.n == 1 && st.window1[0x2C1] == 3);

    /* slot value failures at each read: earlier effects stand, later ones are not made, no hook runs */
    { static const uint8_t c4a[] = { 0x0C, 0x01, 0xB2, 0x00, 0x02, 0x07, 0x00, 0x02, 0x56, 0x00 };   /* slot 1: unsupported class-4 */
      setup(0, 0, 0); st.script = c4a; env_init(&env, &rec, 3);
      CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && env.b6f81 == 0xEE && st.window1[0x2C0] == 0xAAAA && rec.n == 0); }
    { static const uint8_t c4b[] = { 0x0C, 0x02, 0x34, 0x12, 0x01, 0xB2, 0x00, 0x02, 0x56, 0x00 };   /* slot 2 */
      setup(0, 0, 0); st.script = c4b; env_init(&env, &rec, 3);
      CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_CORE && env.b6f81 == 0x34 && st.window1[0x2C0] == 0xAAAA && env.b6f82 == 0xEE && rec.n == 0); }
    { static const uint8_t c4c[] = { 0x0C, 0x02, 0x34, 0x12, 0x02, 0x07, 0x00, 0x01, 0xB2, 0x00 };   /* slot 3 */
      setup(0, 0, 0); st.script = c4c; env_init(&env, &rec, 3);
      CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_CORE && env.b6f81 == 0x34 && st.window1[0x2C0] == 7 && env.b6f82 == 0xEE && rec.n == 0); }
    /* a class-1 slot read: the handler completes (hooks run), then reports HOOK; 0x6E80 is a plain load (OK), 0x6B18 an effect address */
    { static const uint8_t c1[] = { 0x0C, 0x02, 0x00, 0x00, 0x02, 0x07, 0x00, 0x01, 0x18, 0x6B };
      setup(0, 0, 0); st.script = c1; st.window1[0x18] = 0x0042; env_init(&env, &rec, 9);
      CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status[2] == PR_REC_ECL_WINDOW_HOOK);
      CHECK(env.b6f82 == 0x42 && rec.n == 2 && st.window1[0x2C1] == 7); }
    { static const uint8_t pl[] = { 0x0C, 0x02, 0x00, 0x00, 0x02, 0x07, 0x00, 0x01, 0x80, 0x6E };
      setup(0, 0, 0); st.script = pl; st.window1[0x380] = 0x0005; env_init(&env, &rec, 9);
      CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_OK && env.b6f82 == 5); }

    /* the window words are written directly: no store status, even at addresses whose store hook would report HOOK */
    setup(1, 2, 3); env_init(&env, &rec, 200);
    CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_OK && st.window1[0x2C0] == 2 && st.window1[0x2C1] == 2);

    /* loader failure: nothing else happens */
    { static const uint8_t trunc[] = { 0x0C, 0x02, 0x01, 0x00 };
      setup(0, 0, 0); st.script = trunc; st.script_size = sizeof trunc; env_init(&env, &rec, 3);
      CHECK(pr_u1c_op_setup_monster(&st, &env, &rep) == PR_U1C_SMON_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && env.b6f81 == 0xEE && rec.n == 0); }

    CHECK(pr_u1c_op_setup_monster(NULL, &env, &rep) == PR_U1C_SMON_ARGUMENT && pr_u1c_op_setup_monster(&st, NULL, &rep) == PR_U1C_SMON_ARGUMENT);
    setup(1, 2, 3); env_init(&env, &rec, 3);
    CHECK(pr_u1c_op_setup_monster(&st, &env, NULL) == PR_U1C_SMON_OK);
    puts("overlay_u1c_setup_monster_03b4: ok");
    return 0;
}
