/* Synthetic test for the unit 0x1c +0x10D1 HORIZONTAL MENU handler model. The script (six-bit packed string items), the
 * four code-segment constants, the DS bytes and both hooks are fabricated. The composed text is compared with an
 * independent restatement of the original's order (separator, item, separator per item, the last item without the trailing
 * separator, each store truncated to 50 bytes); the n = 1 buffer replacement, the menu parameters, the 0x84E0/0x84E1
 * flag, hook order and arguments, the retained effects at failing or missing hooks, loader and slot failures, the count
 * bounds and the destination classes are checked individually. */
#include "poolrad/recovered/overlay_u1c_horizontal_menu_10d1.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;
static uint8_t script[512];
static size_t script_len;

typedef struct {
    int log[4]; unsigned n;
    uint8_t var5_in, kind, p3e, p3d, p3b, p3c; uint8_t text[256]; uint8_t al; uint8_t set_var5;
    uint8_t region[4];
    int fail_call;
} Rec;

static int h_menu(void *c, pr_u1c_hmenu_env *e, uint8_t *var5, const uint8_t *text, uint8_t kind, uint8_t p3e, uint8_t p3d,
                  uint8_t p3b, uint8_t p3c, uint8_t *al)
{
    Rec *r = c; (void)e;
    r->var5_in = *var5; *var5 = r->set_var5; r->kind = kind; r->p3e = p3e; r->p3d = p3d; r->p3b = p3b; r->p3c = p3c;
    memcpy(r->text, text, 1u + text[0]); *al = r->al; r->log[r->n++] = 1;
    return r->fail_call == 1 ? 9 : 0;
}
static int h_region(void *c, pr_u1c_hmenu_env *e, const uint8_t a[4])
{ Rec *r = c; (void)e; memcpy(r->region, a, 4); r->log[r->n++] = 2; return r->fail_call == 2 ? 9 : 0; }

/* the text of a packed string operand "80 len bytes...": the model's own unpacker is the lane-A one */
static void item_text(const uint8_t *packed, unsigned len, uint8_t *out)
{
    out[0] = (uint8_t)pr_rec_ecl_unpack_text(packed, len, (char *)out + 1, 255);
}

static const uint8_t P1[3] = { 0x41, 0x82, 0xC3 }, P2[3] = { 0x11, 0x22, 0x33 }, P3[3] = { 0x5A, 0x6B, 0x7C };
static const uint8_t LONGP[30] = { 0x41, 0x82, 0xC3, 0x41, 0x82, 0xC3, 0x41, 0x82, 0xC3, 0x41, 0x82, 0xC3, 0x41, 0x82, 0xC3,
                                   0x41, 0x82, 0xC3, 0x41, 0x82, 0xC3, 0x41, 0x82, 0xC3, 0x41, 0x82, 0xC3, 0x41, 0x82, 0xC3 };

/* opcode, slot 1 = word destination (tag 1), slot 2 = literal count (tag 0), then `n` string items with packed lengths */
static void setup(uint16_t dest, uint8_t n, const uint8_t *items[], const unsigned lens[])
{
    unsigned k;
    size_t o = 0;
    script[o++] = 0x2B;
    script[o++] = 0x01; script[o++] = (uint8_t)(dest & 0xFF); script[o++] = (uint8_t)(dest >> 8);
    script[o++] = 0x00; script[o++] = n;
    for (k = 0; k < n; k++) {
        script[o++] = 0x80; script[o++] = (uint8_t)lens[k];
        memcpy(script + o, items[k], lens[k]); o += lens[k];
    }
    script_len = o;
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = script_len; st.pc = 0x9900u; st.latch = 0x2B;
    st.window2[0x10] = 0xAAAA;
}

static const uint8_t K1085[] = { 2, 'A', 'B' }, K10AA[] = { 3, 'X', 'Y', 'Z' }, K10CD[] = { 1, '>' }, K10CF[] = { 1, '|' };

static void env_init(pr_u1c_hmenu_env *e, Rec *r, uint8_t al)
{
    memset(e, 0, sizeof *e); memset(r, 0, sizeof *r);
    e->str_1085 = K1085; e->str_10aa = K10AA; e->str_10cd = K10CD; e->str_10cf = K10CF;
    r->al = al; r->set_var5 = 0x77;
    e->hooks.menu_25_0061 = h_menu; e->hooks.region_709_0dd3 = h_region; e->ctx = r;
}

/* independent restatement: text := text + a + item [+ b], truncated to 50 after each store */
static void rcat(uint8_t *t, const uint8_t *a, const uint8_t *item, const uint8_t *b)
{
    uint8_t tmp[300]; unsigned len = t[0], k;
    memcpy(tmp, t + 1, len);
    for (k = 0; k < a[0]; k++) tmp[len++] = a[1 + k];
    for (k = 0; k < item[0]; k++) tmp[len++] = item[1 + k];
    if (b) for (k = 0; k < b[0]; k++) tmp[len++] = b[1 + k];
    if (len > 50) len = 50;
    t[0] = (uint8_t)len; memcpy(t + 1, tmp, len);
}

int main(void)
{
    pr_u1c_hmenu_env env; Rec rec; pr_u1c_hmenu_report rep;
    uint8_t it1[256], it2[256], it3[256], itL[256], want[64];
    const uint8_t *items3[3] = { P1, P2, P3 }; const unsigned lens3[3] = { 3, 3, 3 };
    const uint8_t *items1[1] = { P1 }; const unsigned lens1[1] = { 3 };
    unsigned i;
    item_text(P1, 3, it1); item_text(P2, 3, it2); item_text(P3, 3, it3); item_text(LONGP, 30, itL);
    CHECK(it1[0] > 0 && it2[0] > 0 && it3[0] > 0 && itL[0] == 40);

    /* n = 3: > item1 | > item2 | > item3 ; parameters p3b 0, p3d 0x0F, p3e 0x0A */
    setup(0x9710, 3, items3, lens3); env_init(&env, &rec, 7);
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_OK);
    memset(want, 0, sizeof want);
    rcat(want, K10CD, it1, K10CF); rcat(want, K10CD, it2, K10CF); rcat(want, K10CD, it3, NULL);
    CHECK(rep.text[0] == want[0] && memcmp(rep.text, want, 1u + want[0]) == 0 && rec.text[0] == want[0] && memcmp(rec.text, want, 1u + want[0]) == 0);
    CHECK(rep.n == 3 && rep.p3b == 0 && rep.p3d == 0x0F && rep.p3e == 0x0A && !rep.buffer1_replaced);
    CHECK(rec.var5_in == 0 && rec.kind == 0x0D && rec.p3b == 0 && rec.p3d == 0x0F && rec.p3e == 0x0A && rec.p3c == 0 && rep.var5 == 0x77 && rep.result == 7);
    CHECK(rec.n == 2 && rec.log[0] == 1 && rec.log[1] == 2 && rec.region[0] == 0 && rec.region[1] == 0x18 && rec.region[2] == 0x27 && rec.region[3] == 0x18);
    CHECK(rep.dest == 0x9710 && st.window2[0x10] == 7 && rep.stored && st.pc == 0x9900u + script_len);

    /* n = 1, buffer 1 different from cs:0x1085: > item1 ; p3b 1, p3d 0x0F, p3e 0x0F */
    setup(0x9710, 1, items1, lens1); env_init(&env, &rec, 2);
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_OK && !rep.buffer1_replaced);
    memset(want, 0, sizeof want); rcat(want, K10CD, it1, NULL);
    CHECK(memcmp(rep.text, want, 1u + want[0]) == 0 && rep.p3b == 1 && rep.p3d == 0x0F && rep.p3e == 0x0F);
    /* n = 1, buffer 1 equal to cs:0x1085: buffer 1 := cs:0x10AA before the text is built */
    {
        uint8_t k1085[256];
        memcpy(k1085, it1, 1u + it1[0]);
        setup(0x9710, 1, items1, lens1); env_init(&env, &rec, 2); env.str_1085 = k1085;
        CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_OK && rep.buffer1_replaced);
        CHECK(st.string_len[1] == 3 && memcmp(st.string_text[1], "XYZ", 3) == 0);
        memset(want, 0, sizeof want); rcat(want, K10CD, K10AA, NULL);
        CHECK(memcmp(rep.text, want, 1u + want[0]) == 0);
        /* a different length with a common prefix is not equal */
        k1085[0] = (uint8_t)(it1[0] + 1); k1085[1u + it1[0]] = 'Q';
        setup(0x9710, 1, items1, lens1); env_init(&env, &rec, 2); env.str_1085 = k1085;
        CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_OK && !rep.buffer1_replaced);
    }

    /* every store is truncated to 50 bytes: five long items */
    {
        const uint8_t *items5[5] = { LONGP, LONGP, LONGP, LONGP, LONGP }; const unsigned lens5[5] = { 30, 30, 30, 30, 30 };
        setup(0x9710, 5, items5, lens5); env_init(&env, &rec, 1);
        CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_OK);
        memset(want, 0, sizeof want);
        for (i = 0; i < 4; i++) rcat(want, K10CD, itL, K10CF);
        rcat(want, K10CD, itL, NULL);
        CHECK(want[0] == 50 && rep.text[0] == 50 && memcmp(rep.text, want, 51) == 0);
    }

    /* p3c from the two DS bytes */
    for (i = 0; i < 4; i++) {
        setup(0x9710, 3, items3, lens3); env_init(&env, &rec, 1); env.b84e0 = (uint8_t)(i & 1 ? 5 : 0); env.b84e1 = (uint8_t)(i & 2 ? 9 : 0);
        CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_OK && rec.p3c == (i == 3) && rep.p3c == (i == 3));
    }

    /* failing hooks keep earlier effects: a failing menu hook stores nothing; a failing region hook runs after the store */
    setup(0x9710, 3, items3, lens3); env_init(&env, &rec, 7); rec.fail_call = 1;
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_CALLBACK && rep.callback_status == 9 && !rep.stored && st.window2[0x10] == 0xAAAA && rec.n == 1);
    setup(0x9710, 3, items3, lens3); env_init(&env, &rec, 7); rec.fail_call = 2;
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_CALLBACK && rec.n == 2 && st.window2[0x10] == 7);
    setup(0x9710, 3, items3, lens3); env_init(&env, &rec, 7); env.hooks.menu_25_0061 = NULL;
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_HOOK && !rep.stored && rec.n == 0);
    setup(0x9710, 3, items3, lens3); env_init(&env, &rec, 7); env.hooks.region_709_0dd3 = NULL;
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_HOOK && rep.stored && st.window2[0x10] == 7);

    /* missing code constants where reached */
    setup(0x9710, 1, items1, lens1); env_init(&env, &rec, 1); env.str_1085 = NULL;
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_STRING && rec.n == 0);
    setup(0x9710, 3, items3, lens3); env_init(&env, &rec, 1); env.str_10cf = NULL;
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_STRING);
    setup(0x9710, 1, items1, lens1); env_init(&env, &rec, 1); env.str_10cf = NULL; env.str_10aa = NULL;      /* n = 1 needs neither 0x10CF nor (when unequal) 0x10AA */
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_OK);

    /* destination classes through the store word */
    setup(0x6E82, 1, items1, lens1); env_init(&env, &rec, 5);
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_OK && st.window1[0x382] == 5);
    setup(0x6B20, 1, items1, lens1); env_init(&env, &rec, 5);                  /* a store-effect class-1 address: stored, region hook still runs, then HOOK */
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && st.window1[0x20] == 5 && rep.region_called);
    setup(0x00B2, 1, items1, lens1); env_init(&env, &rec, 5);                  /* an unsupported destination stops before the region hook */
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_CORE && rep.core_status != PR_REC_ECL_HANDLER_OK && rep.stored && !rep.region_called);

    /* count bounds and loader/slot failures */
    setup(0x9710, 0, items1, lens1); env_init(&env, &rec, 1);
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_COUNT && rec.n == 0);
    { static const uint8_t big[] = { 0x2B, 0x01, 0x10, 0x97, 0x00, 0x40 };      /* n = 64: the loader bound */
      memset(&st, 0, sizeof st); st.script = big; st.script_size = sizeof big; st.pc = 0x9900u; env_init(&env, &rec, 1);
      CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_CORE && rep.core_status == PR_REC_ECL_HANDLER_ARGUMENT && rep.n == 64 && st.pc == 0x9905u); }
    { static const uint8_t trunc[] = { 0x2B, 0x01 };
      memset(&st, 0, sizeof st); st.script = trunc; st.script_size = sizeof trunc; st.pc = 0x9900u; env_init(&env, &rec, 1);
      CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS); }
    /* slot value(2) from a class-1 address whose load hook may override the word (0x6B18, rel 0x18): n is unknown, so the handler stops
       at that boundary: CORE/HOOK, the PC is not decremented, no loader(n), no hook, no store */
    { static const uint8_t hk[] = { 0x2B, 0x01, 0x10, 0x97, 0x01, 0x18, 0x6B, 0x80, 0x00 };
      memset(&st, 0, sizeof st); st.script = hk; st.script_size = sizeof hk; st.pc = 0x9900u; st.window1[0x18] = 1; st.window2[0x10] = 0xAAAA; env_init(&env, &rec, 1);
      CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status == PR_REC_ECL_WINDOW_HOOK);
      CHECK(rep.n == 0 && rec.n == 0 && !rep.stored && !rep.menu_called && st.window2[0x10] == 0xAAAA && st.string_len[1] == 0);
      CHECK(st.pc == 0x9907u); }                                              /* the loader(2) advance stands; no decrement */
    /* the same read from a plain class-1 address (0x6E80, rel 0x380) is an ordinary value: the handler runs with that n */
    { static const uint8_t pl[] = { 0x2B, 0x01, 0x10, 0x97, 0x01, 0x80, 0x6E, 0x80, 0x00 };
      memset(&st, 0, sizeof st); st.script = pl; st.script_size = sizeof pl; st.pc = 0x9900u; st.window1[0x380] = 1; env_init(&env, &rec, 3);
      CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_OK && rep.n == 1 && rec.n == 2 && st.window2[0x10] == 3); }
    { static const uint8_t c4[] = { 0x2B, 0x01, 0x10, 0x97, 0x01, 0xB2, 0x00 };   /* slot 2: unsupported class-4 */
      memset(&st, 0, sizeof st); st.script = c4; st.script_size = sizeof c4; st.pc = 0x9900u; env_init(&env, &rec, 1);
      CHECK(pr_u1c_op_horizontal_menu(&st, &env, &rep) == PR_U1C_HMENU_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && rec.n == 0 && st.pc == 0x9907u); }

    CHECK(pr_u1c_op_horizontal_menu(NULL, &env, &rep) == PR_U1C_HMENU_ARGUMENT && pr_u1c_op_horizontal_menu(&st, NULL, &rep) == PR_U1C_HMENU_ARGUMENT);
    setup(0x9710, 1, items1, lens1); env_init(&env, &rec, 1);
    CHECK(pr_u1c_op_horizontal_menu(&st, &env, NULL) == PR_U1C_HMENU_OK);
    puts("overlay_u1c_horizontal_menu_10d1: ok");
    return 0;
}
