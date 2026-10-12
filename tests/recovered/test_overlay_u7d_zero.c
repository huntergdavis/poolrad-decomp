/* Synthetic parity test for the unit 0x7d +0x0000 model. DS flags, limits,
 * window 1, the member/item lists and the unit 0x9e hook are fabricated.
 * An index-based restatement (items identified by id, lists as arrays) is
 * compared with the pointer-based model over many generated cases; the
 * original's head-restart quirk is pinned by hand. */
#include "poolrad/recovered/overlay_u7d_zero.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define MAXM 5
#define MAXI 6

typedef struct { int len; int ids[MAXI]; } rlist;            /* reference list of item ids */
typedef struct { uint8_t type[MAXM * MAXI]; uint16_t q[MAXM * MAXI]; rlist m[MAXM]; int nm; } world;

/* ---- reference (arrays) ---- */
static int r_next(const rlist *l, int id)
{
    int j;
    for (j = 0; j < l->len; j++) if (l->ids[j] == id) return j + 1 < l->len ? l->ids[j + 1] : -1;
    return -1;
}
static int r_head(const rlist *l) { return l->len ? l->ids[0] : -1; }
static void r_remove(rlist *l, int id)
{
    int j, k = 0;
    for (j = 0; j < l->len; j++) if (l->ids[j] != id) l->ids[k++] = l->ids[j];
    l->len = k;
}
static int r_snap(const rlist *l, int cur)
{
    int j;
    for (j = 0; j < l->len; j++) {
        if (l->ids[j] == cur) return l->ids[j];
        if (r_next(l, l->ids[j]) == cur) return l->ids[j];
    }
    return -2;   /* null */
}
static void r_member(world *w, uint8_t *flags, int mi, int fi, unsigned chunk, unsigned *hooks, int *removed, int *nrem)
{
    rlist *l = &w->m[mi];
    int tail = l->len ? l->ids[l->len - 1] : -1, cur = r_head(l), snap = r_head(l), done = 0;
    for (;;) {
        if (cur < 0 || done) break;
        if (cur == tail) done = 1;
        if (w->q[cur] == 0) cur = r_next(l, cur);
        else if (chunk < w->q[cur]) { w->q[cur] = (uint16_t)(w->q[cur] - chunk); flags[fi] = 1; cur = r_next(l, cur); }
        else {
            int saved = r_next(l, cur);
            (*hooks)++; removed[(*nrem)++] = cur; r_remove(l, cur);
            cur = (snap == r_head(l)) ? r_head(l) : saved;
        }
        snap = r_snap(l, cur);
        if (snap == -2) snap = -1;     /* null */
    }
    for (; cur >= 0; cur = r_next(l, cur)) if (w->q[cur]) flags[fi] = 1;
}
static void r_run(world *w, uint8_t *flags, uint8_t mode, uint8_t n, uint8_t idx, const uint16_t *lim,
                  unsigned *hooks, int *removed, int *nrem)
{
    unsigned rem = n, b, mi, fi;
    if (mode != 2) memset(flags + 1, 1, 0x48);
    for (b = (unsigned)(int8_t)idx; (int)b > 1; b--) rem = (uint16_t)(lim[b - 1] * rem);
    while (rem) {
        unsigned chunk = rem > 10 ? 10 : rem;
        for (mi = 0, fi = 1; (int)mi < w->nm; mi++, fi++) {
            if (!flags[fi & 0xFF]) continue;
            flags[fi & 0xFF] = 0;
            r_member(w, flags, (int)mi, (int)(fi & 0xFF), chunk, hooks, removed, nrem);
        }
        rem = rem > 10 ? rem - 10 : 0;
    }
}

/* ---- model world ---- */
typedef struct { pr_u7d0_item it[MAXM * MAXI]; pr_u7d0_member mem[MAXM]; int removed[64]; int nrem; pr_u7d0_item *base; } mworld;
static void on_remove(void *ctx, pr_u7d0_member *m, uint8_t type, pr_u7d0_item *item)
{
    mworld *mw = ctx;
    pr_u7d0_item **pp;
    mw->removed[mw->nrem++] = (int)(item - mw->base);
    (void)type;
    for (pp = &m->head; *pp; pp = &(*pp)->next)
        if (*pp == item) { *pp = item->next; break; }
}
static void build(const world *w, mworld *mw)
{
    int i, j;
    memset(mw, 0, sizeof *mw);
    mw->base = mw->it;
    for (i = 0; i < w->nm; i++) {
        pr_u7d0_item *prev = NULL;
        for (j = 0; j < w->m[i].len; j++) {
            int id = w->m[i].ids[j];
            mw->it[id].type = w->type[id]; mw->it[id].quantity = w->q[id];
            if (prev) prev->next = &mw->it[id]; else mw->mem[i].head = &mw->it[id];
            prev = &mw->it[id];
        }
        mw->mem[i].next = i + 1 < w->nm ? &mw->mem[i + 1] : NULL;
    }
}
static int same(const world *w, const mworld *mw)
{
    int i, j;
    for (i = 0; i < w->nm; i++) {
        const pr_u7d0_item *p = mw->mem[i].head;
        for (j = 0; j < w->m[i].len; j++, p = p->next) {
            if (!p || p != &mw->it[w->m[i].ids[j]] || p->quantity != w->q[w->m[i].ids[j]]) return 0;
        }
        if (p) return 0;
    }
    return 1;
}

static unsigned rng_s = 12345u;
static unsigned rnd(unsigned m) { rng_s = rng_s * 1103515245u + 12345u; return (rng_s >> 16) % m; }

static int gen_case(unsigned seed, uint8_t mode)
{
    static const uint16_t lim[7] = { 60, 6, 3, 2, 5, 4, 7 };
    world w; mworld mw; pr_u7d0_state st; pr_u7d0_report rep;
    uint8_t rflags[PR_U7D0_FLAG_COUNT]; uint8_t win[0x700];
    int removed[64], nrem = 0, i, j, k, id = 0, ok;
    unsigned hooks = 0, n, idx, nm;
    pr_u7d0_status s;
    rng_s = seed * 7919u + 1u;
    memset(&w, 0, sizeof w);
    nm = 1 + rnd(MAXM); w.nm = (int)nm;
    for (i = 0; i < w.nm; i++) {
        w.m[i].len = (int)rnd(MAXI + 1);
        for (j = 0; j < w.m[i].len; j++) {
            w.m[i].ids[j] = id; w.type[id] = (uint8_t)rnd(256);
            k = (int)rnd(8); w.q[id] = (uint16_t)(k == 0 ? 0 : k == 1 ? 10 : k == 2 ? 11 : 1 + rnd(30));
            id++;
        }
    }
    n = rnd(5); idx = rnd(4) + (rnd(5) == 0 ? 0u : 0u);
    memset(rflags, 0, sizeof rflags);
    for (i = 0; i < 6; i++) rflags[i + 1] = (uint8_t)(rnd(3) != 0);
    memset(win, 0, sizeof win); win[0x67C] = 6;
    build(&w, &mw);
    memset(&st, 0, sizeof st);
    st.mode = mode; memcpy(st.flags, rflags, sizeof rflags);
    st.window1 = win; st.window1_size = sizeof win; st.limit = lim; st.limit_count = 7;
    st.list_head = &mw.mem[0]; st.remove = on_remove; st.ctx = &mw;
    s = pr_u7d0_run(&st, (uint8_t)n, (uint8_t)idx, &rep);
    /* reference */
    {
        int found = 0;
        if (mode == 2) { for (i = 1; i <= 6; i++) if (rflags[i]) found = 1; }
        if (mode == 2 && !found) {
            CHECK(s == PR_U7D0_OK && rep.early_return == 1 && rep.passes == 0);
            CHECK(memcmp(st.flags, rflags, sizeof rflags) == 0 && same(&w, &mw));
            return 0;
        }
    }
    CHECK(s == PR_U7D0_OK);
    r_run(&w, rflags, mode, (uint8_t)n, (uint8_t)idx, lim, &hooks, removed, &nrem);
    ok = memcmp(st.flags, rflags, sizeof rflags) == 0 && same(&w, &mw) && mw.nrem == nrem && rep.hook_calls == hooks;
    for (i = 0; ok && i < nrem; i++) ok = mw.removed[i] == removed[i];
    if (!ok) fprintf(stderr, "mismatch seed %u mode %u n %u idx %u\n", seed, mode, n, idx);
    CHECK(ok);
    return 0;
}

int main(void)
{
    unsigned seed;
    for (seed = 0; seed < 3000; seed++)
        if (gen_case(seed, 0) || gen_case(seed, 2)) return 1;

    /* pinned: unit conversion (n=2, idx=3, limits 60,6,3 -> 2 * 6 * 3... uses limit[2] then limit[1]) */
    {
        static const uint16_t lim[7] = { 60, 6, 3, 2, 5, 4, 7 };
        pr_u7d0_state st; pr_u7d0_report rep;
        memset(&st, 0, sizeof st);
        st.limit = lim; st.limit_count = 7; st.mode = 0;
        CHECK(pr_u7d0_run(&st, 2, 3, &rep) == PR_U7D0_OK);
        CHECK(rep.remaining_start == 2u * 3u * 6u && rep.filled == 1 && rep.passes == 4);   /* 36 -> 10,10,10,6 */
        CHECK(st.flags[0] == 0 && st.flags[1] == 1 && st.flags[0x48] == 1 && st.flags[0x49] == 0);
        CHECK(pr_u7d0_run(&st, 5, 1, &rep) == PR_U7D0_OK && rep.remaining_start == 5);        /* idx 1: no multiply */
        CHECK(pr_u7d0_run(&st, 5, 0x80, &rep) == PR_U7D0_OK && rep.remaining_start == 5);     /* signed byte: negative */
        CHECK(pr_u7d0_run(&st, 0, 3, &rep) == PR_U7D0_OK && rep.passes == 0);                 /* n = 0 */
        CHECK(pr_u7d0_run(&st, 255, 3, &rep) == PR_U7D0_OK && rep.remaining_start == (uint16_t)(255u * 18u));
    }
    /* pinned quirk: A(q 30) -> B(q 5) -> C(q 30), chunk 10: B expires with head as its predecessor, so the
       walk restarts at A and subtracts from A a second time (30 -> 20 -> 10); C is then visited once (30 -> 20). */
    {
        pr_u7d0_item a = { 1, 30, NULL }, b = { 2, 5, NULL }, c = { 3, 30, NULL };
        pr_u7d0_member m = { &a, NULL };
        mworld mw; pr_u7d0_state st; pr_u7d0_report rep;
        static const uint16_t lim[2] = { 60, 6 };
        a.next = &b; b.next = &c;
        memset(&mw, 0, sizeof mw);
        mw.base = &a;    /* removed[] records pointer differences; only the count is used here */
        memset(&st, 0, sizeof st);
        st.limit = lim; st.limit_count = 2; st.list_head = &m;
        st.remove = on_remove; st.ctx = &mw;
        CHECK(pr_u7d0_run(&st, 10, 1, &rep) == PR_U7D0_OK);
        CHECK(rep.passes == 1 && rep.hook_calls == 1);
        CHECK(a.quantity == 10 && c.quantity == 20 && m.head == &a && a.next == &c);
        CHECK(st.flags[1] == 1);
    }
    /* mode 2 early returns; wrap guard; ARGUMENT guards */
    {
        uint8_t win[0x700]; pr_u7d0_state st; pr_u7d0_report rep; static const uint16_t lim[7] = {0};
        memset(win, 0, sizeof win); memset(&st, 0, sizeof st);
        st.mode = 2; st.window1 = win; st.window1_size = sizeof win; st.limit = lim; st.limit_count = 7;
        win[0x67C] = 3; st.flags[4] = 1;                     /* flag outside 1..3: not scanned */
        CHECK(pr_u7d0_run(&st, 9, 3, &rep) == PR_U7D0_OK && rep.early_return == 1 && st.flags[4] == 1);
        win[0x67C] = 0;                                      /* w = 0: index 1 is still tested */
        st.flags[1] = 1;
        CHECK(pr_u7d0_run(&st, 1, 1, &rep) == PR_U7D0_OK && rep.early_return == 0 && rep.passes == 1 && st.flags[1] == 1);
        memset(st.flags, 0, sizeof st.flags); win[0x67C] = 0xFF;                  /* w >= 255: wraps; flag 0 is reached */
        CHECK(pr_u7d0_run(&st, 1, 1, &rep) == PR_U7D0_NONTERMINATING);
        st.flags[0] = 1;
        CHECK(pr_u7d0_run(&st, 1, 1, &rep) == PR_U7D0_OK && rep.early_return == 0);
        st.window1_size = 0x67D;
        CHECK(pr_u7d0_run(&st, 1, 1, &rep) == PR_U7D0_ARGUMENT);
        st.mode = 0; st.limit_count = 2;
        memset(st.flags, 0, sizeof st.flags);
        CHECK(pr_u7d0_run(&st, 1, 3, &rep) == PR_U7D0_ARGUMENT && st.flags[1] == 0);   /* needs 3 words */
        CHECK(pr_u7d0_run(NULL, 1, 1, NULL) == PR_U7D0_ARGUMENT);
    }
    /* a hook that never unlinks (here: none) on a two-item list: the head restart repeats, the step cap fires */
    {
        pr_u7d0_item b = { 2, 1, NULL }, a = { 1, 1, NULL };
        pr_u7d0_member m = { &a, NULL };
        pr_u7d0_state st; pr_u7d0_report rep;
        a.next = &b;
        memset(&st, 0, sizeof st);
        st.list_head = &m;      /* head is not the tail, so the restart at the head never reaches `done` */
        CHECK(pr_u7d0_run(&st, 1, 1, &rep) == PR_U7D0_STEP_LIMIT);
    }
    puts("overlay_u7d_zero: ok");
    return 0;
}
