/* Synthetic test for the unit 0x1c +0x271F DELAY handler model and the resident argument routine 0x709:0x0DBE. The
 * timing hook is fabricated: it records the word it is given and does not wait. Every byte value of [0x49E2] is checked
 * against an independent restatement of the argument (multiplication by 225, no 16-bit overflow), the PC advance before
 * the call, and the missing and failing hook paths. */
#include "poolrad/recovered/overlay_u1c_delay_271f.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;
typedef struct { unsigned n; uint16_t ms; uint16_t pc_at_call; int fail; } Rec;
static int h_delay(void *c, pr_u1c_delay_env *e, uint16_t ms)
{ Rec *r = c; (void)e; r->n++; r->ms = ms; r->pc_at_call = st.pc; return r->fail ? 7 : 0; }

int main(void)
{
    unsigned b;
    pr_u1c_delay_env env; Rec rec; pr_u1c_delay_report rep;

    for (b = 0; b < 256; b++) {
        unsigned want = 0, k;
        for (k = 0; k < 225; k++) want += b;                             /* independent: repeated addition, 225 times */
        CHECK(want < 65536u && pr_res_0709_0dbe_arg((uint8_t)b) == want);
        memset(&st, 0, sizeof st); st.pc = 0x9C28u; st.window0[0x1F8] = 0x1111;
        memset(&env, 0, sizeof env); memset(&rec, 0, sizeof rec);
        env.b49e2 = (uint8_t)b; env.hooks.delay_a40_029c = h_delay; env.ctx = &rec;
        CHECK(pr_u1c_op_delay(&st, &env, &rep) == PR_U1C_DELAY_OK);
        CHECK(rec.n == 1 && rec.ms == want && rep.ms == want && rep.called && st.pc == 0x9C29u);
        CHECK(rec.pc_at_call == 0x9C29u);                                 /* the PC is advanced before the call */
        CHECK(st.window0[0x1F8] == 0x1111 && st.pc == 0x9C29u);          /* no other state is touched */
    }
    CHECK(pr_res_0709_0dbe_arg(255) == 57375u && pr_res_0709_0dbe_arg(0) == 0 && pr_res_0709_0dbe_arg(4) == 900);

    /* PC wrap */
    memset(&st, 0, sizeof st); st.pc = 0xFFFFu; memset(&env, 0, sizeof env); memset(&rec, 0, sizeof rec);
    env.hooks.delay_a40_029c = h_delay; env.ctx = &rec;
    CHECK(pr_u1c_op_delay(&st, &env, NULL) == PR_U1C_DELAY_OK && st.pc == 0 && rec.ms == 0);
    /* failing hook: the PC has already advanced */
    memset(&st, 0, sizeof st); st.pc = 0x9C28u; memset(&rec, 0, sizeof rec); rec.fail = 1; env.b49e2 = 4;
    CHECK(pr_u1c_op_delay(&st, &env, &rep) == PR_U1C_DELAY_CALLBACK && rep.callback_status == 7 && st.pc == 0x9C29u && rep.ms == 900);
    /* missing hook: HOOK status, the PC has advanced, the word is still reported */
    memset(&st, 0, sizeof st); st.pc = 0x9C28u; env.hooks.delay_a40_029c = NULL;
    CHECK(pr_u1c_op_delay(&st, &env, &rep) == PR_U1C_DELAY_HOOK && !rep.called && st.pc == 0x9C29u && rep.ms == 900);

    CHECK(pr_u1c_op_delay(NULL, &env, &rep) == PR_U1C_DELAY_ARGUMENT && pr_u1c_op_delay(&st, NULL, &rep) == PR_U1C_DELAY_ARGUMENT);
    puts("overlay_u1c_delay_271f: ok");
    return 0;
}
