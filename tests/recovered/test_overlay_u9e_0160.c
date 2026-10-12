/* Synthetic parity test for the unit 0x9e +0x0160 model and the unit 0x3c6
 * slot helpers (entries 12..15) it uses. Party, sidecar lists, DS tables,
 * the unit 0x3c2 entry 6 scan and the handlers are fabricated. A straight
 * restatement is compared with the model over generated worlds. */
#include "poolrad/recovered/overlay_u9e_0160.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define NCH 5
#define NIT 4

static pr_u3c6_state slots_g;

/* ---- unit 0x3c6 reference ---- */
static unsigned ref_slot(const pr_u3c6_state *s, const void *p)
{
    unsigned i, n = s->bytes[3];
    if (n == 0) {                          /* byte counter: 1..255 then 0 are all examined */
        for (i = 1; i < 256; i++) if (s->slot_ptr[i] == p) return i;
        return 0;                          /* slot 0 itself is examined last but reports 0 either way */
    }
    for (i = 1; i <= n; i++) if (s->slot_ptr[i] == p) return i;
    return 0;
}

static int test_slots(void)
{
    pr_u3c6_state s; int a = 0, b = 0, c = 0, d = 0, i = 0;
    unsigned j;
    memset(&s, 0, sizeof s);
    s.bytes[3] = 3;
    s.slot_ptr[1] = &a; s.slot_ptr[2] = &b; s.slot_ptr[3] = &c; s.slot_ptr[4] = &d; s.slot_ptr[5] = &a;
    for (j = 0; j < sizeof s.bytes; j++) s.bytes[j] = (uint8_t)(j * 7u + 1u);
    s.bytes[3] = 3;
    CHECK(pr_u3c6_entry15(&s, &a) == 1 && pr_u3c6_entry15(&s, &b) == 2 && pr_u3c6_entry15(&s, &c) == 3);
    CHECK(pr_u3c6_entry15(&s, &d) == 0);                 /* slot 4 is past the count */
    CHECK(pr_u3c6_entry15(&s, &i) == 0);
    CHECK(pr_u3c6_entry12(&s, &b) == s.bytes[8] && pr_u3c6_entry13(&s, &b) == s.bytes[9] && pr_u3c6_entry14(&s, &b) == s.bytes[11]);
    CHECK(pr_u3c6_entry12(&s, &i) == s.bytes[0] && pr_u3c6_entry13(&s, &i) == s.bytes[1]);   /* slot 0 */
    CHECK(pr_u3c6_entry14(&s, &i) == 3);                 /* slot 0: [0x5F2A] is the count byte itself */
    /* count 0: the byte counter walks 1..255 and 0 */
    s.bytes[3] = 0;
    CHECK(pr_u3c6_entry15(&s, &d) == 4 && pr_u3c6_entry15(&s, &a) == 1);
    s.slot_ptr[200] = &i;
    CHECK(pr_u3c6_entry15(&s, &i) == 200);
    for (j = 0; j < 400; j++) {            /* generated: counts 0..8 */
        pr_u3c6_state g; int k; void *keys[9]; unsigned n = j % 9u, x;
        memset(&g, 0, sizeof g);
        g.bytes[3] = (uint8_t)n;
        for (x = 0; x < 9; x++) keys[x] = &keys[x];
        for (x = 1; x < 12; x++) g.slot_ptr[x] = ((x * 5u + j) % 4u) ? keys[(x + j) % 9u] : NULL;
        for (k = 0; k < 9; k++) {
            CHECK(pr_u3c6_entry15(&g, keys[k]) == ref_slot(&g, keys[k]));
            CHECK(pr_u3c6_entry15(&g, NULL) == ref_slot(&g, NULL));
        }
    }
    return 0;
}

/* ---- unit 0x9e +0x0160 worlds ---- */
typedef struct {
    pr_u7d0_member ch[NCH];
    pr_u7d0_item it[NCH][NIT];
    int h_calls; pr_u7d0_member *h_member; pr_u7d0_item *h_item; uint8_t h_mode; uint8_t h_type_slot;
    unsigned scan_calls;
    uint16_t sa, sb, smode, sff, sc;
    unsigned lookup_calls;
} W;

static void on_handler(void *ctx, pr_u7d0_member *m, pr_u7d0_item *i, uint8_t mode)
{
    W *w = ctx; w->h_calls++; w->h_member = m; w->h_item = i; w->h_mode = mode;
}
static void on_scan(void *ctx, uint16_t a, uint16_t b, uint16_t rm, uint16_t ff, uint16_t c, uint16_t off, uint16_t seg, uint8_t *scratch)
{
    W *w = ctx; unsigned k, n = (unsigned)(a + b + c) % 4u;
    w->scan_calls++; w->sa = a; w->sb = b; w->smode = rm; w->sff = ff; w->sc = c;
    scratch[0] = (uint8_t)off; scratch[1] = (uint8_t)(off >> 8); scratch[2] = (uint8_t)seg; scratch[3] = (uint8_t)(seg >> 8);
    scratch[4] = (uint8_t)n;
    for (k = 1; k <= 3; k++) scratch[2 + 3 * k] = (uint8_t)((a * 3u + b + k * 2u + rm) % 7u);
    for (k = 20; k < 216; k++) scratch[5 + k] ^= 0x5A;         /* scribble the rest of the saved span (records 1..3 stay readable) */
}
static uint8_t counting_lookup(void *ctx, pr_u7d0_member *c, uint8_t key, pr_u7d0_item **cell)
{
    pr_u7d0_item *n; W *w = ctx;
    w->lookup_calls++;
    for (n = c->head; n; n = n->next) if (n->type == key) { *cell = n; return 1; }
    *cell = NULL; return 0;
}

static unsigned rng = 1;
static unsigned rnd(unsigned m) { rng = rng * 1664525u + 1013904223u; return (rng >> 16) % m; }

static int gen_case(unsigned seed, uint8_t mode, int use_hook)
{
    static const uint8_t pool[6] = { 0x15, 0x2D, 0x31, 0x40, 0x07, 0x2E };
    W w; pr_u9e_state hs; pr_u9e0160_state st; pr_u9e0160_report rep;
    uint8_t saved_scratch[PR_U9E0160_SCRATCH_SIZE];
    int i, j, nparty;
    uint8_t type; unsigned B;
    pr_u7d0_item *want = NULL; unsigned want_scans = 0;
    rng = seed * 2654435761u + 99u;
    memset(&w, 0, sizeof w); memset(&hs, 0, sizeof hs); memset(&st, 0, sizeof st);
    for (i = 0; i < NCH; i++) {
        int n = (int)rnd(NIT + 1);
        for (j = 0; j < n; j++) {
            w.it[i][j].type = pool[rnd(6)]; w.it[i][j].quantity = (uint16_t)(i * 10 + j);
            w.it[i][j].next = j + 1 < n ? &w.it[i][j + 1] : NULL;
        }
        w.ch[i].head = n ? &w.it[i][0] : NULL;
    }
    nparty = (int)rnd(NCH + 1);
    for (i = 0; i < nparty; i++) w.ch[i].next = i + 1 < nparty ? &w.ch[i + 1] : NULL;
    st.party_head = nparty ? &w.ch[0] : NULL;
    for (i = 0; i < NCH; i++) if (rnd(4) != 0) st.slots.slot_ptr[1 + i] = &w.ch[(i + (int)seed) % NCH];
    st.slots.bytes[3] = (uint8_t)(1 + rnd(NCH));
    for (j = 4; j < 4 * 8; j++) st.slots.bytes[j] = (uint8_t)rnd(7);
    st.mode = mode;
    for (j = 0; j < (int)PR_U9E0160_SCRATCH_SIZE; j++) st.scratch[j] = (uint8_t)(j * 3u + 1u);
    memcpy(saved_scratch, st.scratch, sizeof saved_scratch);
    hs.handlers[0x15] = on_handler; hs.handlers[0x2D] = on_handler; hs.handlers[0x31] = on_handler;
    hs.handlers[0x40] = on_handler; hs.handlers[0x2E] = on_handler; hs.handlers[0x07] = on_handler; hs.ctx = &w;
    st.handlers = &hs; st.scan = on_scan; st.ctx = &w;
    if (use_hook) st.lookup = counting_lookup;
    type = pool[rnd(6)]; B = rnd(NCH);

    /* straight restatement */
    {
        pr_u7d0_member *Bc = &w.ch[B]; pr_u7d0_item *n; pr_u7d0_member *cur; int ins;
        for (n = Bc->head; n && n->type != type; n = n->next) ;
        if (n) want = n;
        ins = type == 0x15 || type == 0x2D || type == 0x2E || type == 0x31;
        if (!want && ins) {
            for (cur = st.party_head; cur && !want; cur = cur->next) {
                for (n = cur->head; n && n->type != type; n = n->next) ;
                if (!n) continue;
                if (mode != 5) { want = n; break; }
                {
                    unsigned a = st.slots.bytes[4 * ref_slot(&st.slots, cur)], b = st.slots.bytes[4 * ref_slot(&st.slots, cur) + 1];
                    unsigned c = st.slots.bytes[4 * ref_slot(&st.slots, cur) + 3], cnt = (a + b + c) % 4u, k;
                    unsigned rm = type == 0x31 ? 6u : 1u, mine = ref_slot(&st.slots, Bc);
                    want_scans++;
                    for (k = 1; k <= cnt && k <= 3; k++)
                        if ((a * 3u + b + k * 2u + rm) % 7u == mine) want = n;
                    /* a count of 0..3 only; records beyond are never read */
                }
            }
        }
    }

    CHECK(pr_u9e0160_run(&st, &w.ch[B], type, &rep) == PR_U9E0160_OK);
    if (want) {
        CHECK(rep.found == 1 && rep.cell == want && w.h_calls == 1 && w.h_item == want && w.h_member == &w.ch[B] && w.h_mode == 0);
    } else {
        CHECK(rep.found == 0 && w.h_calls == 0 && rep.cell == NULL);
    }
    CHECK(rep.scans == want_scans || mode != 5);
    if (mode != 5) CHECK(rep.scans == 0);
    if (use_hook) CHECK(w.lookup_calls >= 1);
    /* DS span restored when the save ran; the scratch outside the span keeps the scan's writes */
    if (rep.in_set && !rep.found_on_character) {
        CHECK(memcmp(st.scratch + 5, saved_scratch + 5, 216) == 0);
    }
    if (!rep.in_set || rep.found_on_character) CHECK(memcmp(st.scratch, saved_scratch, sizeof saved_scratch) == 0 && w.scan_calls == 0);
    if (w.scan_calls) { CHECK(w.sff == 0xFF && (w.smode == 6 || w.smode == 1) && (type == 0x31 ? w.smode == 6 : w.smode == 1)); }
    (void)i;
    return 0;
}

int main(void)
{
    unsigned seed;
    (void)slots_g;
    if (test_slots()) return 1;
    CHECK(pr_u9e0160_type_in_set(0x15) && pr_u9e0160_type_in_set(0x2D) && pr_u9e0160_type_in_set(0x2E) && pr_u9e0160_type_in_set(0x31));
    CHECK(!pr_u9e0160_type_in_set(0x14) && !pr_u9e0160_type_in_set(0x16) && !pr_u9e0160_type_in_set(0x00) && !pr_u9e0160_type_in_set(0xFF) && !pr_u9e0160_type_in_set(0x2C) && !pr_u9e0160_type_in_set(0x32));
    for (seed = 0; seed < 6000; seed++)
        if (gen_case(seed, 5, (int)(seed & 1)) || gen_case(seed, 2, (int)(seed & 1)) || gen_case(seed, 0, 0)) return 1;

    /* pinned: found on the character itself skips the set test and the party; call order for a party hit in mode 5 */
    {
        W w; pr_u9e_state hs; pr_u9e0160_state st; pr_u9e0160_report rep;
        memset(&w, 0, sizeof w); memset(&hs, 0, sizeof hs); memset(&st, 0, sizeof st);
        w.it[0][0].type = 0x40; w.ch[0].head = &w.it[0][0];
        hs.handlers[0x40] = on_handler; hs.ctx = &w;
        st.handlers = &hs; st.party_head = &w.ch[1]; w.ch[1].next = NULL; st.ctx = &w; st.scan = on_scan;
        CHECK(pr_u9e0160_run(&st, &w.ch[0], 0x40, &rep) == PR_U9E0160_OK);
        CHECK(rep.found_on_character && rep.calls_n == 2 && rep.calls[0] == PR_U9E0160_CALL_LOOKUP && rep.calls[1] == PR_U9E0160_CALL_DISPATCH);
        CHECK(w.h_item == &w.it[0][0] && w.h_mode == 0 && rep.members_visited == 0);
        /* type 0x31 on the party member only, mode 5, slot table maps B to slot 1: calls = lookup, set, save, lookup, scan, restore, ... */
        memset(&w, 0, sizeof w); memset(&st.scratch, 0, sizeof st.scratch);
        w.it[1][0].type = 0x31; w.ch[1].head = &w.it[1][0]; w.ch[2].next = NULL;
        hs.handlers[0x31] = on_handler;
        st.party_head = &w.ch[1]; st.mode = 5;
        st.slots.slot_ptr[1] = &w.ch[0]; st.slots.slot_ptr[2] = &w.ch[1]; st.slots.bytes[3] = 2;
        st.slots.bytes[8] = 7; st.slots.bytes[9] = 1; st.slots.bytes[11] = 2;      /* slot 2 = ch[1]: a=7 b=1 c=2 */
        CHECK(pr_u9e0160_run(&st, &w.ch[0], 0x31, &rep) == PR_U9E0160_OK);
        CHECK(w.sa == 7 && w.sb == 1 && w.smode == 6 && w.sff == 0xFF && w.sc == 2);
        /* n = (7+1+2)%4 = 2 records: k=1: (21+1+2+6)%7 = 2; k=2: (21+1+4+6)%7 = 4; B is slot 1: no match */
        CHECK(rep.found == 0 && w.h_calls == 0 && rep.scans == 1);
        CHECK(rep.calls_n == 6 && rep.calls[0] == PR_U9E0160_CALL_LOOKUP && rep.calls[1] == PR_U9E0160_CALL_SET_TEST &&
              rep.calls[2] == PR_U9E0160_CALL_SAVE && rep.calls[3] == PR_U9E0160_CALL_LOOKUP &&
              rep.calls[4] == PR_U9E0160_CALL_SCAN && rep.calls[5] == PR_U9E0160_CALL_RESTORE);
        st.slots.bytes[8] = 0; st.slots.bytes[9] = 0; st.slots.bytes[11] = 1;      /* a=0 b=0 c=1 -> n=1: k=1: (0+0+2+6)%7 = 1 = B's slot */
        CHECK(pr_u9e0160_run(&st, &w.ch[0], 0x31, &rep) == PR_U9E0160_OK);
        CHECK(rep.found == 1 && w.h_calls == 1 && w.h_item == &w.it[1][0] && w.h_member == &w.ch[0]);
        /* mode 0: any party member's node is enough, no scan */
        st.mode = 0; w.scan_calls = 0; w.h_calls = 0;
        CHECK(pr_u9e0160_run(&st, &w.ch[0], 0x31, &rep) == PR_U9E0160_OK && rep.found && w.scan_calls == 0 && w.h_calls == 1);
        /* type outside the set never visits the party; ARGUMENT guards */
        w.it[1][0].type = 0x40; w.h_calls = 0;
        CHECK(pr_u9e0160_run(&st, &w.ch[0], 0x40, &rep) == PR_U9E0160_OK && !rep.found && !rep.in_set && rep.members_visited == 0 && w.h_calls == 0);
        CHECK(pr_u9e0160_run(NULL, &w.ch[0], 1, NULL) == PR_U9E0160_ARGUMENT && pr_u9e0160_run(&st, NULL, 1, NULL) == PR_U9E0160_ARGUMENT);
    }
    /* a cyclic party list stops at the cap with the DS span restored */
    {
        W w; pr_u9e0160_state st; pr_u9e0160_report rep; unsigned j;
        memset(&w, 0, sizeof w); memset(&st, 0, sizeof st);
        w.ch[0].next = &w.ch[0]; st.party_head = &w.ch[0]; st.mode = 0;
        for (j = 0; j < PR_U9E0160_SCRATCH_SIZE; j++) st.scratch[j] = (uint8_t)j;
        CHECK(pr_u9e0160_run(&st, &w.ch[1], 0x15, &rep) == PR_U9E0160_STEP_LIMIT && rep.found == 0);
        for (j = 0; j < PR_U9E0160_SCRATCH_SIZE; j++) CHECK(st.scratch[j] == (uint8_t)j);
    }
    puts("overlay_u9e_0160: ok");
    return 0;
}
