/* Synthetic test for the unit 0x1c +0x0E5E GETTABLE handler model. Scripts, tables and state are fabricated. The
 * element is read through the lane-A load word at (index + base): a table in window 0, window 2 and the script
 * buffer, the low-byte-only index, the 16-bit address wrap, slot 1 used as an address word whatever its tag,
 * class-1 and class-4 element addresses, destination classes, and the loader and slot failure paths. */
#include "poolrad/recovered/overlay_u1c_gettable_0e5e.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static pr_rec_ecl_loader_state st;
static uint8_t script[0x200];

/* opcode, slot 1 = word address base (tag 1), slot 2 = literal index byte (tag 0), slot 3 = word destination (tag 1) */
static size_t build(uint16_t base, uint8_t idx, uint16_t dest)
{
    size_t o = 0;
    script[o++] = 0x2A;
    script[o++] = 0x01; script[o++] = (uint8_t)(base & 0xFF); script[o++] = (uint8_t)(base >> 8);
    script[o++] = 0x00; script[o++] = idx;
    script[o++] = 0x01; script[o++] = (uint8_t)(dest & 0xFF); script[o++] = (uint8_t)(dest >> 8);
    return o;
}

static void setup(size_t len)
{
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = len; st.pc = 0x9900u; st.latch = 0x2A;
    st.window2[0x10] = 0xAAAA;
}

int main(void)
{
    pr_u1c_gettable_report rep;
    unsigned k;
    size_t len;

    /* a table in window 0 (class 0, base 0x4900 + 0x40) read for every index byte */
    for (k = 0; k < 256; k++) {
        len = build(0x4940, (uint8_t)k, 0x9710); setup(len);
        st.window0[0x40 + k] = (uint16_t)(0x1000 + k * 3);
        CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_OK);
        CHECK(rep.base == 0x4940 && rep.index == k && rep.addr == 0x4940 + k && rep.value == (uint16_t)(0x1000 + k * 3));
        CHECK(rep.dest == 0x9710 && st.window2[0x10] == rep.value && rep.loaded && rep.stored && st.pc == 0x9909u);
    }
    /* a table in window 2 (class 2) */
    len = build(0x9710, 3, 0x4910); setup(len); st.window2[0x13] = 0x4242;
    CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_OK && st.window0[0x10] == 0x4242 && rep.addr == 0x9713);
    /* a table in the script buffer (class 3): the load word returns the zero-extended script BYTE at the address */
    len = build(0x9900 + 0x80, 4, 0x9710); setup(len); st.script_size = sizeof script;      /* the whole buffer is script space */
    script[0x80 + 4] = 0x34; script[0x80 + 5] = 0x12;
    CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_OK && rep.addr == 0x9984 && rep.value == 0x0034 && st.window2[0x10] == 0x0034);
    /* the index is the low byte only: a word slot value 0x01FF indexes 0xFF */
    { static const uint8_t w[] = { 0x2A, 0x01, 0x40, 0x49, 0x02, 0xFF, 0x01, 0x01, 0x10, 0x97 };
      memset(&st, 0, sizeof st); st.script = w; st.script_size = sizeof w; st.pc = 0x9900u; st.latch = 0x2A; st.window0[0x40 + 0xFF] = 0x7777; st.window0[0x40 + 0x1FF] = 0x8888;
      CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_OK && rep.index == 0xFF && rep.value == 0x7777 && st.window2[0x10] == 0x7777); }
    /* slot 1 is an address word whatever its tag: a literal byte tag supplies the low byte, the loader state's stale high byte the rest */
    { static const uint8_t lit[] = { 0x2A, 0x00, 0x40, 0x00, 0x02, 0x01, 0x10, 0x97 };
      memset(&st, 0, sizeof st); st.script = lit; st.script_size = sizeof lit; st.pc = 0x9900u; st.latch = 0x2A; st.high[1] = 0x49; st.window0[0x40 + 2] = 0x5151;
      CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_OK && rep.base == 0x4940 && rep.value == 0x5151); }
    /* 16-bit address wrap: 0xFFF0 + 0x20 = 0x0010 (the report shows the wrapped address) */
    len = build(0xFFF0, 0x20, 0x9710); setup(len);
    { pr_u1c_gettable_status s = pr_u1c_op_gettable(&st, &rep); (void)s; CHECK(rep.addr == 0x0010 && rep.loaded); }
    /* class-1 element addresses: the word is read and stored, then HOOK (exempt addresses are OK) */
    len = build(0x6B18, 0, 0x9710); setup(len); st.window1[0x18] = 0x0909;
    CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.value == 0x0909 && st.window2[0x10] == 0x0909 && rep.stored);
    len = build(0x6E7F, 0, 0x9710); setup(len); st.window1[0x37F] = 0x0A0A;
    CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_OK && st.window2[0x10] == 0x0A0A);
    /* class-4 element addresses: 0xC04F is [0x6AB1] (resolved); an unsupported one stops before the store */
    len = build(0xC04F, 0, 0x9710); setup(len); st.b6ab1 = 0x42;
    CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_OK && rep.value == 0x42 && st.window2[0x10] == 0x42);
    len = build(0x00B2, 0, 0x9710); setup(len);
    CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && rep.loaded && !rep.stored && st.window2[0x10] == 0xAAAA);
    /* destination classes */
    len = build(0x4940, 1, 0x6E82); setup(len); st.window0[0x41] = 0x0C0C;
    CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_OK && st.window1[0x382] == 0x0C0C);
    len = build(0x4940, 1, 0x6B40); setup(len); st.window0[0x41] = 0x0C0C;
    CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && st.window1[0x40] == 0x0C0C && rep.stored);
    len = build(0x4940, 1, 0x00B2); setup(len); st.window0[0x41] = 0x0C0C;
    CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_CORE && rep.core_status != PR_REC_ECL_HANDLER_OK && rep.stored);

    /* loader failure; a slot 2 failure stops before the element load; a class-1 slot 2 completes then HOOKs */
    { static const uint8_t trunc[] = { 0x2A, 0x01, 0x40, 0x49 };
      memset(&st, 0, sizeof st); st.script = trunc; st.script_size = sizeof trunc; st.pc = 0x9900u; st.latch = 0x2A;
      CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && !rep.loaded); }
    { static const uint8_t c4[] = { 0x2A, 0x01, 0x40, 0x49, 0x01, 0xB2, 0x00, 0x01, 0x10, 0x97 };
      memset(&st, 0, sizeof st); st.script = c4; st.script_size = sizeof c4; st.pc = 0x9900u; st.latch = 0x2A; st.window2[0x10] = 0xAAAA;
      CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_CORE && rep.core_status == PR_REC_ECL_HANDLER_UNRESOLVED && !rep.loaded && !rep.stored && st.window2[0x10] == 0xAAAA); }
    { static const uint8_t c1[] = { 0x2A, 0x01, 0x40, 0x49, 0x01, 0x18, 0x6B, 0x01, 0x10, 0x97 };
      memset(&st, 0, sizeof st); st.script = c1; st.script_size = sizeof c1; st.pc = 0x9900u; st.latch = 0x2A; st.window1[0x18] = 3; st.window0[0x40 + 3] = 0x3333;
      CHECK(pr_u1c_op_gettable(&st, &rep) == PR_U1C_GETTABLE_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && rep.slot_status == PR_REC_ECL_WINDOW_HOOK);
      CHECK(rep.index == 3 && rep.value == 0x3333 && st.window2[0x10] == 0x3333 && rep.stored); }

    CHECK(pr_u1c_op_gettable(NULL, &rep) == PR_U1C_GETTABLE_ARGUMENT);
    len = build(0x4940, 0, 0x9710); setup(len);
    CHECK(pr_u1c_op_gettable(&st, NULL) == PR_U1C_GETTABLE_OK);
    puts("overlay_u1c_gettable_0e5e: ok");
    return 0;
}
