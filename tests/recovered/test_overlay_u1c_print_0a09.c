/* Synthetic test for the unit 0x1c +0x0A09 PRINT/PRINTCLEAR handler model. Scripts, DS bytes,
 * windows and every hook are fabricated. Both opcodes (latch 0x11 and 0x12), the numeric and string
 * operand tags, the call order and arguments, the retained effects at every failing or missing hook,
 * and the core-status paths are checked individually. */
#include "poolrad/recovered/overlay_u1c_print_0a09.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;

typedef struct {
    int log[8]; unsigned n;
    uint16_t num_value;
    uint8_t store_max; uint8_t store_len; uint8_t store_payload[PR_U1C_PRINT_LOCAL_SIZE];   /* length byte and payload only: the model's local does not outlive the call */
    uint8_t print_args[6]; uint8_t print_len; char print_text[8]; const char *print_text_ptr;
    int fail_call;                      /* PR_U1C_PRINT_CALL_* that returns 9; 0 = none */
} Rec;

static int note(Rec *r, int which) { r->log[r->n++] = which; return r->fail_call == which ? 9 : 0; }
static int h_num(void *c, pr_u1c_print_env *e, uint8_t local[PR_U1C_PRINT_LOCAL_SIZE], uint16_t v)
{ Rec *r = c; (void)e; local[0] = 2; local[1] = '4'; local[2] = '2'; r->num_value = v; return note(r, PR_U1C_PRINT_CALL_NUM); }
static int h_store(void *c, pr_u1c_print_env *e, pr_rec_ecl_loader_state *s, uint8_t max, const uint8_t local[PR_U1C_PRINT_LOCAL_SIZE])
{ Rec *r = c; (void)e; r->store_max = max; r->store_len = local[0]; memcpy(r->store_payload, local + 1, local[0]);
  s->string_len[1] = local[0]; memcpy(s->string_text[1], local + 1, local[0]);   /* a synthetic callee that assigns */
  return note(r, PR_U1C_PRINT_CALL_STORE); }
static int h_print(void *c, pr_u1c_print_env *e, const uint8_t a[6], uint8_t len, const char *s)
{ Rec *r = c; (void)e; memcpy(r->print_args, a, 6); r->print_len = len; r->print_text_ptr = s; memcpy(r->print_text, s, len < 7 ? len : 7);
  return note(r, PR_U1C_PRINT_CALL_PRINT); }

static void setup(const uint8_t *script, size_t n, uint8_t latch)
{
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = n; st.pc = 0x9900u; st.latch = latch;
    st.b5e24 = 7; st.b5e25 = 7;
}

static void env_init(pr_u1c_print_env *e, Rec *r)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    e->b84e4 = 5; e->b4a00 = 6;
    e->hooks.num_ba_1073 = h_num; e->hooks.store_af8_0b25 = h_store; e->hooks.print_709_0768 = h_print; e->ctx = r;
}

int main(void)
{
    pr_u1c_print_env env; Rec rec; pr_u1c_print_report rep;
    static const uint8_t str_op[] = { 0x00, 0x80, 0x03, 0x41, 0x82, 0xC3 };      /* string operand: 3 six-bit packed bytes */
    static const uint8_t empty_op[] = { 0x00, 0x80, 0x00 };
    static const uint8_t num_op[] = { 0x00, 0x02, 0x34, 0x12 };                  /* immediate word 0x1234 */
    static const uint8_t c1_op[] = { 0x00, 0x01, 0x40, 0x6B };                   /* window-1 variable: the load hook model */
    static const uint8_t c4_op[] = { 0x00, 0x01, 0xB2, 0x00 };                   /* unsupported class-4 address */
    unsigned latch;
    char want[8]; size_t want_len;
    want_len = pr_rec_ecl_unpack_text(str_op + 3, 3, want, sizeof want);

    /* string tag, both opcodes: no numeric hooks, one print call with buffer 1 */
    for (latch = 0x11; latch <= 0x12; latch++) {
        setup(str_op, sizeof str_op, (uint8_t)latch); env_init(&env, &rec);
        CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_OK);
        CHECK(!rep.numeric_path && !rep.slot_read && rep.calls_n == 1 && rep.calls[0] == PR_U1C_PRINT_CALL_PRINT);
        CHECK(rec.n == 1 && rec.log[0] == PR_U1C_PRINT_CALL_PRINT && rec.print_len == want_len && want_len > 0 && memcmp(rec.print_text, want, want_len) == 0 && rec.print_text_ptr == st.string_text[1]);
        CHECK(rec.print_args[0] == 1 && rec.print_args[1] == 0x11 && rec.print_args[2] == 0x26 && rec.print_args[3] == 0x16 && rec.print_args[4] == 0x0A);
        CHECK(rec.print_args[5] == (latch == 0x11 ? 0 : 1) && rep.print_arm == (latch == 0x11));
        CHECK(env.b84e4 == 0 && env.b4a00 == 0 && st.pc == 0x9906u);
        if (latch == 0x11) CHECK(st.b5e24 == 7 && st.b5e25 == 7);                /* PRINT does not write them */
        else CHECK(st.b5e24 == 1 && st.b5e25 == 0x11);                           /* PRINTCLEAR does */
    }
    /* empty string */
    setup(empty_op, sizeof empty_op, 0x12); env_init(&env, &rec);
    CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_OK && rec.print_len == 0 && rec.print_args[5] == 1 && st.b5e25 == 0x11);

    /* numeric tag, both opcodes: conversion hook, store hook, print hook in order */
    for (latch = 0x11; latch <= 0x12; latch++) {
        setup(num_op, sizeof num_op, (uint8_t)latch); env_init(&env, &rec);
        CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_OK && rep.numeric_path && rep.slot_read && rep.value == 0x1234);
        CHECK(rec.n == 3 && rec.log[0] == PR_U1C_PRINT_CALL_NUM && rec.log[1] == PR_U1C_PRINT_CALL_STORE && rec.log[2] == PR_U1C_PRINT_CALL_PRINT);
        CHECK(rec.num_value == 0x1234 && rec.store_max == 0xFF && rec.print_len == 2 && memcmp(rec.print_text, "42", 2) == 0);
        CHECK(env.b84e4 == 0 && env.b4a00 == 0 && rep.calls_n == 3);
    }

    /* hook B sees only what hook A wrote: a Pascal string at the start of the local; bytes past it are host scratch */
    { static const uint8_t ek[] = { 0x00, 0x02, 0x34, 0x12 };
      unsigned i;
      setup(ek, sizeof ek, 0x12); env_init(&env, &rec); memset(st.string_text[1], 0x5A, sizeof st.string_text[1]);
      CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_OK);
      CHECK(st.string_len[1] == 2 && st.string_text[1][0] == '4' && st.string_text[1][1] == '2');   /* the synthetic hook B copied len + payload */
      for (i = 2; i < PR_REC_ECL_TEXT_MAX; i++) CHECK(st.string_text[1][i] == 0x5A);               /* nothing else of the local reached buffer 1 */
      CHECK(rec.store_len == 2 && rec.store_payload[0] == '4' && rec.store_payload[1] == '2');   /* hook B read the length byte and payload only */
    }

    /* failing hook at each step: earlier effects stay, [0x4A00] is not cleared, later hooks do not run */
    for (latch = 0x11; latch <= 0x12; latch++) {
        int f;
        for (f = PR_U1C_PRINT_CALL_NUM; f <= PR_U1C_PRINT_CALL_PRINT; f++) {
            setup(num_op, sizeof num_op, (uint8_t)latch); env_init(&env, &rec); rec.fail_call = f;
            CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_CALLBACK && rep.callback_status == 9 && rep.failed_call == (pr_u1c_print_call)f);
            CHECK(rec.n == (unsigned)f && env.b84e4 == 0 && env.b4a00 == 1);
            if (f == PR_U1C_PRINT_CALL_PRINT && latch == 0x12) CHECK(st.b5e25 == 0x11 && st.b5e24 == 1);   /* written before the call */
            else CHECK(st.b5e25 == 7 && st.b5e24 == 7);
        }
        /* missing hook at each step */
        setup(num_op, sizeof num_op, (uint8_t)latch); env_init(&env, &rec); env.hooks.num_ba_1073 = NULL;
        CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_HOOK && rec.n == 0 && env.b84e4 == 0 && env.b4a00 == 1);
        setup(num_op, sizeof num_op, (uint8_t)latch); env_init(&env, &rec); env.hooks.store_af8_0b25 = NULL;
        CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_HOOK && rec.n == 1 && env.b4a00 == 1);
        setup(str_op, sizeof str_op, (uint8_t)latch); env_init(&env, &rec); env.hooks.print_709_0768 = NULL;
        CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_HOOK && rec.n == 0 && env.b4a00 == 1 && st.b5e25 == (latch == 0x12 ? 0x11 : 7));
        /* the numeric hooks are not reached for a string operand even when absent */
        setup(str_op, sizeof str_op, (uint8_t)latch); env_init(&env, &rec); env.hooks.num_ba_1073 = NULL; env.hooks.store_af8_0b25 = NULL;
        CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_OK);
    }

    /* core statuses: truncated operand (BOUNDS) stops before the DS byte writes; a class-4 source (UNRESOLVED) stops after them and before any hook */
    { static const uint8_t trunc[] = { 0x00, 0x80 };
      setup(trunc, sizeof trunc, 0x11); env_init(&env, &rec);
      CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && rec.n == 0 && env.b84e4 == 5 && env.b4a00 == 6); }
    setup(c4_op, sizeof c4_op, 0x12); env_init(&env, &rec);
    CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && rec.n == 0);
    CHECK(env.b84e4 == 0 && env.b4a00 == 1 && st.b5e25 == 7 && st.b5e24 == 7);
    /* class-1 slot read: the handler completes, then reports HOOK */
    for (latch = 0x11; latch <= 0x12; latch++) {
        setup(c1_op, sizeof c1_op, (uint8_t)latch); env_init(&env, &rec); st.window1[0x40] = 0x0099;
        CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status == PR_REC_ECL_WINDOW_HOOK);
        CHECK(rec.n == 3 && rep.value == 0x0099 && env.b4a00 == 0 && env.b84e4 == 0);
    }
    /* a failing hook wins over the pending slot HOOK */
    setup(c1_op, sizeof c1_op, 0x11); env_init(&env, &rec); rec.fail_call = PR_U1C_PRINT_CALL_STORE;
    CHECK(pr_u1c_op_print(&st, &env, &rep) == PR_U1C_PRINT_CALLBACK && rec.n == 2 && env.b4a00 == 1);

    CHECK(pr_u1c_op_print(NULL, &env, &rep) == PR_U1C_PRINT_ARGUMENT && pr_u1c_op_print(&st, NULL, &rep) == PR_U1C_PRINT_ARGUMENT);
    setup(str_op, sizeof str_op, 0x11); env_init(&env, &rec);
    CHECK(pr_u1c_op_print(&st, &env, NULL) == PR_U1C_PRINT_OK);                /* NULL report is allowed */
    puts("overlay_u1c_print_0a09: ok");
    return 0;
}
