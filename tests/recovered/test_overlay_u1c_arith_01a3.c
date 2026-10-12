/* Synthetic test for the unit 0x1c +0x01A3 ECL arithmetic handler model. Scripts and state are fabricated.
 * Edge behavior is compared with an independent restatement in 32-bit arithmetic over boundary words for all
 * four opcodes: 16-bit wrap of add and subtract, the subtract operand order (second minus first), unsigned
 * divide and its zero-divisor fault, and the multiply's dropped high word. Operand forms (byte literal, word
 * immediate, window variable), the store classes, loader and slot failures, and the stack-residue path are
 * checked individually. A two-instruction same-word chain and a variable zero divisor also check
 * evaluation order, PC advance, and the absence of a store on a fault. */
#include "poolrad/recovered/overlay_u1c_arith_01a3.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;
static uint8_t script[16];

/* opcode, two word-immediate operands (tag 2), a word destination address (tag 1) */
static void setup_words(uint8_t latch, uint16_t a, uint16_t b, uint16_t dest)
{
    size_t o = 0;
    script[o++] = latch;
    script[o++] = 0x02; script[o++] = (uint8_t)(a & 0xFF); script[o++] = (uint8_t)(a >> 8);
    script[o++] = 0x02; script[o++] = (uint8_t)(b & 0xFF); script[o++] = (uint8_t)(b >> 8);
    script[o++] = 0x01; script[o++] = (uint8_t)(dest & 0xFF); script[o++] = (uint8_t)(dest >> 8);
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = o; st.pc = 0x9900u; st.latch = latch;
    st.window2[0x10] = 0xAAAA;
}

int main(void)
{
    static const uint16_t vals[] = { 0, 1, 2, 3, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF };
    const unsigned nv = sizeof vals / sizeof vals[0];
    unsigned op, ia, ib;
    pr_u1c_arith_report rep;

    for (op = 4; op <= 7; op++) for (ia = 0; ia < nv; ia++) for (ib = 0; ib < nv; ib++) {
        uint32_t a = vals[ia], b = vals[ib], want = 0;
        pr_u1c_arith_status s;
        setup_words((uint8_t)op, vals[ia], vals[ib], 0x9710);
        s = pr_u1c_op_arith(&st, 0xBEEF, &rep);
        if (op == 6 && b == 0) {
            CHECK(s == PR_U1C_ARITH_DIVIDE_ERROR && rep.runtime_error == 200 && !rep.stored && st.window2[0x10] == 0xAAAA);
            CHECK(rep.v1 == a && rep.v2 == b && rep.dest == 0x9710);
            continue;
        }
        if (op == 4) want = (a + b) & 0xFFFF;
        if (op == 5) want = (b - a) & 0xFFFF;                    /* the second operand minus the first */
        if (op == 6) want = a / b;
        if (op == 7) want = (a * b) & 0xFFFF;
        CHECK(s == PR_U1C_ARITH_OK && rep.runtime_error == 0 && rep.stored);
        CHECK(rep.v1 == a && rep.v2 == b && rep.result == want && rep.dest == 0x9710 && st.window2[0x10] == want && st.pc == 0x990Au);
    }
    /* named observations: subtract order, add wrap, divide truncation and operand order, multiply high word dropped */
    setup_words(5, 3, 10, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window2[0x10] == 7);               /* 10 - 3, not 3 - 10 */
    setup_words(5, 10, 3, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window2[0x10] == 0xFFF9);
    setup_words(4, 0xFFFF, 1, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window2[0x10] == 0);
    setup_words(6, 10, 3, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window2[0x10] == 3);               /* first / second, truncated */
    setup_words(6, 3, 10, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window2[0x10] == 0);
    setup_words(6, 0xFFFF, 1, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window2[0x10] == 0xFFFF);          /* unsigned: no quotient overflow */
    setup_words(7, 0x100, 0x100, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window2[0x10] == 0);               /* 0x10000 low word */

    /* operand forms: a byte literal (tag 0), a window variable (tag 1) and a word immediate (tag 2) all give a 16-bit value */
    { static const uint8_t forms[] = { 0x04, 0x00, 0xFF, 0x01, 0x10, 0x97, 0x01, 0x10, 0x97 };          /* 0xFF + w2[0x10] -> w2[0x10] */
      memset(&st, 0, sizeof st); st.script = forms; st.script_size = sizeof forms; st.pc = 0x9900u; st.latch = 4; st.window2[0x10] = 0x0101;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && rep.v1 == 0xFF && rep.v2 == 0x0101 && st.window2[0x10] == 0x0200); }
    /* the saved-route shape (analogous values): DIVIDE a window word by a literal into the same word */
    { static const uint8_t shape[] = { 0x06, 0x01, 0x10, 0x97, 0x00, 0x03, 0x01, 0x10, 0x97 };
      memset(&st, 0, sizeof st); st.script = shape; st.script_size = sizeof shape; st.pc = 0x9900u; st.latch = 6; st.window2[0x10] = 10;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window2[0x10] == 3 && st.pc == 0x9909u); }

    /* Two synthetic instructions reuse one source/destination word. The second must read the first store,
     * and both loaders must leave PC at the following opcode. This also checks the 16-bit top boundary. */
    { static const uint8_t chain[] = {
          0x06, 0x01, 0x10, 0x97, 0x00, 0x03, 0x01, 0x10, 0x97,
          0x07, 0x01, 0x10, 0x97, 0x00, 0x03, 0x01, 0x10, 0x97
      };
      memset(&st, 0, sizeof st); st.script = chain; st.script_size = sizeof chain;
      st.pc = 0x9900u; st.latch = 6; st.window2[0x10] = 0xFFFFu;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK);
      CHECK(rep.v1 == 0xFFFFu && rep.v2 == 3u && rep.result == 0x5555u);
      CHECK(rep.store_status == PR_REC_ECL_WINDOW_OK && st.window2[0x10] == 0x5555u && st.pc == 0x9909u);
      st.latch = 7; /* the dispatcher fetches the next opcode before calling the handler */
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK);
      CHECK(rep.v1 == 0x5555u && rep.v2 == 3u && rep.result == 0xFFFFu);
      CHECK(rep.store_status == PR_REC_ECL_WINDOW_OK && st.window2[0x10] == 0xFFFFu && st.pc == 0x9912u); }

    /* A zero divisor read from another class-2 word faults after all three operands are loaded.
     * The aliased destination keeps its old value, and the store helper is never called. */
    { static const uint8_t variable_zero[] = {
          0x06, 0x01, 0x10, 0x97, 0x01, 0x12, 0x97, 0x01, 0x10, 0x97
      };
      memset(&st, 0, sizeof st); st.script = variable_zero; st.script_size = sizeof variable_zero;
      st.pc = 0x9900u; st.latch = 6; st.window2[0x10] = 7u;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_DIVIDE_ERROR);
      CHECK(rep.v1 == 7u && rep.v2 == 0u && rep.dest == 0x9710u);
      CHECK(rep.runtime_error == 200u && !rep.stored && st.window2[0x10] == 7u && st.pc == 0x990Au); }

    /* destination classes through the store word */
    setup_words(4, 2, 3, 0x6E82);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window1[0x382] == 5);
    setup_words(4, 2, 3, 0x6B40);                                  /* class-1 address whose store hook is not exempt */
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && st.window1[0x40] == 5 && rep.stored);
    setup_words(4, 2, 3, 0x4910);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK && st.window0[0x10] == 5);
    setup_words(4, 2, 3, 0x00B2);                                  /* class-4 destination: unsupported store */
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_CORE && rep.core_status != PR_REC_ECL_HANDLER_OK && rep.stored);

    /* the divide fault leaves loader and slot effects but stores nothing */
    setup_words(6, 7, 0, 0x6B40);
    CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_DIVIDE_ERROR && !rep.stored && st.window1[0x40] == 0 && st.pc == 0x990Au);

    /* loader failure and slot value failures */
    { static const uint8_t trunc[] = { 0x04, 0x02, 0x01 };
      memset(&st, 0, sizeof st); st.script = trunc; st.script_size = sizeof trunc; st.pc = 0x9900u; st.latch = 4;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && !rep.stored); }
    { static const uint8_t c4[] = { 0x04, 0x01, 0xB2, 0x00, 0x00, 0x01, 0x01, 0x10, 0x97 };               /* slot 1: unsupported class-4 */
      memset(&st, 0, sizeof st); st.script = c4; st.script_size = sizeof c4; st.pc = 0x9900u; st.latch = 4; st.window2[0x10] = 0xAAAA;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && !rep.stored && st.window2[0x10] == 0xAAAA); }
    { static const uint8_t c4[] = { 0x04, 0x00, 0x01, 0x01, 0xB2, 0x00, 0x01, 0x10, 0x97 };               /* slot 2 */
      memset(&st, 0, sizeof st); st.script = c4; st.script_size = sizeof c4; st.pc = 0x9900u; st.latch = 4;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && rep.v1 == 1 && !rep.stored); }
    /* class-1 slot reads: the handler completes, then reports HOOK */
    { static const uint8_t c1[] = { 0x04, 0x01, 0x18, 0x6B, 0x00, 0x02, 0x01, 0x10, 0x97 };
      memset(&st, 0, sizeof st); st.script = c1; st.script_size = sizeof c1; st.pc = 0x9900u; st.latch = 4; st.window1[0x18] = 5;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status[0] == PR_REC_ECL_WINDOW_HOOK);
      CHECK(rep.stored && st.window2[0x10] == 7); }

    /* At 0x6B18 the original load hook supplies active record byte +0x14, not the
     * window word. It runs before arithmetic, including a zero-divisor fault. */
    { static const uint8_t source[] = { 0x06, 0x01, 0x18, 0x6B, 0x00, 0x03, 0x01, 0x10, 0x97 };
      uint8_t record[PR_REC_CHAR_RECORD_SIZE] = { 0 };
      memset(&st, 0, sizeof st); st.script = source; st.script_size = sizeof source;
      st.pc = 0x9900u; st.latch = 6; st.window1[0x18] = 10;
      st.active_record = record; st.active_record_size = sizeof record;
      record[0x14] = 7;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK);
      CHECK(rep.slot_status[0] == PR_REC_ECL_WINDOW_OK && rep.v1 == 7 && rep.v2 == 3);
      CHECK(rep.result == 2 && rep.stored && st.window2[0x10] == 2 && st.window1[0x18] == 10 && st.pc == 0x9909u);
      st.pc = 0x9900u; st.active_record = NULL; st.window2[0x10] = 0;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_CORE);
      CHECK(rep.slot_status[0] == PR_REC_ECL_WINDOW_HOOK && rep.v1 == 10 && st.window2[0x10] == 3);
      st.pc = 0x9900u; st.active_record = record; st.active_record_size = 0x14u; st.window2[0x10] = 0xAAAA;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_CORE);
      CHECK(rep.slot_status[0] == PR_REC_ECL_WINDOW_BOUNDS && !rep.stored && st.window2[0x10] == 0xAAAA); }
    { static const uint8_t divisor[] = { 0x06, 0x00, 0x15, 0x01, 0x18, 0x6B, 0x01, 0x10, 0x97 };
      uint8_t record[PR_REC_CHAR_RECORD_SIZE] = { 0 };
      memset(&st, 0, sizeof st); st.script = divisor; st.script_size = sizeof divisor;
      st.pc = 0x9900u; st.latch = 6; st.window1[0x18] = 3;
      st.active_record = record; st.active_record_size = sizeof record;
      record[0x14] = 7;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_OK);
      CHECK(rep.v1 == 21 && rep.v2 == 7 && rep.result == 3 && st.window2[0x10] == 3);
      st.pc = 0x9900u; st.window2[0x10] = 0xAAAA; record[0x14] = 0;
      CHECK(pr_u1c_op_arith(&st, 0, &rep) == PR_U1C_ARITH_DIVIDE_ERROR);
      CHECK(rep.v2 == 0 && rep.runtime_error == 200 && !rep.stored && st.window2[0x10] == 0xAAAA); }

    /* a latch outside 4..7: the stored word is the host's stack residue, status RESIDUE */
    setup_words(8, 1, 2, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0x1234, &rep) == PR_U1C_ARITH_RESIDUE && st.window2[0x10] == 0x1234 && rep.stored);
    setup_words(3, 1, 2, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0xFFFF, &rep) == PR_U1C_ARITH_RESIDUE && st.window2[0x10] == 0xFFFF);

    CHECK(pr_u1c_op_arith(NULL, 0, &rep) == PR_U1C_ARITH_ARGUMENT);
    setup_words(4, 1, 2, 0x9710);
    CHECK(pr_u1c_op_arith(&st, 0, NULL) == PR_U1C_ARITH_OK);
    puts("overlay_u1c_arith_01a3: ok");
    return 0;
}
