/* Synthetic parity test for the unit 0x9e entry 2 / entry 1 model. Lists,
 * the handler table and the release hook are fabricated. A reference on
 * id arrays (remove first-by-type or the given id) is compared with the
 * pointer model over generated lists; handler/release ordering and the
 * connection to overlay_u7d_zero.c are checked separately. */
#include "poolrad/recovered/overlay_u9e_entry2.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define N 6

typedef struct {
    pr_u7d0_item it[N];
    pr_u7d0_member m;
    unsigned h_calls, rel_calls;
    uint8_t h_type, h_mode;
    int h_item, h_present_in_list;   /* index; whether the item was still linked at handler time */
    int rel_item; uint16_t rel_size;
    int rel_was_linked;
    pr_u7d0_member *h_member;
} W;

static int linked(const W *w, const pr_u7d0_item *x)
{
    const pr_u7d0_item *p;
    for (p = w->m.head; p; p = p->next) if (p == x) return 1;
    return 0;
}
static void on_handler_a(void *ctx, pr_u7d0_member *m, pr_u7d0_item *it, uint8_t mode)
{
    W *w = ctx;
    w->h_calls++; w->h_mode = mode; w->h_member = m; w->h_item = (int)(it - w->it); w->h_present_in_list = linked(w, it);
    w->h_type = 1;
}
static void on_handler_b(void *ctx, pr_u7d0_member *m, pr_u7d0_item *it, uint8_t mode)
{
    W *w = ctx; (void)m; (void)it; (void)mode; w->h_calls++; w->h_type = 2;
}
static void on_release(void *ctx, pr_u7d0_item *it, uint16_t size)
{
    W *w = ctx;
    w->rel_calls++; w->rel_item = (int)(it - w->it); w->rel_size = size; w->rel_was_linked = linked(w, it);
}

static void build(W *w, int len, const uint8_t *types, const uint8_t *flags)
{
    int i;
    memset(w, 0, sizeof *w);
    for (i = 0; i < len; i++) {
        w->it[i].type = types[i]; w->it[i].flag4 = flags[i]; w->it[i].quantity = (uint16_t)(100 + i);
        w->it[i].next = i + 1 < len ? &w->it[i + 1] : NULL;
    }
    w->m.head = len ? &w->it[0] : NULL;
}
static void setup(pr_u9e_state *st, W *w)
{
    memset(st, 0, sizeof *st);
    st->handlers[5] = on_handler_a; st->handlers[9] = on_handler_b;
    st->release = on_release; st->ctx = w;
}
static int order(const W *w, int *out)   /* indices in list order */
{
    int n = 0; const pr_u7d0_item *p;
    for (p = w->m.head; p; p = p->next) out[n++] = (int)(p - w->it);
    return n;
}

int main(void)
{
    unsigned seed, len;
    uint8_t types[N], flags[N];

    /* generated: every list of length 0..6 with small type alphabet, every target (null+type, or an index) */
    for (seed = 0; seed < 4000; seed++) {
        W w; pr_u9e_state st; pr_u9e_report rep;
        int ref[N], rn, got[N], gn, sel = -1, tgt, i, expect_handler;
        unsigned x = seed * 2654435761u + 7u;
        len = x % (N + 1); x /= 7;
        for (i = 0; i < N; i++) { types[i] = (uint8_t)(5 + (x % 3) * 2); x = x * 1103515245u + 12345u; flags[i] = (uint8_t)((x >> 9) & 1); x = x * 1103515245u + 12345u; }
        build(&w, (int)len, types, flags); setup(&st, &w);
        tgt = (int)((x >> 5) % (len + 2));    /* 0..len-1: given item; len: null + type 5; len+1: null + type 7 */
        {
            uint8_t want = (tgt == (int)len + 1) ? 7 : (tgt == (int)len ? 5 : types[tgt]);
            if (tgt < (int)len) sel = tgt;
            else for (i = 0; i < (int)len; i++) if (types[i] == want) { sel = i; break; }
            gn = 0;
            CHECK(pr_u9e_entry2(&st, &w.m, want, tgt < (int)len ? &w.it[tgt] : NULL, &rep) == PR_U9E_OK);
            rn = 0; for (i = 0; i < (int)len; i++) if (i != sel) ref[rn++] = i;
            gn = order(&w, got);
            CHECK(gn == rn && memcmp(got, ref, sizeof(int) * (size_t)rn) == 0);
            CHECK(rep.searched == (tgt >= (int)len) && rep.found == (sel >= 0));
            CHECK(w.rel_calls == (sel >= 0 ? 1u : 0u));
            expect_handler = sel >= 0 && flags[sel] && (want == 5 || want == 9);
            if (sel >= 0 && flags[sel]) CHECK(rep.handler_called == 1 && rep.handler_was_null == !(want == 5 || want == 9));
            else CHECK(rep.handler_called == 0);
            CHECK(w.h_calls == (unsigned)expect_handler);
            if (sel >= 0) {
                CHECK(w.rel_item == sel && w.rel_size == 9 && !w.rel_was_linked);
                CHECK(rep.was_head == (sel == 0));
                if (expect_handler && want == 5) CHECK(w.h_present_in_list && w.h_mode == 1 && w.h_item == sel && w.h_member == &w.m);
            }
        }
    }

    /* pinned: search takes the FIRST match; a given item wins over type order */
    {
        W w; pr_u9e_state st; pr_u9e_report rep; int o[N];
        static const uint8_t t[4] = { 7, 5, 5, 7 }, f[4] = { 0, 0, 0, 0 };
        build(&w, 4, t, f); setup(&st, &w);
        CHECK(pr_u9e_entry2(&st, &w.m, 5, NULL, &rep) == PR_U9E_OK && w.rel_item == 1);
        CHECK(order(&w, o) == 3 && o[0] == 0 && o[1] == 2 && o[2] == 3);
        CHECK(pr_u9e_entry2(&st, &w.m, 5, &w.it[3], &rep) == PR_U9E_OK && w.rel_item == 3);   /* type argument ignored */
        CHECK(order(&w, o) == 2 && o[0] == 0 && o[1] == 2 && w.it[2].next == NULL);
        CHECK(pr_u9e_entry2(&st, &w.m, 5, &w.it[0], &rep) == PR_U9E_OK && rep.was_head && w.m.head == &w.it[2]);
    }
    /* handler selection by type byte (table index), table slot NULL, mode byte, flag 0 */
    {
        W w; pr_u9e_state st; pr_u9e_report rep;
        static const uint8_t t[2] = { 9, 200 }, f[2] = { 1, 1 };
        build(&w, 2, t, f); setup(&st, &w);
        CHECK(pr_u9e_entry2(&st, &w.m, 9, NULL, &rep) == PR_U9E_OK && w.h_type == 2 && w.h_calls == 1);
        CHECK(pr_u9e_entry2(&st, &w.m, 200, NULL, &rep) == PR_U9E_OK && rep.handler_called && rep.handler_was_null && w.h_calls == 1);
        CHECK(w.m.head == NULL && w.rel_calls == 2);
        pr_u9e_dispatch(&st, 5, &w.m, &w.it[0], 7);
        CHECK(w.h_calls == 2 && w.h_mode == 7);
        pr_u9e_dispatch(NULL, 5, &w.m, &w.it[0], 7);          /* null state: no call */
    }
    /* UNLINK_FAILED: record not in the list; and a handler that removes the record itself */
    {
        W w; pr_u9e_state st; pr_u9e_report rep; pr_u7d0_item stray = { 3, 1, NULL, 0 };
        static const uint8_t t[2] = { 5, 6 }, f[2] = { 0, 0 };
        build(&w, 2, t, f); setup(&st, &w);
        CHECK(pr_u9e_entry2(&st, &w.m, 3, &stray, &rep) == PR_U9E_UNLINK_FAILED && w.rel_calls == 0);
        CHECK(w.m.head == &w.it[0] && w.it[0].next == &w.it[1]);
        CHECK(pr_u9e_entry2(&st, &w.m, 3, NULL, &rep) == PR_U9E_OK && !rep.found && w.rel_calls == 0);
        CHECK(pr_u9e_entry2(NULL, &w.m, 3, NULL, &rep) == PR_U9E_ARGUMENT);
        CHECK(pr_u9e_entry2(&st, NULL, 3, NULL, &rep) == PR_U9E_ARGUMENT);
        w.it[1].flag4 = 1;
        st.handlers[6] = on_handler_a;
        CHECK(pr_u9e_entry2(&st, &w.m, 6, NULL, &rep) == PR_U9E_OK && rep.handler_called && w.h_present_in_list);
    }
    /* connected: overlay_u7d_zero.c with this model as its unit 0x9e hook really unlinks and frees expiring items */
    {
        W w; pr_u9e_state st; pr_u7d0_state zs; pr_u7d0_report zr; static const uint16_t lim[2] = { 60, 6 };
        static const uint8_t t[3] = { 5, 6, 7 }, f[3] = { 1, 0, 0 };
        int o[N];
        build(&w, 3, t, f); setup(&st, &w);
        w.it[0].quantity = 3; w.it[1].quantity = 50; w.it[2].quantity = 10;
        memset(&zs, 0, sizeof zs);
        zs.limit = lim; zs.limit_count = 2; zs.list_head = &w.m;
        zs.remove = pr_u9e_as_u7d0_remove_hook; zs.ctx = &st;
        CHECK(pr_u7d0_run(&zs, 10, 1, &zr) == PR_U7D0_OK);   /* idx 1: one chunk of 10 */
        /* A (3 <= 10): handler (flag4), unlinked, freed; B (50 > 10): 40, re-flagged; C (10 <= 10, tail):
           freed, then the head restart finds `done` and ends the walk. Hand-traced from the bytes. */
        CHECK(zr.hook_calls == 2 && w.h_calls == 1 && w.rel_calls == 2);
        CHECK(order(&w, o) == 1 && o[0] == 1 && w.it[1].quantity == 40 && zs.flags[1] == 1);
        CHECK(w.rel_item == 2 && w.rel_size == 9 && !w.rel_was_linked);
    }
    puts("overlay_u9e_entry2: ok");
    return 0;
}
