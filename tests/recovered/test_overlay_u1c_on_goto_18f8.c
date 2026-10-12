/* Synthetic test for the unit 0x1c +0x18F8 ON GOTO / ON GOSUB handler model. Scripts, targets and state are
 * fabricated. An independent restatement of the selection rule (first index equal to the selector, only below the
 * count) is compared over a grid of selectors and counts for both latches; the count and stack bounds, the
 * loader and slot-value failure paths, a class-1 slot read and the stale high byte are checked individually. */
#include "poolrad/recovered/overlay_u1c_on_goto_18f8.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;
static uint8_t script[512];

/* opcode byte, selector (literal), count (literal), then n word targets 0xA000 + 0x11*k */
static size_t build(uint8_t sel, uint8_t n)
{
    size_t o = 0, k;
    script[o++] = 0x25;
    script[o++] = 0x00; script[o++] = sel;
    script[o++] = 0x00; script[o++] = n;
    for (k = 0; k < n; k++) {
        uint16_t t = (uint16_t)(0xA000u + 0x11u * k);
        script[o++] = 0x01; script[o++] = (uint8_t)(t & 0xFFu); script[o++] = (uint8_t)(t >> 8);
    }
    return o;
}

static void setup(size_t len, uint8_t latch)
{
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = len; st.pc = 0x9900u; st.latch = latch;
}

int main(void)
{
    pr_u1c_on_goto_report rep;
    unsigned sel, n, latch_i;
    static const uint8_t latches[2] = { 0x25, 0x26 };

    for (latch_i = 0; latch_i < 2; latch_i++) {
        uint8_t latch = latches[latch_i];
        for (n = 0; n <= 7; n++) {
            for (sel = 0; sel <= 9; sel++) {
                size_t len = build((uint8_t)sel, (uint8_t)n);
                uint16_t end = (uint16_t)(0x9905u + 3u * n);       /* the byte after the last operand */
                int match = sel < n;
                setup(len, latch);
                CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_OK && rep.core_status == PR_REC_ECL_HANDLER_OK);
                CHECK(rep.selector == sel && rep.count == n && rep.selected == (unsigned)match);
                if (!match) {
                    CHECK(st.pc == end && st.call_depth == 0);
                } else if (latch == 0x25) {
                    CHECK(rep.went_goto && st.pc == 0xA000u + 0x11u * sel && st.call_depth == 0);
                } else {
                    CHECK(!rep.went_goto && st.pc == 0xA000u + 0x11u * sel && st.call_depth == 1 && st.call_stack[0] == end);
                }
            }
        }
    }
    /* every other latch takes the call-frame arm */
    { size_t len = build(1, 3); setup(len, 0x40);
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_OK && st.call_depth == 1 && st.pc == 0xA011u); }

    /* a count of 63 works, the bound is 64 */
    { size_t len = build(62, 63); setup(len, 0x25);
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_OK && st.pc == 0xA000u + 0x11u * 62u); }
    { size_t len = build(0, 64); setup(len, 0x25);
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_CORE && rep.core_status == PR_REC_ECL_HANDLER_ARGUMENT);
      CHECK(st.pc == 0x9904u && rep.selector == 0 && rep.count == 64); }          /* the decrement already happened */

    /* call stack full: the GOSUB stops, the PC stays at the first-loader-then-second-loader end */
    { size_t len = build(1, 3); setup(len, 0x26); st.call_depth = PR_REC_ECL_CALL_STACK_MAX;
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_CORE && rep.core_status == PR_REC_ECL_HANDLER_STACK_FULL);
      CHECK(st.pc == 0x9905u + 9u && st.call_depth == PR_REC_ECL_CALL_STACK_MAX && rep.selected); }

    /* first loader runs out of script: nothing else happens; the second loader runs out */
    { build(1, 3); setup(3, 0x25);
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && rep.count == 0); }
    { build(1, 3); setup(5 + 6, 0x25);                                            /* two of the three targets only */
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && rep.count == 3 && st.call_depth == 0); }

    /* slot value failures: a class-4 source in slot 1 or 2 stops after the first loader, before the PC decrement */
    { static const uint8_t c4[] = { 0x25, 0x01, 0xB2, 0x00, 0x00, 0x02 };
      memset(&st, 0, sizeof st); st.script = c4; st.script_size = sizeof c4; st.pc = 0x9900u; st.latch = 0x25;
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && st.pc == 0x9906u); }
    { static const uint8_t c4[] = { 0x25, 0x00, 0x00, 0x01, 0xB2, 0x00 };
      memset(&st, 0, sizeof st); st.script = c4; st.script_size = sizeof c4; st.pc = 0x9900u; st.latch = 0x25;
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && st.pc == 0x9906u); }

    /* a class-1 selector (the load hook model): the handler completes the selection, then reports HOOK */
    { static const uint8_t c1[] = { 0x25, 0x01, 0x40, 0x6B, 0x00, 0x02, 0x01, 0x11, 0xA0, 0x01, 0x22, 0xA0 };
      memset(&st, 0, sizeof st); st.script = c1; st.script_size = sizeof c1; st.pc = 0x9900u; st.latch = 0x25; st.window1[0x40] = 1;
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK);
      CHECK(rep.slot_status[0] == PR_REC_ECL_WINDOW_HOOK && rep.selected && st.pc == 0xA022u); }

    /* a slot with a literal tag keeps the stale high byte left in the loader state */
    { static const uint8_t lit[] = { 0x25, 0x00, 0x00, 0x00, 0x01, 0x00, 0x34 };
      memset(&st, 0, sizeof st); st.script = lit; st.script_size = sizeof lit; st.pc = 0x9900u; st.latch = 0x25; st.high[1] = 0x12;
      CHECK(pr_u1c_op_on_goto(&st, &rep) == PR_U1C_ON_GOTO_OK && st.pc == 0x1234u); }

    CHECK(pr_u1c_op_on_goto(NULL, &rep) == PR_U1C_ON_GOTO_ARGUMENT);
    { size_t len = build(1, 3); setup(len, 0x25); CHECK(pr_u1c_op_on_goto(&st, NULL) == PR_U1C_ON_GOTO_OK); }
    puts("overlay_u1c_on_goto_18f8: ok");
    return 0;
}
