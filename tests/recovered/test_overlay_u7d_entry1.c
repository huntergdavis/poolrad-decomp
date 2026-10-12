/* Synthetic parity test for the unit 0x7d entry 1 model. Window 0, the
 * limit table, the list nodes and the update hook are fabricated. An
 * independent word-at-a-time restatement is compared with the model over
 * every (n, idx) pair for several starting word sets. */
#include "poolrad/recovered/overlay_u7d_entry1.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define WIN0 0x400u

typedef struct { unsigned calls; uint16_t n, idx; uint8_t seen[14]; const uint8_t *win; } rec;

static void on_update(void *ctx, uint16_t n, uint16_t idx)
{
    rec *r = ctx;
    r->calls++; r->n = n; r->idx = idx;
    memcpy(r->seen, r->win + PR_U7D_WINDOW0_OFFSET, 14);   /* write-back precedes the call */
}

/* raw restatement: counters are bumped once per walk per node */
static void ref_run(uint16_t w[7], const uint16_t lim[7], unsigned n, unsigned idx, unsigned *walks)
{
    unsigned k, i;
    for (k = 0; k < n; k++) {
        w[idx]++;
        for (i = 0; i < 7; i++) {
            if (w[i] >= lim[i]) {
                if (i < 6) { w[i + 1]++; w[i] = (uint16_t)(w[i] - lim[i]); }
                else (*walks)++;
            }
        }
    }
}

static uint16_t rd(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static int run_case(const uint16_t start[7], const uint16_t lim[7], unsigned n, unsigned idx)
{
    uint8_t win[WIN0];
    uint8_t recs[3][0x40];
    pr_u7d_node nodes[3];
    pr_u7d_state st;
    pr_u7d_report rep;
    rec rc;
    uint16_t exp[7];
    unsigned walks = 0, i;
    memset(win, 0xC3, sizeof win);
    for (i = 0; i < 7; i++) { win[PR_U7D_WINDOW0_OFFSET + 2 * i] = (uint8_t)(start[i] & 0xFF); win[PR_U7D_WINDOW0_OFFSET + 2 * i + 1] = (uint8_t)(start[i] >> 8); }
    memset(recs, 0x11, sizeof recs);
    recs[1][0x30] = 0xFF; recs[1][0x31] = 0xFF;    /* one counter wraps */
    for (i = 0; i < 3; i++) { nodes[i].record = recs[i]; nodes[i].record_size = 0x40; nodes[i].next = i < 2 ? &nodes[i + 1] : NULL; }
    memset(&st, 0, sizeof st);
    st.window0 = win; st.window0_size = sizeof win;
    memcpy(st.limit, lim, sizeof st.limit);
    st.list_head = &nodes[0]; st.update = on_update; st.ctx = &rc;
    memset(&rc, 0, sizeof rc); rc.win = win;
    memcpy(exp, start, sizeof exp);
    ref_run(exp, lim, n, idx, &walks);
    CHECK(pr_u7d_entry1(&st, (uint8_t)n, (uint8_t)idx, &rep) == PR_U7D_OK);
    for (i = 0; i < 7; i++) CHECK(rd(win + PR_U7D_WINDOW0_OFFSET + 2 * i) == exp[i] && rep.words_out[i] == exp[i]);
    for (i = 0; i < WIN0; i++)
        if (i < PR_U7D_WINDOW0_OFFSET || i >= PR_U7D_WINDOW0_OFFSET + 14) CHECK(win[i] == 0xC3);
    CHECK(rep.iterations == n && rep.list_walks == walks && rep.nodes_bumped == walks * 3);
    CHECK(rc.calls == 1 && rc.n == (0x9300u | n) && rc.idx == (0x9300u | idx));
    CHECK(rep.update_n_word == rc.n && rep.update_idx_word == rc.idx);
    CHECK(memcmp(rc.seen, win + PR_U7D_WINDOW0_OFFSET, 14) == 0);
    CHECK(rd(recs[0] + 0x30) == (uint16_t)(0x1111u + walks));
    CHECK(rd(recs[1] + 0x30) == (uint16_t)(0xFFFFu + walks));
    CHECK(rd(recs[2] + 0x30) == (uint16_t)(0x1111u + walks));
    for (i = 0; i < 0x40; i++) if (i != 0x30 && i != 0x31) CHECK(recs[0][i] == 0x11 && recs[2][i] == 0x11);
    return 0;
}

int main(void)
{
    static const uint16_t lim[7] = { 60, 60, 24, 30, 12, 100, 3 };
    static const uint16_t lim0[7] = { 0, 1, 2, 3, 4, 5, 6 };
    static const uint16_t starts[][7] = {
        { 0, 0, 0, 0, 0, 0, 0 }, { 59, 59, 23, 29, 11, 99, 1 }, { 58, 59, 23, 29, 11, 99, 2 },
        { 100, 200, 30, 40, 20, 150, 7 }, { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF }
    };
    unsigned s, n, idx;
    for (s = 0; s < sizeof starts / sizeof starts[0]; s++)
        for (n = 0; n <= 6; n++)
            for (idx = 0; idx < 7; idx++)
                if (run_case(starts[s], lim, n, idx) || run_case(starts[s], lim0, n, idx)) return 1;
    for (n = 250; n <= 255; n++) if (run_case(starts[1], lim, n, 2)) return 1;   /* byte count upper range */

    /* pinned: (1, 1) on seconds-like word at its limit carries once; empty list is the null test */
    {
        uint8_t win[WIN0] = {0};
        uint16_t w[7] = { 0, 59, 23, 29, 11, 99, 2 };
        unsigned c[7] = {0}, walks = 0, bumped = 0;
        pr_u7d_state st;
        pr_u7d_report rep;
        win[PR_U7D_WINDOW0_OFFSET + 2] = 59;
        memset(&st, 0, sizeof st);
        st.window0 = win; st.window0_size = sizeof win; memcpy(st.limit, lim, sizeof st.limit);
        CHECK(pr_u7d_entry1(&st, 1, 1, &rep) == PR_U7D_OK);
        CHECK(rd(win + PR_U7D_WINDOW0_OFFSET + 2) == 0 && rd(win + PR_U7D_WINDOW0_OFFSET + 4) == 1 && rep.carries[1] == 1);
        CHECK(pr_u7d_carry_normalize(w, lim, NULL, c, &walks, &bumped) == PR_U7D_OK);
        CHECK(w[0] == 0 && w[1] == 59 && w[6] == 2 && walks == 0);   /* nothing at/over its limit */
        w[6] = 3;
        CHECK(pr_u7d_carry_normalize(w, lim, NULL, c, &walks, &bumped) == PR_U7D_OK);
        CHECK(w[6] == 3 && walks == 1 && bumped == 0);               /* w[6] is never reduced */
    }

    /* ARGUMENT: nothing written, no hook run */
    {
        uint8_t win[WIN0], recs[0x40], small[0x10];
        pr_u7d_node a, b;
        pr_u7d_state st;
        rec rc;
        memset(win, 0x55, sizeof win); memset(recs, 0, sizeof recs); memset(&rc, 0, sizeof rc);
        a.record = recs; a.record_size = sizeof recs; a.next = &b;
        b.record = small; b.record_size = sizeof small; b.next = NULL;
        memset(&st, 0, sizeof st);
        st.window0 = win; st.window0_size = sizeof win; st.list_head = &a; st.update = on_update; st.ctx = &rc; rc.win = win;
        CHECK(pr_u7d_entry1(&st, 1, 0, NULL) == PR_U7D_ARGUMENT && rc.calls == 0 && win[PR_U7D_WINDOW0_OFFSET] == 0x55);
        b.record_size = 0x32; b.next = &a;                           /* cycle */
        CHECK(pr_u7d_entry1(&st, 1, 0, NULL) == PR_U7D_ARGUMENT && rc.calls == 0);
        b.next = NULL; b.record = NULL;
        CHECK(pr_u7d_entry1(&st, 1, 0, NULL) == PR_U7D_ARGUMENT);
        b.record = small; b.record_size = 0x32; small[0] = 0;        /* nonsense size vs buffer is the host's contract */
        st.list_head = NULL;
        CHECK(pr_u7d_entry1(&st, 1, 7, NULL) == PR_U7D_ARGUMENT);    /* idx 7 would write past the local array */
        st.window0_size = PR_U7D_WINDOW0_MIN_SIZE - 1;
        CHECK(pr_u7d_entry1(&st, 1, 0, NULL) == PR_U7D_ARGUMENT);
        st.window0_size = sizeof win; st.window0 = NULL;
        CHECK(pr_u7d_entry1(&st, 1, 0, NULL) == PR_U7D_ARGUMENT);
        CHECK(pr_u7d_entry1(NULL, 1, 0, NULL) == PR_U7D_ARGUMENT);
        CHECK(pr_u7d_carry_normalize(NULL, lim, NULL, NULL, NULL, NULL) == PR_U7D_ARGUMENT);
    }
    puts("overlay_u7d_entry1: ok");
    return 0;
}
