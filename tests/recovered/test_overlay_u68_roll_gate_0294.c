/* Synthetic parity test for the unit 0x68 +0x0294 bounded model. The list
 * segment, the entry-3 buffer, the direction table and both hooks are
 * fabricated. A range-table restatement of the type/value rules is compared
 * with the model over generated lists; the map-updater call is also wired to
 * the merged overlay_u68_map_update model on the same buffer. */
#include "poolrad/recovered/overlay_u68_roll_gate_0294.h"
#include "poolrad/recovered/overlay_u68_map_update.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t seg[65536];
static const uint16_t NODE_OFF[4] = { 0x0000, 0x0200, 0x0400, 0x0600 };
#define SEG_SEL 0x1000u

typedef struct {
    uint8_t script[16]; unsigned nscript, rolls;
    uint8_t range[32], count[32];
    unsigned marks; uint8_t mx[4], my[4], msel[4];
    uint8_t *map; /* connected map-update target or NULL */
} H;

static uint8_t h_roll(void *c, uint8_t range, uint8_t count)
{
    H *h = c; uint8_t v = h->script[h->rolls % h->nscript];
    h->range[h->rolls] = range; h->count[h->rolls] = count; h->rolls++;
    return v;
}
static void h_mark(void *c, uint8_t x, uint8_t y, uint8_t sel)
{
    H *h = c;
    h->mx[h->marks] = x; h->my[h->marks] = y; h->msel[h->marks] = sel; h->marks++;
    if (h->map) (void)pr_u68_map_update(x, y, sel, 0, h->map, 65536u);
}

static void make_buffer(uint8_t *buf, unsigned x, unsigned y, unsigned sel, unsigned res)
{
    unsigned cell = 16u * y + x;
    memset(buf, 0, 0x400);
    if (res == 1) return;
    buf[(sel & 4u ? 0x100u : 0u) + cell] = 0x11;
    buf[0x300u + cell] = (uint8_t)(res << sel);
}

/* the rule table, rows: entry3==3?, type lo..hi, value lo..hi, sides, limit, equal, clear, noroll */
typedef struct { int r3; unsigned tlo, thi, vlo, vhi, sides, limit, eq, clear, noroll; } row;
static const row table[] = {
    { 1, 0x12, 0x12, 0x5B, 0x63, 6, 1, 1, 0, 0 }, { 1, 0x12, 0x12, 0x64, 0x64, 6, 2, 0, 0, 0 },
    { 1, 0x12, 0x12, 0x00, 0x5A, 0, 0, 0, 1, 0 }, { 1, 0x12, 0x12, 0x65, 0xFF, 0, 0, 0, 1, 0 },
    { 1, 0x13, 0x14, 0, 255, 6, 3, 0, 0, 0 }, { 1, 0x15, 0x16, 0, 255, 6, 4, 0, 0, 0 },
    { 1, 0x17, 0x17, 0, 255, 6, 5, 0, 0, 0 }, { 1, 0x18, 0x18, 0, 255, 8, 7, 0, 0, 0 },
    { 1, 0x19, 0x19, 0, 255, 0, 0, 0, 0, 1 }, { 1, 0x00, 0x11, 0, 255, 0, 0, 0, 1, 0 },
    { 1, 0x1A, 0xFF, 0, 255, 0, 0, 0, 1, 0 },
    { 0, 0x03, 0x07, 0, 255, 6, 1, 1, 0, 0 }, { 0, 0x08, 0x0F, 0, 255, 6, 2, 0, 0, 0 },
    { 0, 0x10, 0x11, 0, 255, 6, 3, 0, 0, 0 },
    { 0, 0x12, 0x12, 0x00, 0x32, 6, 3, 0, 0, 0 }, { 0, 0x12, 0x12, 0x33, 0x63, 6, 4, 0, 0, 0 },
    { 0, 0x12, 0x12, 0x64, 0x64, 6, 5, 0, 0, 0 },
};
static const row *find_row(int r3, unsigned t, unsigned v)
{
    unsigned i;
    for (i = 0; i < sizeof table / sizeof table[0]; i++)
        if (table[i].r3 == r3 && t >= table[i].tlo && t <= table[i].thi && v >= table[i].vlo && v <= table[i].vhi)
            return &table[i];
    return NULL;                      /* no rule: nothing happens */
}

static void put_node(unsigned i, unsigned t, unsigned v, int last)
{
    uint16_t o = NODE_OFF[i], n = last ? 0 : NODE_OFF[i + 1];
    seg[o + 0x10] = (uint8_t)t; seg[o + 0x16] = (uint8_t)v;
    seg[o + 0x104] = (uint8_t)n; seg[o + 0x105] = (uint8_t)(n >> 8);
    seg[o + 0x106] = last ? 0 : (uint8_t)SEG_SEL; seg[o + 0x107] = last ? 0 : (uint8_t)(SEG_SEL >> 8);
}

static void init(pr_u68_roll_gate_state *st, H *h, uint8_t *buf, pr_u68_segment_view *view, pr_u68_far *visited,
                 const uint8_t *tab)
{
    memset(st, 0, sizeof *st);
    view->selector = SEG_SEL; view->bytes = seg; view->size = sizeof seg;
    st->head.offset = 0; st->head.segment = SEG_SEL;
    st->views = view; st->view_count = 1; st->visited = visited; st->visited_capacity = 8;
    st->party_x = 15; st->party_y = 4; st->facing = 6; st->ds_6dde = 1;
    st->direction_table = tab; st->direction_table_size = 265;
    st->field.map_id = 3; st->field.buffer = buf;
    st->hooks.roll_9e_entry8 = h_roll; st->hooks.mark_0109 = h_mark; st->hooks.ctx = h;
    memset(h, 0, sizeof *h);
}

int main(void)
{
    static uint8_t buf[0x400], tab[265];
    static const unsigned types[] = { 0x00, 0x02, 0x03, 0x07, 0x08, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0xFF };
    static const unsigned vals[] = { 0x00, 0x32, 0x33, 0x5A, 0x5B, 0x63, 0x64, 0x65, 0xFF };
    H h; pr_u68_roll_gate_state st; pr_u68_segment_view view; pr_u68_far visited[8];
    pr_u68_roll_gate_report rep;
    unsigned seed, ti, vi;
    uint8_t al;

    memset(tab, 0, sizeof tab);
    tab[6] = 0xFF; tab[6 + 9] = 0x01;                       /* facing 6: dx = -1, dy = +1 */
    tab[2] = 0x01;

    /* every type/value pair, single node, every roll value 0..9, entry 3 result 3 and not 3 */
    for (ti = 0; ti < sizeof types / sizeof types[0]; ti++)
        for (vi = 0; vi < sizeof vals / sizeof vals[0]; vi++) {
            unsigned r3, roll;
            for (r3 = 0; r3 < 2; r3++)
                for (roll = 0; roll <= 9; roll++) {
                    const row *rw = find_row((int)r3, types[ti], vals[vi]);
                    unsigned dec = 0, expect_found = 0, expect_rolls = 0, expect_clear = 0;
                    init(&st, &h, buf, &view, visited, tab);
                    make_buffer(buf, 15, 4, 6, r3 ? 3 : 2);
                    put_node(0, types[ti], vals[vi], 1);
                    h.script[0] = (uint8_t)roll; h.nscript = 1;
                    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_OK);
                    if (rw) {
                        if (rw->noroll) expect_found = 1;
                        else if (rw->sides) { expect_rolls = 1; expect_found = rw->eq ? roll == rw->limit : roll <= rw->limit; }
                        expect_clear = rw->clear;
                        if (expect_rolls) CHECK(h.range[0] == rw->sides && h.count[0] == 1);
                    }
                    (void)dec;
                    CHECK(al == expect_found && rep.rolls == expect_rolls && h.rolls == expect_rolls);
                    CHECK(rep.first_entry3 == (r3 ? 3u : 2u) && rep.nodes == 1);
                    CHECK(rep.cleared_6dde == expect_clear && st.ds_6dde == (expect_clear ? 0u : 1u));
                    CHECK(h.marks == (expect_found ? 2u : 0u) && rep.marks == h.marks);
                    if (expect_found) {
                        CHECK(h.mx[0] == 15 && h.my[0] == 4 && h.msel[0] == 6);
                        CHECK(h.mx[1] == 14 && h.my[1] == 5 && h.msel[1] == 2);   /* 15-1, 4+1, (6+4) % 8 */
                    }
                }
        }

    /* generated lists of 0..4 nodes (the first hit ends the walk; later nodes are not rolled for) */
    for (seed = 0; seed < 3000; seed++) {
        unsigned n = seed % 5, i, r3 = (seed >> 3) & 1, k;
        unsigned t[4], v[4], exp_rolls = 0, exp_clear = 0, exp_found = 0, visited_n = 0;
        unsigned x = seed * 2654435761u + 5u;
        init(&st, &h, buf, &view, visited, tab);
        make_buffer(buf, 15, 4, 6, r3 ? 3 : 0);
        for (i = 0; i < 4; i++) { x = x * 1103515245u + 12345u; t[i] = types[(x >> 8) % (sizeof types / sizeof types[0])]; v[i] = vals[(x >> 4) % (sizeof vals / sizeof vals[0])]; }
        for (i = 0; i < n; i++) put_node(i, t[i], v[i], i == n - 1);
        if (!n) st.head.offset = st.head.segment = 0;
        for (k = 0; k < 16; k++) { x = x * 1103515245u + 12345u; h.script[k] = (uint8_t)((x >> 9) % 9u); }
        h.nscript = 16;
        for (i = 0; i < n && !exp_found; i++) {
            const row *rw = find_row((int)r3, t[i], v[i]);
            visited_n++;
            if (!rw) continue;
            if (rw->clear) exp_clear++;
            if (rw->noroll) exp_found = 1;
            else if (rw->sides) {
                uint8_t roll = h.script[exp_rolls % 16];
                CHECK(1);
                exp_rolls++;
                if (rw->eq ? roll == rw->limit : roll <= rw->limit) exp_found = 1;
            }
        }
        CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_OK);
        CHECK(al == exp_found && rep.nodes == visited_n && rep.rolls == exp_rolls && rep.cleared_6dde == exp_clear);
        CHECK(h.marks == (exp_found ? 2u : 0u) && st.ds_6dde == (exp_clear ? 0u : 1u));
    }

    /* connected: the marks drive the merged map updater on the same buffer as entry 3 */
    {
        static uint8_t mapseg[65536], ref[0x400];
        init(&st, &h, mapseg, &view, visited, tab);              /* entry 3 reads the first 1,024 bytes of the map segment */
        make_buffer(mapseg, 15, 4, 6, 3);                       /* entry 3 result 3 at (15,4) facing 6 */
        mapseg[0x300 + 16 * 4 + 15] = 0xFF;                     /* every field 3 */
        mapseg[0x300 + 16 * 5 + 14] = 0xFF;                     /* the neighbour cell (14, 5) */
        mapseg[0x00 + 16 * 4 + 15] = 0x11; mapseg[0x100 + 16 * 4 + 15] = 0x11;
        memcpy(ref, mapseg, sizeof ref);
        put_node(0, 0x19, 0, 1);                                /* type 0x19 with result 3: found, no roll */
        h.map = mapseg;
        CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_OK && al == 1 && h.rolls == 0 && h.marks == 2);
        ref[0x300 + 16 * 4 + 15] = 0x7F;                        /* (15, 4) selector 6: (0xFF & 0x3F) | 0x40, the field set to 1 */
        ref[0x300 + 16 * 5 + 14] = 0xF7;                        /* (14, 5) selector 2: (0xFF & 0xF3) | 0x04 */
        CHECK(memcmp(mapseg, ref, sizeof ref) == 0);
    }

    /* guards: missing hooks (reached only), cycle, capacity, views, link at 0xFFFF, short table, entry 3 buffer */
    init(&st, &h, buf, &view, visited, tab); make_buffer(buf, 15, 4, 6, 3);
    put_node(0, 0x13, 0, 1);
    st.hooks.roll_9e_entry8 = NULL;
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_HOOK);
    put_node(0, 0x19, 0, 1); st.hooks.roll_9e_entry8 = NULL; st.hooks.mark_0109 = NULL;
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_HOOK && rep.rolls == 0);   /* found with no roll, the mark hook is the missing one */
    put_node(0, 0x18, 0, 1); st.hooks.roll_9e_entry8 = h_roll; st.hooks.mark_0109 = h_mark;
    h.script[0] = 1; h.nscript = 1;
    st.direction_table_size = 6 + 9;                        /* facing 6 needs 16 */
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_ARGUMENT && h.marks == 1);   /* the first mark already ran */
    st.direction_table_size = 265; st.direction_table = NULL; memset(&h, 0, sizeof h); h.script[0] = 1; h.nscript = 1;
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_ARGUMENT);
    st.direction_table = tab;
    /* cycle */
    put_node(0, 0x00, 0, 0); seg[NODE_OFF[0] + 0x104] = 0; seg[NODE_OFF[0] + 0x105] = 0;   /* next = itself */
    seg[NODE_OFF[0] + 0x106] = (uint8_t)SEG_SEL; seg[NODE_OFF[0] + 0x107] = (uint8_t)(SEG_SEL >> 8);
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_CYCLE);
    st.visited_capacity = 1; put_node(0, 0x00, 0, 0); put_node(1, 0x00, 0, 1);
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_CAPACITY);
    st.visited_capacity = 8; view.size = 100;
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_ARGUMENT);
    view.size = sizeof seg; st.head.segment = 0x2222;
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_ARGUMENT);
    st.head.segment = SEG_SEL; st.head.offset = (uint16_t)(0xFFFFu - 0x104u);               /* link low word at FFFF */
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_ARGUMENT);
    st.head.offset = 0; st.field.buffer = NULL;
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_ARGUMENT && rep.rolls == 0 && rep.nodes == 1);   /* entry 3 fails before any roll */
    st.head.offset = st.head.segment = 0;                                                      /* empty list: no entry 3 call at all */
    CHECK(pr_u68_roll_gate(&st, &al, &rep) == PR_U68_ROLL_OK && al == 0 && rep.nodes == 0);
    CHECK(pr_u68_roll_gate(NULL, &al, NULL) == PR_U68_ROLL_ARGUMENT && pr_u68_roll_gate(&st, NULL, NULL) == PR_U68_ROLL_ARGUMENT);

    puts("overlay_u68_roll_gate_0294: ok");
    return 0;
}
