/* Synthetic test for the LOAD MONSTER entry-prefix model (unit 0x1c +0x0450 up to the return of the unit 0x25 entry-5 wrapper).
 * The script, heap, object bytes and both wrapper hooks are fabricated; nothing is invented about the real record. Checked: the
 * [0x4435] >= 0x3F early return (no slot read, no wrapper), the exact order loader(3) -> cap test -> slot value(1) -> wrapper, a class-1
 * load-hook HOOK at slot value(1) stopping before the wrapper, plain class-1 and literal ids, the full initializer word the wrapper
 * passes ((GetMem offset & 0xFF00) | id low byte), untouched outputs on every wrapper failure, and the argument/loader/slot guards. */
#include "poolrad/recovered/overlay_u1c_load_monster_prefix_0450.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;
static uint8_t obj_bytes[0x200];
static pr_u25_e5_object table[1];
static pr_u25_e5_env wenv;
static pr_u1c_lmon_env env;

typedef struct { int log[4]; unsigned n; uint16_t size; uint16_t id_word; pr_far16 obj; int fail_at; pr_far16 give; } Rec;
static Rec rec;

static int h_getmem(void *c, pr_u25_e5_env *e, uint16_t size, pr_far16 *res)
{ Rec *r = c; (void)e; r->size = size; *res = r->give; r->log[r->n++] = 1; return r->fail_at == 1 ? 9 : 0; }
static int h_init(void *c, pr_u25_e5_env *e, uint16_t id_word, pr_far16 obj)
{
    Rec *r = c; (void)e;
    r->id_word = id_word; r->obj = obj; r->log[r->n++] = 2;
    obj_bytes[0x10 + 0xC8] = 0x34; obj_bytes[0x10 + 0xC9] = 0x12; obj_bytes[0x10 + 0xCA] = 0x78; obj_bytes[0x10 + 0xCB] = 0x56;     /* what a fake initializer leaves */
    obj_bytes[0x10 + 0x7F] = 0xBC; obj_bytes[0x10 + 0x80] = 0x9A; obj_bytes[0x10 + 0x81] = 0xF0; obj_bytes[0x10 + 0x82] = 0xDE;
    return r->fail_at == 2 ? 9 : 0;
}

/* opcode, then three operands: slot 1 per `s1`, slot 2 and slot 3 literal bytes (they are not read by the prefix) */
static uint8_t script[16];
static void setup(const uint8_t *s1, size_t n1)
{
    size_t o = 0;
    script[o++] = 0x0B;
    memcpy(script + o, s1, n1); o += n1;
    script[o++] = 0x00; script[o++] = 0x01;
    script[o++] = 0x00; script[o++] = 0x02;
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = o; st.pc = 0x9900u; st.latch = 0x0B;
    memset(obj_bytes, 0x5A, sizeof obj_bytes);
    memset(&rec, 0, sizeof rec); rec.give.off = 0x0010; rec.give.seg = 0x3000;
    table[0].segment = obj_bytes; table[0].segment_size = sizeof obj_bytes; table[0].segment_id = 0x3000; table[0].offset = 0x0010;
    memset(&wenv, 0, sizeof wenv);
    wenv.objects = table; wenv.object_count = 1; wenv.hooks.getmem_af8_023f = h_getmem; wenv.hooks.init_u6f_0ec4 = h_init; wenv.ctx = &rec;
    memset(&env, 0, sizeof env);
    env.b4435 = 0; env.w5d92 = 0x1111; env.w5d94 = 0x2222;
    env.stack_a.off = 0xA1A1; env.stack_a.seg = 0xA2A2; env.stack_b.off = 0xB1B1; env.stack_b.seg = 0xB2B2; env.stack_c.off = 0xC1C1; env.stack_c.seg = 0xC2C2;
    env.wrapper = &wenv;
}

int main(void)
{
    static const uint8_t lit7[] = { 0x00, 0x07 };                     /* slot 1: literal byte 7 */
    static const uint8_t word[] = { 0x02, 0x34, 0x12 };               /* slot 1: word immediate 0x1234 (low byte 0x34) */
    static const uint8_t c1plain[] = { 0x01, 0x80, 0x6E };            /* slot 1: class-1 plain address 0x6E80 (no load-hook effect) */
    static const uint8_t c1hook[] = { 0x01, 0x18, 0x6B };             /* slot 1: class-1 load-hook EFFECT address 0x6B18 (rel 0x18) */
    static const uint8_t c4bad[] = { 0x01, 0xB2, 0x00 };              /* slot 1: unsupported class-4 address */
    pr_u1c_lmon_report rep;
    unsigned b;

    /* the normal prefix: loader(3), cap test passes, slot value(1), wrapper (GetMem then initializer), stop at its return */
    setup(lit7, sizeof lit7);
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_PREFIX_DONE && rep.wrapper_called && rep.wrapper_status == PR_U25_E5_OK);
    CHECK(rep.id == 7 && rec.n == 2 && rec.log[0] == 1 && rec.log[1] == 2 && rec.size == 0x11D);
    CHECK(st.pc == 0x9907u);                                                                         /* 1 opcode + 3 literal operands of 2 bytes, then the loader's own increment */
    CHECK(rep.var_a.off == 0x0010 && rep.var_a.seg == 0x3000 && rep.var_b.off == 0x1234 && rep.var_b.seg == 0x5678 && rep.var_c.off == 0x9ABC && rep.var_c.seg == 0xDEF0);
    CHECK(rep.saved_w5d92 == 0x1111 && rep.saved_w5d94 == 0x2222 && env.w5d92 == 0x1111);          /* read only */

    /* slot 1 as a word value: the id is its low byte; the wrapper's initializer word = (GetMem offset & 0xFF00) | id low byte */
    setup(word, sizeof word); rec.give.off = 0xAB10; table[0].offset = 0xAB10;
    (void)pr_u1c_op_load_monster_prefix(&st, &env, &rep);
    CHECK(rep.id == 0x34 && rec.id_word == 0xAB34 && rec.obj.off == 0xAB10);                      /* the full 16-bit word, not the byte */
    for (b = 0; b < 256; b++) {
        uint8_t lit[2]; lit[0] = 0x00; lit[1] = (uint8_t)b;
        setup(lit, 2); rec.give.off = 0xCD10; table[0].offset = 0xCD10;
        (void)pr_u1c_op_load_monster_prefix(&st, &env, &rep);
        CHECK(rep.id == b && rec.id_word == (uint16_t)(0xCD00u | b));
    }

    /* [0x4435] >= 0x3F: the routine returns at once: the loader's PC advance only, no slot read (even of a hooking operand), no wrapper */
    for (b = 0x3F; b <= 0xFF; b += (b < 0x42 ? 1u : 0x3Du)) {
        setup(c1hook, sizeof c1hook); env.b4435 = (uint8_t)b;
        CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_CAP_RETURN && !rep.wrapper_called && rec.n == 0);
        CHECK(rep.slot_status == PR_REC_ECL_WINDOW_OK && rep.id == 0 && st.pc == 0x9908u);
        CHECK(rep.var_a.off == 0xA1A1 && rep.var_b.off == 0xB1B1 && rep.var_c.off == 0xC1C1);        /* outputs untouched */
    }
    setup(lit7, sizeof lit7); env.b4435 = 0x3E;                                                      /* one below the cap: the body runs */
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_PREFIX_DONE);

    /* slot value(1) HOOK (a load-hook effect class-1 address): stop before the wrapper; outputs untouched; the plain class-1 address passes */
    setup(c1hook, sizeof c1hook); st.window1[0x18] = 5;
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status == PR_REC_ECL_WINDOW_HOOK);
    CHECK(!rep.wrapper_called && rec.n == 0 && rep.var_a.off == 0xA1A1 && rep.var_b.off == 0xB1B1 && rep.var_c.off == 0xC1C1 && rep.id == 0);
    setup(c1plain, sizeof c1plain); st.window1[0x380] = 9;
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_PREFIX_DONE && rep.id == 9);

    /* an unsupported slot-1 source and a truncated operand list stop before the wrapper */
    setup(c4bad, sizeof c4bad);
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && rec.n == 0);
    { static const uint8_t trunc[] = { 0x0B, 0x00 };
      setup(lit7, sizeof lit7); st.script = trunc; st.script_size = sizeof trunc;
      CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && rec.n == 0 && !rep.wrapper_called); }

    /* wrapper failures: the model stops with the wrapper's status; outputs are what the wrapper left (untouched residue where it wrote nothing) */
    setup(lit7, sizeof lit7); rec.fail_at = 1;                                                       /* GetMem fails: A is nil, B and C untouched */
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_WRAPPER && rep.wrapper_status == PR_U25_E5_CALLBACK && rep.wrapper_report.failed_call == PR_U25_E5_CALL_GETMEM);
    CHECK(rep.var_a.off == 0 && rep.var_a.seg == 0 && rep.var_b.off == 0xB1B1 && rep.var_c.off == 0xC1C1);
    setup(lit7, sizeof lit7); rec.fail_at = 2;                                                       /* initializer fails: A = P, B and C untouched */
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_WRAPPER && rep.wrapper_report.failed_call == PR_U25_E5_CALL_INIT);
    CHECK(rep.var_a.off == 0x0010 && rep.var_b.off == 0xB1B1 && rep.var_c.off == 0xC1C1);
    setup(lit7, sizeof lit7); wenv.hooks.init_u6f_0ec4 = NULL;                                       /* the unresolved record initializer is not bound */
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_WRAPPER && rep.wrapper_status == PR_U25_E5_HOOK && rep.var_a.off == 0x0010 && rep.var_b.off == 0xB1B1);
    setup(lit7, sizeof lit7); rec.give.off = 0x0020;                                                 /* an unmapped object */
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_WRAPPER && rep.wrapper_status == PR_U25_E5_UNMAPPED && rep.var_b.off == 0xB1B1);

    /* argument guards */
    setup(lit7, sizeof lit7);
    CHECK(pr_u1c_op_load_monster_prefix(NULL, &env, &rep) == PR_U1C_LMON_ARGUMENT && pr_u1c_op_load_monster_prefix(&st, NULL, &rep) == PR_U1C_LMON_ARGUMENT);
    env.wrapper = NULL;
    CHECK(pr_u1c_op_load_monster_prefix(&st, &env, &rep) == PR_U1C_LMON_ARGUMENT && st.pc == 0x9900u);
    puts("overlay_u1c_load_monster_prefix_0450: ok");
    return 0;
}
