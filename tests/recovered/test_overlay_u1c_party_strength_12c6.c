/* Synthetic test for the unit 0x1c +0x12C6 PARTYSTRENGTH handler model. The party list, records, script and
 * window state are fabricated. An independent restatement (integer arithmetic with max() instead of the original's
 * compare-and-subtract) is compared with the model over the full product of boundary values for one node and
 * over pseudo-random lists; the byte wrap, the 16-bit offset wrap, host guards, destination classes and
 * loader failure are checked individually. */
#include "poolrad/recovered/overlay_u1c_party_strength_12c6.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define MAXN 8
static pr_rec_ecl_loader_state st;
static uint8_t seg[MAXN][0x200];
static pr_u68_0236_node node[MAXN];
static uint8_t big[0x10000];
static pr_u68_0236_node bignode;

static void make_list(unsigned len)
{
    unsigned i;
    for (i = 0; i < MAXN; i++) {
        node[i].segment = seg[i]; node[i].segment_size = sizeof seg[i]; node[i].offset = 0;
        node[i].next = i + 1 < len ? &node[i + 1] : NULL;
    }
}

static void set_rec(unsigned i, uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e)
{
    seg[i][PR_U1C_PSTR_A] = a; seg[i][PR_U1C_PSTR_B] = b; seg[i][PR_U1C_PSTR_C] = c;
    seg[i][PR_U1C_PSTR_D] = d; seg[i][PR_U1C_PSTR_E] = e;
}

static int ref(int a, int b, int c, int d, int e)
{
    int bb = b - 0x3C > 0 ? b - 0x3C : 0;
    int cc = c - 0x27 > 0 ? c - 0x27 : 0;
    return (a + 5 * bb + 5 * cc + 8 * d + 4 * e) / 10;
}

static void setup(uint16_t dest)
{
    static uint8_t script[4];
    script[0] = 0x1D; script[1] = 0x01; script[2] = (uint8_t)(dest & 0xFF); script[3] = (uint8_t)(dest >> 8);
    memset(&st, 0, sizeof st);
    st.script = script; st.script_size = sizeof script; st.pc = 0x9900u; st.latch = 0x1D;
    st.window2[0x10] = 0xAAAA;
}

int main(void)
{
    pr_u1c_pstr_report rep;
    static const int vals[] = { 0, 1, 0x26, 0x27, 0x28, 0x3B, 0x3C, 0x3D, 0x7F, 0x80, 0xFF };
    const unsigned nv = sizeof vals / sizeof vals[0];
    unsigned ia, ib, ic, id, ie, t, i, len;
    unsigned long seed = 12345;

    /* one node, full product of boundary values for the five fields */
    for (ia = 0; ia < nv; ia++) for (ib = 0; ib < nv; ib++) for (ic = 0; ic < nv; ic++)
    for (id = 0; id < nv; id++) for (ie = 0; ie < nv; ie++) {
        make_list(1);
        set_rec(0, (uint8_t)vals[ia], (uint8_t)vals[ib], (uint8_t)vals[ic], (uint8_t)vals[id], (uint8_t)vals[ie]);
        setup(0x9710);
        CHECK(pr_u1c_op_party_strength(&st, &node[0], &rep) == PR_U1C_PSTR_OK);
        CHECK(rep.sum == (uint8_t)ref(vals[ia], vals[ib], vals[ic], vals[id], vals[ie]) && rep.visited == 1);
        CHECK(st.window2[0x10] == rep.sum && rep.dest == 0x9710 && rep.stored && st.pc == 0x9904u);
    }
    /* pseudo-random lists of 0..7 nodes: the byte sum of the per-node quotients */
    for (t = 0; t < 3000; t++) {
        int sum = 0;
        len = (unsigned)(seed >> 8) % (MAXN + 1);
        make_list(len);
        for (i = 0; i < MAXN; i++) {
            int f[5]; unsigned k;
            for (k = 0; k < 5; k++) { seed = seed * 1103515245UL + 12345UL; f[k] = (int)((seed >> 16) & 0xFF); }
            set_rec(i, (uint8_t)f[0], (uint8_t)f[1], (uint8_t)f[2], (uint8_t)f[3], (uint8_t)f[4]);
            if (i < len) sum += ref(f[0], f[1], f[2], f[3], f[4]);
        }
        setup(0x9710);
        CHECK(pr_u1c_op_party_strength(&st, len ? &node[0] : NULL, &rep) == PR_U1C_PSTR_OK);
        CHECK(rep.visited == len && rep.sum == (uint8_t)(sum & 0xFF) && st.window2[0x10] == (uint16_t)(sum & 0xFF));
    }
    /* maximal values: each node adds 537 (5,370 / 10), so 40 nodes sum to 21,480, so the byte accumulator wraps */
    {
        static pr_u68_0236_node many[40]; static uint8_t mseg[0x200];
        memset(mseg, 0, sizeof mseg); mseg[PR_U1C_PSTR_A] = 0xFF; mseg[PR_U1C_PSTR_B] = 0xFF; mseg[PR_U1C_PSTR_C] = 0xFF;
        mseg[PR_U1C_PSTR_D] = 0xFF; mseg[PR_U1C_PSTR_E] = 0xFF;
        for (i = 0; i < 40; i++) { many[i].segment = mseg; many[i].segment_size = sizeof mseg; many[i].offset = 0; many[i].next = i + 1 < 40 ? &many[i + 1] : NULL; }
        CHECK(ref(0xFF, 0xFF, 0xFF, 0xFF, 0xFF) == 537);
        setup(0x9710);
        CHECK(pr_u1c_op_party_strength(&st, &many[0], &rep) == PR_U1C_PSTR_OK && rep.visited == 40);
        CHECK(rep.sum == (uint8_t)(40 * ref(0xFF, 0xFF, 0xFF, 0xFF, 0xFF)) && st.window2[0x10] == rep.sum);
    }
    /* head nil: sum 0 stored */
    setup(0x9710);
    CHECK(pr_u1c_op_party_strength(&st, NULL, &rep) == PR_U1C_PSTR_OK && rep.visited == 0 && rep.sum == 0 && st.window2[0x10] == 0);

    /* 16-bit offset wrap: node at far offset 0xFF00 in a 64 KiB view: fields at (0xFF00 + k) mod 65536 */
    memset(big, 0, sizeof big);
    bignode.segment = big; bignode.segment_size = sizeof big; bignode.offset = 0xFF00; bignode.next = NULL;
    big[(uint16_t)(0xFF00 + PR_U1C_PSTR_A)] = 10; big[(uint16_t)(0xFF00 + PR_U1C_PSTR_B)] = 0x3C + 4;
    big[(uint16_t)(0xFF00 + PR_U1C_PSTR_C)] = 0x27 + 3; big[(uint16_t)(0xFF00 + PR_U1C_PSTR_D)] = 5; big[(uint16_t)(0xFF00 + PR_U1C_PSTR_E)] = 2;
    setup(0x9710);
    CHECK(pr_u1c_op_party_strength(&st, &bignode, &rep) == PR_U1C_PSTR_OK && rep.sum == (10 + 20 + 15 + 40 + 8) / 10);

    /* host guards: a null or too small segment, a cyclic list; nothing stored, the loader state retained */
    make_list(2); set_rec(0, 1, 1, 1, 1, 1); node[1].segment = NULL; setup(0x9710);
    CHECK(pr_u1c_op_party_strength(&st, &node[0], &rep) == PR_U1C_PSTR_LIST && !rep.stored && st.window2[0x10] == 0xAAAA && st.pc == 0x9904u && rep.visited == 1);
    make_list(1); node[0].segment_size = PR_U1C_PSTR_A; setup(0x9710);
    CHECK(pr_u1c_op_party_strength(&st, &node[0], &rep) == PR_U1C_PSTR_LIST && !rep.stored && st.window2[0x10] == 0xAAAA);
    make_list(1); set_rec(0, 1, 1, 1, 1, 1); node[0].next = &node[0]; setup(0x9710);
    CHECK(pr_u1c_op_party_strength(&st, &node[0], &rep) == PR_U1C_PSTR_LIST && !rep.stored && rep.visited == PR_U1C_PSTR_LIST_CAP);

    /* destination classes through the store word */
    make_list(1); set_rec(0, 100, 0, 0, 0, 0);                               /* 100 / 10 = 10 */
    setup(0x6E82);
    CHECK(pr_u1c_op_party_strength(&st, &node[0], &rep) == PR_U1C_PSTR_OK && st.window1[0x382] == 10 && rep.store_status == PR_REC_ECL_WINDOW_OK);
    setup(0x6B40);                                                              /* a class-1 address whose store hook is not exempt */
    CHECK(pr_u1c_op_party_strength(&st, &node[0], &rep) == PR_U1C_PSTR_CORE && rep.core_status == PR_REC_ECL_HANDLER_HOOK && st.window1[0x40] == 10 && rep.stored);
    setup(0x4910);
    CHECK(pr_u1c_op_party_strength(&st, &node[0], &rep) == PR_U1C_PSTR_OK && st.window0[0x10] == 10);
    setup(0x00B2);                                                              /* a class-4 destination: unsupported store */
    CHECK(pr_u1c_op_party_strength(&st, &node[0], &rep) == PR_U1C_PSTR_CORE && rep.core_status != PR_REC_ECL_HANDLER_OK && rep.stored);

    /* loader failure: the list is not walked (a bad node is never reached), nothing stored */
    make_list(1); node[0].segment = NULL;
    { static const uint8_t trunc[] = { 0x1D, 0x01 };
      memset(&st, 0, sizeof st); st.script = trunc; st.script_size = sizeof trunc; st.pc = 0x9900u; st.window2[0x10] = 0xAAAA;
      CHECK(pr_u1c_op_party_strength(&st, &node[0], &rep) == PR_U1C_PSTR_CORE && rep.core_status == PR_REC_ECL_HANDLER_BOUNDS && !rep.stored && rep.visited == 0);
      CHECK(st.window2[0x10] == 0xAAAA); }

    CHECK(pr_u1c_op_party_strength(NULL, NULL, &rep) == PR_U1C_PSTR_ARGUMENT);
    make_list(1); set_rec(0, 5, 0, 0, 0, 0); setup(0x9710);
    CHECK(pr_u1c_op_party_strength(&st, &node[0], NULL) == PR_U1C_PSTR_OK);
    puts("overlay_u1c_party_strength_12c6: ok");
    return 0;
}
