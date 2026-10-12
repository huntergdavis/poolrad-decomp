/* Synthetic test for the unit 0x1c +0x0001 EXIT handler model, including an analogous
 * synthetic flow (own PC layout, operand order and immediates; no game script bytes)
 * through the merged handlers to EXIT and the run stop. */
#include "poolrad/recovered/overlay_u1c_exit_0001.h"
#include "poolrad/recovered/overlay_u1c_and_or_0def.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;

typedef struct { unsigned n; uint8_t args[2][6]; const uint8_t *str[2]; int fail_at; } Box;
static int h_box(void *c, pr_u1c_exit_env *e, const uint8_t a[6], const uint8_t *s)
{
    Box *b = c; (void)e;
    if (b->n < 2) { memcpy(b->args[b->n], a, 6); b->str[b->n] = s; }
    b->n++;
    return (b->fail_at && b->n == (unsigned)b->fail_at) ? 5 : 0;
}

static pr_u1c_exit_env *clear_target;
static int h_box_clears_hook(void *c, pr_u1c_exit_env *e, const uint8_t a[6], const uint8_t *s)
{
    Box *b = c; (void)a; (void)s; b->n++;
    e->hooks.box_3f1_02e8 = NULL;              /* the first call removes the hook the second call would use */
    (void)clear_target;
    return 0;
}

static void init(const uint8_t *script, size_t size)
{
    unsigned i;
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = size; st.pc = 0x9900u;
    for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) st.window0[i] = 0xAAAAu;
    for (i = 0; i < PR_REC_ECL_WINDOW1_WORDS; ++i) st.window1[i] = 0xAAAAu;
    for (i = 0; i < PR_REC_ECL_WINDOW2_WORDS; ++i) st.window2[i] = 0xAAAAu;
}

static void env_init(pr_u1c_exit_env *e, Box *b)
{
    memset(e, 0, sizeof *e); memset(b, 0, sizeof *b);
    e->hooks.box_3f1_02e8 = h_box; e->ctx = b;
}

/* ---- the dispatcher table for the composition ---- */
static pr_u1c_exit_env cenv; static Box cbox; static pr_u1c_exit_report crep;
static pr_rec_ecl_handler_status d_save(pr_rec_ecl_loader_state *s, void *c) { (void)c; return pr_rec_ecl_op_save(s, NULL); }
static pr_rec_ecl_handler_status d_cmp(pr_rec_ecl_loader_state *s, void *c) { (void)c; return pr_rec_ecl_op_compare(s); }
static pr_rec_ecl_handler_status d_if(pr_rec_ecl_loader_state *s, void *c) { (void)c; return pr_rec_ecl_op_if(s); }
static pr_rec_ecl_handler_status d_goto(pr_rec_ecl_loader_state *s, void *c) { (void)c; return pr_rec_ecl_op_goto(s); }
static pr_rec_ecl_handler_status d_andor(pr_rec_ecl_loader_state *s, void *c) { (void)c; return pr_u1c_op_and_or(s, NULL, NULL); }
static pr_rec_ecl_handler_status d_exit(pr_rec_ecl_loader_state *s, void *c)
{
    (void)c;
    switch (pr_u1c_op_exit(s, &cenv, &crep)) {
    case PR_U1C_EXIT_OK: return PR_REC_ECL_HANDLER_OK;
    case PR_U1C_EXIT_ARGUMENT: return PR_REC_ECL_HANDLER_ARGUMENT;
    default: return PR_REC_ECL_HANDLER_HOOK;       /* a missing or failing hook is never mapped to OK */
    }
}

int main(void)
{
    static pr_u1c_exit_env env; static Box box; pr_u1c_exit_report rep;
    unsigned i, depth;
    static const uint8_t s0[] = { 0x00 };

    /* ---- the handler alone: [0x84E2] = 0 (no box), empty stack ---- */
    init(s0, sizeof s0); env_init(&env, &box);
    st.pc = 0x9974u; st.b84c8 = 7; st.b84c9 = 8; st.b84e0 = 9; st.b84e2 = 0; st.b442e = 0; st.b5e24 = 0; st.b5e25 = 0;
    env.b84de = 5; env.b4430 = 0; env.w5d92 = 0x1111; env.w5d94 = 0x2222; env.w4436 = 0x3333; env.w4438 = 0x4444;
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_OK);
    CHECK(st.pc == 0x9975u && st.b442e == 1 && st.b84c8 == 0 && st.b84c9 == 0 && st.b84e0 == 0 && st.b84e2 == 0);
    CHECK(st.b5e24 == 1 && st.b5e25 == 0x11 && env.b84de == 0 && box.n == 0 && rep.boxes == 0 && rep.freed_nodes == 0);
    CHECK(env.w5d92 == 0x1111 && env.w5d94 == 0x2222 && env.b4430 == 0 && !rep.restored_character);        /* [0x4430] = 0: no restore */
    /* the PC is a 16-bit word increment */
    init(s0, sizeof s0); env_init(&env, &box); st.pc = 0xFFFFu;
    CHECK(pr_u1c_op_exit(&st, &env, NULL) == PR_U1C_EXIT_OK && st.pc == 0x0000u);

    /* ---- [0x4430] restore of the active-character pointer ---- */
    init(s0, sizeof s0); env_init(&env, &box);
    env.b4430 = 1; env.w5d92 = 0x1111; env.w5d94 = 0x2222; env.w4436 = 0x3333; env.w4438 = 0x4444;
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_OK && env.w5d92 == 0x3333 && env.w5d94 == 0x4444 && env.b4430 == 0 && rep.restored_character);
    CHECK(env.w4436 == 0x3333 && env.w4438 == 0x4444);                                    /* the saved copy is left as is */
    init(s0, sizeof s0); env_init(&env, &box); env.b4430 = 0xFF; env.w4436 = 0xAAAA; env.w4438 = 0xBBBB;
    CHECK(pr_u1c_op_exit(&st, &env, NULL) == PR_U1C_EXIT_OK && env.w5d92 == 0xAAAA && env.w5d94 == 0xBBBB && env.b4430 == 0);   /* any nonzero byte */

    /* ---- call list: empty, one node, many, the array limit ---- */
    for (depth = 0; depth <= PR_REC_ECL_CALL_STACK_MAX; depth += (depth < 3 ? 1 : 30)) {
        init(s0, sizeof s0); env_init(&env, &box);
        for (i = 0; i < depth; i++) st.call_stack[i] = (uint16_t)(0x9A00u + i);
        st.call_depth = depth;
        CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_OK && st.call_depth == 0 && rep.freed_nodes == depth);
        CHECK(st.b442e == 1 && st.pc == 0x9901u);
    }
    init(s0, sizeof s0); env_init(&env, &box); st.call_depth = PR_REC_ECL_CALL_STACK_MAX;
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_OK && rep.freed_nodes == PR_REC_ECL_CALL_STACK_MAX && st.call_depth == 0);

    /* ---- the box call is gated by [0x84E2] and window-0 word 0xE6 ---- */
    init(s0, sizeof s0); env_init(&env, &box); st.b84e2 = 1; st.window0[0xE6] = 0;
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_OK && box.n == 1 && rep.boxes == 1 && st.b84e2 == 0);
    {
        static const uint8_t a1[6] = { 1, 1, 0x0F, 0x0F, 0, 0x0F };
        CHECK(memcmp(box.args[0], a1, 6) == 0 && box.str[0] != NULL && box.str[0][0] == 0);       /* the empty Pascal string */
    }
    init(s0, sizeof s0); env_init(&env, &box); st.b84e2 = 1; st.window0[0xE6] = 0x0100;           /* nonzero as a word */
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_OK && box.n == 2 && rep.boxes == 2);
    {
        static const uint8_t a1[6] = { 1, 1, 0x0F, 0x0F, 0, 0x0F }, a2[6] = { 3, 3, 0x0D, 0x0D, 0, 0x0F };
        CHECK(memcmp(box.args[0], a1, 6) == 0 && memcmp(box.args[1], a2, 6) == 0 && box.str[1][0] == 0);
    }
    init(s0, sizeof s0); env_init(&env, &box); st.b84e2 = 0; st.window0[0xE6] = 0x0001;           /* [0x84E2] = 0 skips both boxes */
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_OK && box.n == 0);
    init(s0, sizeof s0); env_init(&env, &box); st.b84e2 = 0xFF; st.window0[0xE6] = 0;
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_OK && box.n == 1);

    /* ---- fallible box hook: missing (reached), failing first, failing second ---- */
    init(s0, sizeof s0); env_init(&env, &box); env.hooks.box_3f1_02e8 = NULL; st.b84e2 = 1; st.pc = 0x9974u; st.b442e = 0; st.b84c8 = 7; env.b4430 = 1;
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_HOOK && st.pc == 0x9974u && st.b442e == 0 && st.b84c8 == 7 && st.b84e2 == 1 && env.b4430 == 1);   /* nothing done */
    init(s0, sizeof s0); env_init(&env, &box); env.hooks.box_3f1_02e8 = NULL; st.b84e2 = 0;        /* not reached: no hook needed */
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_OK);
    init(s0, sizeof s0); env_init(&env, &box); box.fail_at = 1; st.b84e2 = 1; st.window0[0xE6] = 1; st.pc = 0x9974u; st.b84c8 = 7; env.b4430 = 1; st.call_depth = 2;
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_CALLBACK && rep.callback_status == 5 && rep.failed_box == 1 && box.n == 1);
    CHECK(st.pc == 0x9974u && st.b442e == 0 && st.b84c8 == 7 && st.b84e2 == 1 && env.b4430 == 1 && st.call_depth == 2 && st.b5e24 == 0);   /* nothing after it */
    init(s0, sizeof s0); env_init(&env, &box); box.fail_at = 2; st.b84e2 = 1; st.window0[0xE6] = 1; st.pc = 0x9974u; st.b84c8 = 7; env.b4430 = 1;
    CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_CALLBACK && rep.failed_box == 2 && box.n == 2 && st.pc == 0x9974u && st.b442e == 0 && st.b84c8 == 7 && env.b4430 == 1);

    /* ---- arguments ---- */
    init(s0, sizeof s0); env_init(&env, &box); st.call_depth = PR_REC_ECL_CALL_STACK_MAX + 1u; st.pc = 0x9974u;
    CHECK(pr_u1c_op_exit(&st, &env, NULL) == PR_U1C_EXIT_ARGUMENT && st.pc == 0x9974u && st.b442e == 0);
    CHECK(pr_u1c_op_exit(NULL, &env, NULL) == PR_U1C_EXIT_ARGUMENT && pr_u1c_op_exit(&st, NULL, NULL) == PR_U1C_EXIT_ARGUMENT);

    /* ---- second box hook: the first call may change env->hooks; a missing hook for the second call stops with HOOK and
       keeps the first call's effects (and nothing after it) ---- */
    {
        init(s0, sizeof s0); env_init(&env, &box); st.b84e2 = 1; st.window0[0xE6] = 1; st.pc = 0x9974u; st.b84c8 = 7; env.b4430 = 1; st.call_depth = 2;
        env.hooks.box_3f1_02e8 = h_box_clears_hook; clear_target = &env;
        CHECK(pr_u1c_op_exit(&st, &env, &rep) == PR_U1C_EXIT_HOOK && rep.boxes == 1 && box.n == 1 && env.hooks.box_3f1_02e8 == NULL);
        CHECK(st.pc == 0x9974u && st.b442e == 0 && st.b84c8 == 7 && st.b84e2 == 1 && env.b4430 == 1 && st.call_depth == 2 && st.b5e24 == 0);
    }

    /* ---- an analogous synthetic flow (not the saved entry's bytes: different PCs, operand order, immediates and branch
       targets, an OR in place of the AND), through pr_rec_ecl_run and the merged handlers ----
       0x9A10 COMPARE [0x49D0] with 9; 0x9A16 IF >=; 0x9A17 SAVE 0x77 -> 0x6E7D (skipped); 0x9A1D SAVE 0x21 -> 0x6E7D;
       0x9A23 SAVE [0x6E7D] -> 0x49FE; 0x9A2A SAVE 5 -> 0x49FD; 0x9A30 COMPARE [0x49D4] with 4; 0x9A36 IF <;
       0x9A37 GOTO 0x9A50; 0x9A3B SAVE 1 -> 0x49FB (not reached); 0x9A50 OR 0x30 with [0xC04F] -> 0x6E82; 0x9A59 EXIT */
    {
        static uint8_t script[0x180];
        static pr_rec_ecl_dispatch_table tb;
        unsigned steps = 0;
        pr_rec_ecl_handler_status hs = PR_REC_ECL_HANDLER_OK;
        static const uint8_t q1[] = { 0x03, 0x01, 0xD0, 0x49, 0x00, 0x09 };
        static const uint8_t q2[] = { 0x1B };
        static const uint8_t q3[] = { 0x09, 0x00, 0x77, 0x01, 0x7D, 0x6E };
        static const uint8_t q4[] = { 0x09, 0x00, 0x21, 0x01, 0x7D, 0x6E };
        static const uint8_t q5[] = { 0x09, 0x01, 0x7D, 0x6E, 0x01, 0xFE, 0x49 };
        static const uint8_t q6[] = { 0x09, 0x00, 0x05, 0x01, 0xFD, 0x49 };
        static const uint8_t q7[] = { 0x03, 0x01, 0xD4, 0x49, 0x00, 0x04 };
        static const uint8_t q8[] = { 0x18 };
        static const uint8_t q9[] = { 0x01, 0x02, 0x50, 0x9A };
        static const uint8_t qA[] = { 0x09, 0x00, 0x01, 0x01, 0xFB, 0x49 };
        static const uint8_t qB[] = { 0x30, 0x00, 0x30, 0x01, 0x4F, 0xC0, 0x01, 0x82, 0x6E };
        static const uint8_t qC[] = { 0x00 };
        unsigned v;
        memset(script, 0xEE, sizeof script);                                  /* never-executed filler */
        memcpy(script + 0x110, q1, sizeof q1); memcpy(script + 0x116, q2, sizeof q2); memcpy(script + 0x117, q3, sizeof q3);
        memcpy(script + 0x11D, q4, sizeof q4); memcpy(script + 0x123, q5, sizeof q5); memcpy(script + 0x12A, q6, sizeof q6);
        memcpy(script + 0x130, q7, sizeof q7); memcpy(script + 0x136, q8, sizeof q8); memcpy(script + 0x137, q9, sizeof q9);
        memcpy(script + 0x13B, qA, sizeof qA); memcpy(script + 0x150, qB, sizeof qB); memcpy(script + 0x159, qC, sizeof qC);
        memset(&tb, 0, sizeof tb);
        tb.handler[0x09] = d_save; tb.handler[0x03] = d_cmp; tb.handler[0x01] = d_goto; tb.handler[0x00] = d_exit;
        for (i = 0x16; i <= 0x1B; i++) tb.handler[i] = d_if;
        tb.handler[0x2F] = d_andor; tb.handler[0x30] = d_andor;
        for (v = 0; v < 256; v += 51) {
            init(script, sizeof script); env_init(&cenv, &cbox); memset(&crep, 0, sizeof crep);
            st.window0[0xD0] = 3;                     /* below 9: IF >= fails and skips the SAVE at 0x9A17 */
            st.window0[0xD4] = 2;                     /* below 4: IF < holds, the GOTO is taken */
            st.b6ab1 = (uint8_t)v; st.b84e2 = 0; st.call_depth = 2; st.call_stack[0] = 1; st.call_stack[1] = 2;
            st.window0[0xFD] = 0xAAAA; st.window0[0xFE] = 0xAAAA;
            CHECK(pr_rec_ecl_run(&st, &tb, 0x9A10u, 100, &steps, &hs) == PR_REC_ECL_RUN_OK);
            CHECK(steps == 10 && st.pc == 0x9A5Au && st.latch == 0x00 && st.prev_opcode == 0x30);     /* the exact stop at EXIT */
            CHECK(st.b442e == 0 && st.b49ff == 0);                                  /* [0x442E] cleared again by the run function */
            CHECK(st.window1[0x37D] == 0x0021);                                      /* the SAVE of 0x77 was skipped; 0x6E7D takes OK */
            CHECK(st.window0[0xFE] == 0x0021 && st.window0[0xFD] == 0x0005 && st.b84e8 == 1);   /* both flag addresses, from a load at 0x6E7D */
            CHECK(st.window1[0x382] == (uint16_t)(0x30u | v));                       /* the OR destination: low byte of 0x30 | host [0x6AB1] */
            {
                unsigned r = (0x30u | v) & 0xFFu;
                CHECK(st.cmp_flags[0] == (r == 0) && st.cmp_flags[1] == (r != 0) && st.cmp_flags[2] == (r != 0) &&
                      st.cmp_flags[3] == 0 && st.cmp_flags[4] == 1 && st.cmp_flags[5] == (r == 0));  /* the OR's own compare of 0 with r */
            }
            CHECK(st.call_depth == 0 && crep.freed_nodes == 2 && st.b5e24 == 1 && st.b5e25 == 0x11);
            CHECK(st.window0[0xFB] == 0xAAAA);                                       /* the unreached SAVE did not run */
        }
        /* a false IF < sends the run into the unreachable filler: it never completes at EXIT */
        init(script, sizeof script); env_init(&cenv, &cbox);
        st.window0[0xD0] = 3; st.window0[0xD4] = 9;
        CHECK(pr_rec_ecl_run(&st, &tb, 0x9A10u, 100, &steps, &hs) != PR_REC_ECL_RUN_OK && st.b84e8 == 1 && st.window1[0x382] == 0xAAAA);
        /* [0x84E2] <> 0 with no box hook stops at EXIT with HOOK, nothing else of EXIT done */
        init(script, sizeof script); env_init(&cenv, &cbox); cenv.hooks.box_3f1_02e8 = NULL;
        st.window0[0xD0] = 3; st.window0[0xD4] = 2; st.b84e2 = 1;
        CHECK(pr_rec_ecl_run(&st, &tb, 0x9A10u, 100, &steps, &hs) == PR_REC_ECL_RUN_HANDLER && hs == PR_REC_ECL_HANDLER_HOOK);
        CHECK(steps == 10 && st.pc == 0x9A59u && st.latch == 0x00 && st.b5e24 == 0 && st.b84e8 == 1);
    }

    puts("overlay_u1c_exit_0001: ok");
    return 0;
}
