/* Synthetic test for the unit 0x1c +0x0234 RANDOM handler model. Script, windows and the three runtime hooks are
 * fabricated; the hook "result" is a made-up function of the argument, not a generator. Every slot byte 0..255 is
 * checked for the saturating increment, the hook order and arguments, the store of the zero-extended AL, and
 * failure/missing hooks at each step with nothing stored, plus the core-status paths. */
#include "poolrad/recovered/overlay_u1c_random_0234.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;

typedef struct {
    int log[4]; unsigned n;
    uint16_t arg_11c0, dx_119a;
    uint8_t al;                         /* what the fabricated 0x119E hook reports */
    int fail_call;                      /* PR_U1C_RANDOM_CALL_* that returns 9; 0 = none */
} Rec;

static int note(Rec *r, int which) { r->log[r->n++] = which; return r->fail_call == which ? 9 : 0; }
static int h_11c0(void *c, pr_u1c_random_env *e, uint16_t a) { Rec *r = c; (void)e; r->arg_11c0 = a; return note(r, PR_U1C_RANDOM_CALL_11C0); }
static int h_119a(void *c, pr_u1c_random_env *e, uint16_t dx) { Rec *r = c; (void)e; r->dx_119a = dx; return note(r, PR_U1C_RANDOM_CALL_119A); }
static int h_119e(void *c, pr_u1c_random_env *e, uint8_t *al) { Rec *r = c; (void)e; *al = r->al; return note(r, PR_U1C_RANDOM_CALL_119E); }

static void setup(uint8_t v, uint16_t dest)
{
    static uint8_t script[6];
    script[0] = 0x08; script[1] = 0x00; script[2] = v;
    script[3] = 0x01; script[4] = (uint8_t)(dest & 0xFF); script[5] = (uint8_t)(dest >> 8);
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = sizeof script; st.pc = 0x9900u; st.latch = 0x08;
    st.window2[0x10] = 0xAAAA;
}

static void env_init(pr_u1c_random_env *e, Rec *r, uint8_t al)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    r->al = al;
    e->hooks.rt_af8_11c0 = h_11c0; e->hooks.rt_af8_119a = h_119a; e->hooks.rt_af8_119e = h_119e; e->ctx = r;
}

static int seeded_tests(void)
{
    pr_u1c_random_report rep;
    unsigned v;

    /* The multiplier is odd. Inverting it chooses a seed whose next high
     * word is each possible 0..254 remainder for handler range 255. */
    for (v = 0; v < 255; ++v) {
        uint32_t expected_seed = (uint32_t)v << 16;
        uint32_t seed = (expected_seed - UINT32_C(1)) * UINT32_C(0xD94FA8CD);
        setup(254, 0x9710); /* saturating slot increment -> range 255 */
        CHECK(pr_u1c_op_random_seeded(&st, &seed, &rep) == PR_U1C_RANDOM_OK);
        CHECK(seed == expected_seed && st.window2[0x10] == v && rep.result == v);
        CHECK(rep.slot_byte == 254 && rep.argument == 255 && rep.stored);
        CHECK(rep.calls_n == 3 && rep.calls[0] == PR_U1C_RANDOM_CALL_11C0 &&
              rep.calls[1] == PR_U1C_RANDOM_CALL_119A &&
              rep.calls[2] == PR_U1C_RANDOM_CALL_119E);
    }
    /* The handler cannot pass range zero: a slot byte of zero becomes one. */
    { uint32_t seed = 0;
      setup(0, 0x9710);
      CHECK(pr_u1c_op_random_seeded(&st, &seed, &rep) == PR_U1C_RANDOM_OK);
      CHECK(seed == 1 && st.window2[0x10] == 0 && rep.argument == 1);
    }
    /* The saved-script operand 13 produces range 14, but this is synthetic
     * seed state and does not assert a DOS clock value or live route. */
    { uint32_t seed = UINT32_C(0x12345678);
      setup(13, 0x9710);
      CHECK(pr_u1c_op_random_seeded(&st, &seed, &rep) == PR_U1C_RANDOM_OK);
      CHECK(seed == UINT32_C(0xCB5B9059));
      CHECK(st.window2[0x10] == ((seed >> 16) % 14) && rep.result == st.window2[0x10]);
    }
    /* PM/p9 native one-Up measurement: seed before/after 11C0, AX after
     * 119E, and the 0x980F word. This tests the isolated handler with a
     * supplied measured seed, not a full C-native Up replay. */
    { uint32_t seed = UINT32_C(3140872353);
      setup(13, 0x980F);
      st.window2[0x10F] = 16;
      CHECK(pr_u1c_op_random_seeded(&st, &seed, &rep) == PR_U1C_RANDOM_OK);
      CHECK(seed == UINT32_C(3238563622) && rep.argument == 14);
      CHECK(rep.result == 10 && st.window2[0x10F] == 10 && rep.stored);
    }
    /* A missing seed is detected at the reached SYSTEM call, after operand
     * loading, but before any generator step or destination store. */
    setup(13, 0x9710);
    CHECK(pr_u1c_op_random_seeded(&st, NULL, &rep) == PR_U1C_RANDOM_SEED_MISSING);
    CHECK(st.pc == 0x9906u && st.window2[0x10] == 0xAAAA && !rep.stored);
    CHECK(rep.argument == 14 && rep.calls_n == 1 && rep.failed_call == PR_U1C_RANDOM_CALL_11C0);
    { static const uint8_t truncated[] = { 0x08, 0x00 };
      memset(&st, 0, sizeof st); st.script = truncated; st.script_size = sizeof truncated;
      st.pc = 0x9900u;
      CHECK(pr_u1c_op_random_seeded(&st, NULL, &rep) == PR_U1C_RANDOM_CORE);
      CHECK(rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && rep.calls_n == 0);
    }
    { uint32_t seed = UINT32_C(0x12345678);
      setup(0xFF, 0x6B40);
      CHECK(pr_u1c_op_random_seeded(&st, &seed, NULL) == PR_U1C_RANDOM_CORE);
      CHECK(seed == UINT32_C(0xCB5B9059));
      CHECK(st.window1[0x40] == ((seed >> 16) % 255));
    }
    return 0;
}

int main(void)
{
    pr_u1c_random_env env; Rec rec; pr_u1c_random_report rep;
    unsigned v;

    for (v = 0; v < 256; v++) {
        uint8_t al = (uint8_t)(v * 7u + 3u);
        unsigned expect_arg = v < 255 ? v + 1 : 255;
        setup((uint8_t)v, 0x9710); env_init(&env, &rec, al);
        CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_OK);
        CHECK(rep.slot_byte == v && rep.argument == expect_arg && rec.arg_11c0 == expect_arg && rec.dx_119a == 0);
        CHECK(rec.n == 3 && rec.log[0] == PR_U1C_RANDOM_CALL_11C0 && rec.log[1] == PR_U1C_RANDOM_CALL_119A && rec.log[2] == PR_U1C_RANDOM_CALL_119E);
        CHECK(rep.calls_n == 3 && rep.result == al && rep.dest == 0x9710 && rep.stored && st.window2[0x10] == al && st.pc == 0x9906u);
    }
    /* the stored word is zero-extended from AL */
    setup(5, 0x9710); env_init(&env, &rec, 0xFF);
    CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_OK && st.window2[0x10] == 0x00FF);

    /* failing hook at each step: later hooks do not run, nothing is stored */
    { int f;
      for (f = PR_U1C_RANDOM_CALL_11C0; f <= PR_U1C_RANDOM_CALL_119E; f++) {
          setup(9, 0x9710); env_init(&env, &rec, 0x42); rec.fail_call = f;
          CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_CALLBACK && rep.callback_status == 9 && rep.failed_call == (pr_u1c_random_call)f);
          CHECK(rec.n == (unsigned)f && !rep.stored && st.window2[0x10] == 0xAAAA);
      }
    }
    /* missing hook at each step: the earlier hooks have run, nothing is stored */
    setup(9, 0x9710); env_init(&env, &rec, 0x42); env.hooks.rt_af8_11c0 = NULL;
    CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_HOOK && rec.n == 0 && !rep.stored && rep.argument == 10);
    setup(9, 0x9710); env_init(&env, &rec, 0x42); env.hooks.rt_af8_119a = NULL;
    CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_HOOK && rec.n == 1 && !rep.stored);
    setup(9, 0x9710); env_init(&env, &rec, 0x42); env.hooks.rt_af8_119e = NULL;
    CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_HOOK && rec.n == 2 && !rep.stored && st.window2[0x10] == 0xAAAA);

    /* destination classes through the store word */
    setup(1, 0x6E82); env_init(&env, &rec, 0x21);
    CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_OK && st.window1[0x382] == 0x21 && rep.store_status == PR_REC_ECL_WINDOW_OK);
    setup(1, 0x6B40); env_init(&env, &rec, 0x21);                     /* class-1 address whose store hook is not exempt */
    CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && st.window1[0x40] == 0x21 && rep.stored);
    setup(1, 0x4910); env_init(&env, &rec, 0x21);
    CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_OK && st.window0[0x10] == 0x21);
    setup(1, 0x00B2); env_init(&env, &rec, 0x21);                     /* a class-4 destination: unsupported store */
    CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_CORE && rep.core_status != PR_REC_ECL_HANDLER_OK && rep.stored && rec.n == 3);

    /* loader failure and slot value failure: before any hook */
    { static const uint8_t trunc[] = { 0x08, 0x00 };
      memset(&st, 0, sizeof st); st.script = trunc; st.script_size = sizeof trunc; st.pc = 0x9900u; env_init(&env, &rec, 1);
      CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && rec.n == 0); }
    { static const uint8_t c4[] = { 0x08, 0x01, 0xB2, 0x00, 0x01, 0x10, 0x97 };      /* slot 1: unsupported class-4 source */
      memset(&st, 0, sizeof st); st.script = c4; st.script_size = sizeof c4; st.pc = 0x9900u; env_init(&env, &rec, 1);
      CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && rec.n == 0 && !rep.stored); }
    /* a class-1 slot read: the handler completes (hooks, store), then reports HOOK */
    { static const uint8_t c1[] = { 0x08, 0x01, 0x18, 0x6B, 0x01, 0x10, 0x97 };
      memset(&st, 0, sizeof st); st.script = c1; st.script_size = sizeof c1; st.pc = 0x9900u; st.window1[0x18] = 0x0003;
      st.window2[0x10] = 0xAAAA; env_init(&env, &rec, 0x66);
      CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status == PR_REC_ECL_WINDOW_HOOK);
      CHECK(rep.slot_byte == 3 && rep.argument == 4 && rec.n == 3 && st.window2[0x10] == 0x66 && rep.stored); }
    /* a word slot value: only its low byte is the argument source */
    { static const uint8_t w[] = { 0x08, 0x02, 0x7F, 0x01, 0x01, 0x10, 0x97 };
      memset(&st, 0, sizeof st); st.script = w; st.script_size = sizeof w; st.pc = 0x9900u; env_init(&env, &rec, 7);
      CHECK(pr_u1c_op_random(&st, &env, &rep) == PR_U1C_RANDOM_OK && rep.slot_byte == 0x7F && rep.argument == 0x80); }

    CHECK(pr_u1c_op_random(NULL, &env, &rep) == PR_U1C_RANDOM_ARGUMENT && pr_u1c_op_random(&st, NULL, &rep) == PR_U1C_RANDOM_ARGUMENT);
    setup(3, 0x9710); env_init(&env, &rec, 1);
    CHECK(pr_u1c_op_random(&st, &env, NULL) == PR_U1C_RANDOM_OK);
    CHECK(seeded_tests() == 0);
    puts("overlay_u1c_random_0234: ok");
    return 0;
}
