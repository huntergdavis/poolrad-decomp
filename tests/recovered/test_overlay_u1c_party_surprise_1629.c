/* Synthetic test for the unit 0x1c +0x1629 PARTY SURPRISE handler model. The party list, records, script and window
 * state are fabricated. Every value of the +0x2F byte is checked (only 4 and 0x0A match), list orders and lengths against
 * an independent any-match restatement, the constant-zero second store, the store order and destination classes, the
 * 16-bit offset wrap, host guards, and loader failure. */
#include "poolrad/recovered/overlay_u1c_party_surprise_1629.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define MAXN 6
static pr_rec_ecl_loader_state st;
static uint8_t seg[MAXN][0x200];
static pr_u68_0236_node node[MAXN];
static uint8_t big[0x10000];

static void make_list(unsigned len)
{
    unsigned i;
    for (i = 0; i < MAXN; i++) {
        node[i].segment = seg[i]; node[i].segment_size = sizeof seg[i]; node[i].offset = 0;
        node[i].next = i + 1 < len ? &node[i + 1] : NULL;
        memset(seg[i], 0x55, sizeof seg[i]);
    }
}

/* opcode, two word destinations (tag 1) */
static void setup(uint16_t d1, uint16_t d2)
{
    static uint8_t script[7];
    script[0] = 0x22;
    script[1] = 0x01; script[2] = (uint8_t)(d1 & 0xFF); script[3] = (uint8_t)(d1 >> 8);
    script[4] = 0x01; script[5] = (uint8_t)(d2 & 0xFF); script[6] = (uint8_t)(d2 >> 8);
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = sizeof script; st.pc = 0x9900u; st.latch = 0x22;
    st.window2[0x10] = 0xAAAA; st.window2[0x11] = 0xBBBB;
}

int main(void)
{
    pr_u1c_psurp_report rep;
    unsigned v, len, i, mask;

    /* one node: every +0x2F value; only 4 and 0x0A give 1 */
    for (v = 0; v < 256; v++) {
        make_list(1); seg[0][PR_U1C_PSURP_FIELD] = (uint8_t)v; setup(0x9710, 0x9711);
        CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_OK && rep.visited == 1);
        CHECK(rep.flag == (v == 4 || v == 10) && st.window2[0x10] == (v == 4 || v == 10) && st.window2[0x11] == 0);
        CHECK(rep.dest1 == 0x9710 && rep.dest2 == 0x9711 && rep.stored1 && rep.stored2 && st.pc == 0x9907u);
    }
    /* lists of 0..5 nodes with every subset of matching nodes: the flag is the any-match, wherever the match sits */
    for (len = 0; len <= 5; len++) for (mask = 0; mask < (1u << len); mask++) {
        make_list(len);
        for (i = 0; i < len; i++) seg[i][PR_U1C_PSURP_FIELD] = (mask >> i & 1u) ? (i & 1 ? 4 : 10) : (uint8_t)(i + 5);   /* non-matching: 5..9 */
        setup(0x9710, 0x9711);
        CHECK(pr_u1c_op_party_surprise(&st, len ? &node[0] : NULL, &rep) == PR_U1C_PSURP_OK && rep.visited == len);
        CHECK(rep.flag == (mask != 0) && st.window2[0x10] == (mask != 0) && st.window2[0x11] == 0);
    }
    /* the matching value of the neighbouring bytes does not matter: 4 and 10 at other offsets are ignored */
    make_list(1); seg[0][PR_U1C_PSURP_FIELD - 1] = 4; seg[0][PR_U1C_PSURP_FIELD + 1] = 10; seg[0][PR_U1C_PSURP_FIELD] = 5; setup(0x9710, 0x9711);
    CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_OK && rep.flag == 0);

    /* 16-bit offset wrap in a 64 KiB view: record at far offset 0xFFF0, +0x2F at 0x001F */
    memset(big, 0, sizeof big);
    { pr_u68_0236_node b; b.segment = big; b.segment_size = sizeof big; b.offset = 0xFFF0; b.next = NULL;
      big[(uint16_t)(0xFFF0 + PR_U1C_PSURP_FIELD)] = 4; setup(0x9710, 0x9711);
      CHECK(pr_u1c_op_party_surprise(&st, &b, &rep) == PR_U1C_PSURP_OK && rep.flag == 1 && (uint16_t)(0xFFF0 + 0x2F) == 0x001F); }

    /* store order and destination classes: the first store is the flag, the second always 0 */
    make_list(1); seg[0][PR_U1C_PSURP_FIELD] = 10; setup(0x6E82, 0x4910);
    CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_OK && st.window1[0x382] == 1 && st.window0[0x10] == 0);
    setup(0x6B20, 0x9710);                                           /* class-1 potential-effect address: stored then HOOK, second store still done */
    CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK);
    CHECK(rep.stored1 && rep.stored2 && st.window1[0x20] == 1 && st.window2[0x10] == 0);
    setup(0x9710, 0x6B20);
    CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && st.window2[0x10] == 1 && st.window1[0x20] == 0);
    setup(0x00B2, 0x9710);                                           /* an unsupported first destination stops before the second store */
    CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_CORE && rep.core_status != PR_REC_ECL_HANDLER_OK && rep.stored1 && !rep.stored2 && st.window2[0x10] == 0xAAAA);
    setup(0x9710, 0x00B2);                                           /* an unsupported second destination: the first store stands */
    CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_CORE && rep.core_status != PR_REC_ECL_HANDLER_OK && rep.stored2 && st.window2[0x10] == 1);

    /* destinations are operand words whatever the tag: literal byte tags take the stale high byte */
    { static const uint8_t lit[] = { 0x22, 0x00, 0x10, 0x00, 0x11 };
      make_list(1); seg[0][PR_U1C_PSURP_FIELD] = 4;
      memset(&st, 0, sizeof st); st.script = lit; st.script_size = sizeof lit; st.pc = 0x9900u; st.latch = 0x22; st.high[1] = 0x97; st.high[2] = 0x97;
      CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_OK && rep.dest1 == 0x9710 && rep.dest2 == 0x9711 && st.window2[0x10] == 1); }

    /* host guards: nothing is stored, the loader state is kept */
    make_list(2); node[1].segment = NULL; seg[0][PR_U1C_PSURP_FIELD] = 4; setup(0x9710, 0x9711);
    CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_LIST && !rep.stored1 && st.window2[0x10] == 0xAAAA && st.pc == 0x9907u && rep.visited == 1);
    make_list(1); node[0].segment_size = PR_U1C_PSURP_FIELD; setup(0x9710, 0x9711);
    CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_LIST && !rep.stored1);
    make_list(1); node[0].next = &node[0]; setup(0x9710, 0x9711);
    CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_LIST && rep.visited == PR_U1C_PSURP_LIST_CAP && !rep.stored1);

    /* loader failure: the list is not reached */
    { static const uint8_t trunc[] = { 0x22, 0x01, 0x10 };
      make_list(1); node[0].segment = NULL;
      memset(&st, 0, sizeof st); st.script = trunc; st.script_size = sizeof trunc; st.pc = 0x9900u; st.latch = 0x22;
      CHECK(pr_u1c_op_party_surprise(&st, &node[0], &rep) == PR_U1C_PSURP_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && rep.visited == 0 && !rep.stored1); }

    CHECK(pr_u1c_op_party_surprise(NULL, NULL, &rep) == PR_U1C_PSURP_ARGUMENT);
    make_list(0); setup(0x9710, 0x9711);
    CHECK(pr_u1c_op_party_surprise(&st, NULL, NULL) == PR_U1C_PSURP_OK && st.window2[0x10] == 0);
    puts("overlay_u1c_party_surprise_1629: ok");
    return 0;
}
