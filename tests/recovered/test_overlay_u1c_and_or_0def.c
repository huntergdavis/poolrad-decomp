/* Synthetic test for the unit 0x1c +0x0DEF AND/OR handler composition. Scripts, windows
 * and flags are fabricated. A plain-arithmetic restatement (low byte of the AND/OR, the
 * flags of comparing 0 with it, the stored word) is compared with the handler over many
 * operand values; the status paths of lane A's cores (class-1 hook, class-4 source,
 * bounds) are checked individually. */
#include "poolrad/recovered/overlay_u1c_and_or_0def.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;

static void setup(const uint8_t *script, size_t size, uint8_t latch)
{
    unsigned i;
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = size; st.pc = 0x9900u; st.latch = latch;
    for (i = 0; i < PR_REC_ECL_WINDOW0_WORDS; ++i) st.window0[i] = 0xAAAAu;
    for (i = 0; i < PR_REC_ECL_WINDOW1_WORDS; ++i) st.window1[i] = 0xAAAAu;
    for (i = 0; i < PR_REC_ECL_WINDOW2_WORDS; ++i) st.window2[i] = 0xAAAAu;
    memset(st.cmp_flags, 0x55, sizeof st.cmp_flags);
}

static int flags_are(int eq, int ne, int lt, int gt, int le, int ge)
{
    return st.cmp_flags[0] == eq && st.cmp_flags[1] == ne && st.cmp_flags[2] == lt && st.cmp_flags[3] == gt &&
           st.cmp_flags[4] == le && st.cmp_flags[5] == ge;
}

/* the dest operand used throughout: a class-0 window-0 address 0x4AC4 = word index 0x1C4 */
#define DEST 0x01, 0xC4, 0x4A

int main(void)
{
    pr_rec_ecl_window_status ws;
    pr_u1c_and_or_report rep;
    unsigned a, b, k;

    /* AND of two immediate bytes: 0x0F & 0x3C = 0x0C stored to the window-0 word; flags of 0 against 0x0C */
    { static const uint8_t s[] = { 0x2F, 0x00, 0x0F, 0x00, 0x3C, DEST };
      setup(s, sizeof s, 0x2F);
      CHECK(pr_u1c_op_and_or(&st, &ws, &rep) == PR_REC_ECL_HANDLER_OK && ws == PR_REC_ECL_WINDOW_OK);
      CHECK(st.window0[0x1C4] == 0x000C && rep.result == 0x0C && rep.v1 == 0x0F && rep.v2 == 0x3C && rep.dest == 0x4AC4 && rep.compared && rep.stored);
      CHECK(flags_are(0, 1, 1, 0, 1, 0) && st.pc == 0x9908u); }
    /* the same operands with the OR latch: 0x3F */
    { static const uint8_t s[] = { 0x30, 0x00, 0x0F, 0x00, 0x3C, DEST };
      setup(s, sizeof s, 0x30);
      CHECK(pr_u1c_op_and_or(&st, NULL, NULL) == PR_REC_ECL_HANDLER_OK && st.window0[0x1C4] == 0x003F); }
    /* any latch other than 0x2F takes the OR path */
    { static const uint8_t s[] = { 0x30, 0x00, 0x0F, 0x00, 0x30, DEST };
      setup(s, sizeof s, 0x12);
      CHECK(pr_u1c_op_and_or(&st, NULL, NULL) == PR_REC_ECL_HANDLER_OK && st.window0[0x1C4] == 0x003F);
      setup(s, sizeof s, 0x2F);
      CHECK(pr_u1c_op_and_or(&st, NULL, NULL) == PR_REC_ECL_HANDLER_OK && st.window0[0x1C4] == 0x0000 && flags_are(1, 0, 0, 0, 1, 1)); }

    /* word immediates: only the low byte of the 16-bit result is kept */
    { static const uint8_t s[] = { 0x2F, 0x02, 0x34, 0x12, 0x02, 0xFF, 0x00, DEST };    /* 0x1234 & 0x00FF = 0x0034 */
      setup(s, sizeof s, 0x2F);
      CHECK(pr_u1c_op_and_or(&st, NULL, &rep) == PR_REC_ECL_HANDLER_OK && st.window0[0x1C4] == 0x0034 && rep.result == 0x34); }
    { static const uint8_t s[] = { 0x30, 0x02, 0xFF, 0xFF, 0x02, 0x00, 0x01, DEST };    /* 0xFFFF | 0x0100 = 0xFFFF -> 0xFF */
      setup(s, sizeof s, 0x30);
      CHECK(pr_u1c_op_and_or(&st, NULL, &rep) == PR_REC_ECL_HANDLER_OK && st.window0[0x1C4] == 0x00FF && rep.result == 0xFF); }
    { static const uint8_t s[] = { 0x30, 0x02, 0x00, 0x01, 0x02, 0x00, 0x02, DEST };    /* 0x0100 | 0x0200 = 0x0300 -> 0 */
      setup(s, sizeof s, 0x30);
      CHECK(pr_u1c_op_and_or(&st, NULL, &rep) == PR_REC_ECL_HANDLER_OK && st.window0[0x1C4] == 0x0000 && flags_are(1, 0, 0, 0, 1, 1)); }

    /* a window-0 variable source (tag 1, class 0) */
    { static const uint8_t s[] = { 0x2F, 0x01, 0x10, 0x49 + 1, 0x00, 0xF0, DEST };      /* address 0x4A10 = index 0x110 */
      setup(s, sizeof s, 0x2F); st.window0[0x110] = 0x00AB;
      CHECK(pr_u1c_op_and_or(&st, NULL, &rep) == PR_REC_ECL_HANDLER_OK && rep.v1 == 0x00AB && st.window0[0x1C4] == 0x00A0); }

    /* generated: low byte of AND/OR of byte and word immediates against plain arithmetic, flags of 0 vs result */
    for (k = 0; k < 2; k++)
        for (a = 0; a < 0x10000; a += 0x0111 + (a & 0xFF)) {
            for (b = 0; b < 0x10000; b += 0x0F13 + (b & 3)) {
                uint8_t s[9];
                uint8_t expect = (uint8_t)(k == 0 ? (a & b) : (a | b));
                s[0] = (uint8_t)(k == 0 ? 0x2F : 0x30);
                s[1] = 0x02; s[2] = (uint8_t)a; s[3] = (uint8_t)(a >> 8);
                s[4] = 0x02; s[5] = (uint8_t)b; s[6] = (uint8_t)(b >> 8);
                s[7] = 0x01; s[8] = 0xC4;                      /* dest tag 1, then the address bytes (second byte below) */
                {
                    uint8_t full[10];
                    memcpy(full, s, 9); full[9] = 0x4A;
                    setup(full, sizeof full, s[0]);
                    CHECK(pr_u1c_op_and_or(&st, &ws, &rep) == PR_REC_ECL_HANDLER_OK && ws == PR_REC_ECL_WINDOW_OK);
                    CHECK(rep.result == expect && st.window0[0x1C4] == expect);
                    CHECK(flags_are(expect == 0, expect != 0, expect != 0, 0, 1, expect == 0));
                    CHECK(st.pc == 0x9900u + 10u);
                }
            }
        }

    /* class-1 source (window 1, address 0x6B40): the read runs lane A's hook model: HOOK after the store */
    { static const uint8_t s[] = { 0x2F, 0x01, 0x40, 0x6B, 0x00, 0xFF, DEST };
      setup(s, sizeof s, 0x2F); st.window1[0x40] = 0x0055;
      CHECK(pr_u1c_op_and_or(&st, &ws, &rep) == PR_REC_ECL_HANDLER_HOOK && ws == PR_REC_ECL_WINDOW_OK);
      CHECK(rep.slot1_status == PR_REC_ECL_WINDOW_HOOK && rep.stored && st.window0[0x1C4] == 0x0055 && flags_are(0, 1, 1, 0, 1, 0)); }
    { static const uint8_t s[] = { 0x2F, 0x00, 0xFF, 0x01, 0x40, 0x6B, DEST };        /* the same in slot 2 */
      setup(s, sizeof s, 0x2F); st.window1[0x40] = 0x0055;
      CHECK(pr_u1c_op_and_or(&st, NULL, &rep) == PR_REC_ECL_HANDLER_HOOK && rep.slot2_status == PR_REC_ECL_WINDOW_HOOK && st.window0[0x1C4] == 0x0055); }

    /* an unsupported class-4 source (address 0x00B2): UNRESOLVED; neither the compare nor the store is made; slot 2 is not read after a slot-1 stop */
    { static const uint8_t s[] = { 0x2F, 0x01, 0xB2, 0x00, 0x00, 0x7F, DEST };
      setup(s, sizeof s, 0x2F);
      CHECK(pr_u1c_op_and_or(&st, &ws, &rep) == PR_REC_ECL_HANDLER_UNRESOLVED && !rep.compared && !rep.stored && !rep.slot2_read);
      CHECK(st.cmp_flags[0] == 0x55 && st.cmp_flags[5] == 0x55 && st.window0[0x1C4] == 0xAAAA && ws == PR_REC_ECL_WINDOW_OK); }
    { static const uint8_t s[] = { 0x2F, 0x00, 0x7F, 0x01, 0xB2, 0x00, DEST };        /* unresolved in slot 2: slot 1 was read */
      setup(s, sizeof s, 0x2F);
      CHECK(pr_u1c_op_and_or(&st, NULL, &rep) == PR_REC_ECL_HANDLER_UNRESOLVED && rep.slot1_read && rep.slot2_read && !rep.compared);
      CHECK(st.cmp_flags[0] == 0x55 && st.window0[0x1C4] == 0xAAAA); }

    /* the saved-run shape: 0x7F AND the class-4 source 0xC04F (byte [0x6AB1]) stored to 0x6E82 (window 1, class 1).
       The source value is whatever the host holds in b6ab1 (synthetic here); the store core reports HOOK after storing,
       the flags are those of comparing 0 with the result */
    { static const uint8_t s[] = { 0x2F, 0x00, 0x7F, 0x01, 0x4F, 0xC0, 0x01, 0x82, 0x6E };
      unsigned v;
      for (v = 0; v < 256; v++) {
          uint8_t expect = (uint8_t)(0x7Fu & v);
          setup(s, sizeof s, 0x2F); st.b6ab1 = (uint8_t)v;
          CHECK(pr_u1c_op_and_or(&st, &ws, &rep) == PR_REC_ECL_HANDLER_HOOK && ws == PR_REC_ECL_WINDOW_HOOK);
          CHECK(rep.v1 == 0x7F && rep.v2 == v && rep.result == expect && st.window1[0x382] == expect && rep.compared && rep.stored);
          CHECK(flags_are(expect == 0, expect != 0, expect != 0, 0, 1, expect == 0) && st.pc == 0x9909u);
      }
      /* an unsupported neighbour (0xC059 = 0xC04B + 0xE) is still unresolved: no compare, no store */
      { static const uint8_t u[] = { 0x2F, 0x00, 0x7F, 0x01, 0x59, 0xC0, 0x01, 0x82, 0x6E };
        setup(u, sizeof u, 0x2F);
        CHECK(pr_u1c_op_and_or(&st, &ws, &rep) == PR_REC_ECL_HANDLER_UNRESOLVED && !rep.compared && !rep.stored && st.window1[0x382] == 0xAAAA); } }

    /* the destination is class 1: the store core reports HOOK after storing; the flags were set before it */
    { static const uint8_t s[] = { 0x2F, 0x00, 0x0F, 0x00, 0x3C, 0x01, 0x40, 0x6B };
      setup(s, sizeof s, 0x2F);
      CHECK(pr_u1c_op_and_or(&st, &ws, &rep) == PR_REC_ECL_HANDLER_HOOK && ws == PR_REC_ECL_WINDOW_HOOK && st.window1[0x40] == 0x000C);
      CHECK(flags_are(0, 1, 1, 0, 1, 0)); }

    /* truncated operands: BOUNDS from the loader, nothing else done */
    { static const uint8_t s[] = { 0x2F, 0x00, 0x0F, 0x00, 0x3C };
      setup(s, sizeof s, 0x2F);
      CHECK(pr_u1c_op_and_or(&st, &ws, &rep) == PR_REC_ECL_HANDLER_BOUNDS && !rep.slot1_read && st.cmp_flags[0] == 0x55); }
    CHECK(pr_u1c_op_and_or(NULL, &ws, &rep) == PR_REC_ECL_HANDLER_ARGUMENT);

    puts("overlay_u1c_and_or_0def: ok");
    return 0;
}
