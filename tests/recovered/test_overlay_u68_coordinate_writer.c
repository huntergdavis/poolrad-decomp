/* Synthetic parity test for the unit 0x68 +0x073B coordinate writer model.
 * The [0x6A5C] buffer, map id, window 1, DS direction tables and hook
 * recorders are fabricated. An independent restatement on raw bytes is
 * compared with the model over every (x, y) byte pair, several facings and
 * both values of the window bit; hook order, arguments and reached-operation
 * failures with preserved earlier effects are checked separately. */
#include "poolrad/recovered/overlay_u68_coordinate_writer.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define TABLE_SIZE 265u   /* facing 255 reads index 264 */
#define WIN1_SIZE 0x800u
#define BUF_SIZE 0x400u

static int sb(uint8_t b) { return b < 0x80u ? b : (int)b - 256; }

typedef struct {
    unsigned n;
    int order[8];          /* 1 = resident, 2 = u7d */
    uint16_t resident_arg;
    uint16_t u7d_a, u7d_b;
    int fail_resident, fail_u7d;
    pr_u68_step_state *state;
    int mutate_x;
    uint8_t replacement_x;
} rec;

static int on_resident(void *ctx, uint16_t w)
{
    rec *r = ctx;
    r->order[r->n++] = 1;
    r->resident_arg = w;
    if (r->mutate_x)
        r->state->party_x = r->replacement_x;
    return r->fail_resident;
}
static int on_u7d(void *ctx, uint16_t a, uint16_t b) { rec *r = ctx; r->order[r->n++] = 2; r->u7d_a = a; r->u7d_b = b; return r->fail_u7d; }

static void fill_buffer(uint8_t *buf)
{
    unsigned i;
    for (i = 0; i < BUF_SIZE; ++i) buf[i] = (uint8_t)(i * 29u + 7u);
}

static void setup_staged(pr_u68_step_state *st, pr_u68_step_hooks *hk,
                         rec *rc, uint8_t *win, uint8_t *tab,
                         const uint8_t *buf)
{
    memset(st, 0, sizeof *st);
    memset(rc, 0, sizeof *rc);
    rc->state = st;
    memset(win, 0, WIN1_SIZE);
    memset(tab, 0, TABLE_SIZE);
    tab[6] = 0xFF; /* synthetic facing-6 step (-1, 0) */
    hk->resident_8ae_00dc = on_resident;
    hk->u7d_entry1 = on_u7d;
    hk->ctx = rc;
    st->party_x = 15; st->party_y = 4; st->facing = 6;
    st->lookup_b0 = 0xEE; st->lookup_b1 = 0xEE;
    st->ds_6dde[0] = st->ds_6dde[1] = st->ds_6dde[2] = 0x77;
    st->word_2628 = 0xBEEF;
    st->window1 = win; st->window1_size = WIN1_SIZE;
    st->direction_table = tab; st->direction_table_size = TABLE_SIZE;
    st->map_id = 1; st->buffer = buf; st->entry2_frame_residue = 0x5A;
    st->hooks = hk;
}

/* The routine restated on raw bytes (wrap, then entry 2 / entry 4 reads at the wrapped cell). */
static void ref_step(const uint8_t *buf, const uint8_t *tab, uint8_t resid, uint8_t x, uint8_t y, uint8_t f,
                     uint8_t *ox, uint8_t *oy, uint8_t *b0, uint8_t *b1)
{
    unsigned cell, b;
    x = (uint8_t)(x + tab[f]);
    y = (uint8_t)(y + tab[f + 9u]);
    if (sb(x) < 0) x = 15; else if (x > 15) x = 0;
    if (sb(y) < 0) y = 15; else if (y > 15) y = 0;
    *ox = x; *oy = y;
    cell = 16u * y + x;
    if (f == 0u || f == 2u || f == 4u || f == 6u) {
        b = buf[(f & 4u ? 0x100u : 0u) + cell];
        *b0 = (uint8_t)((f & 2u) ? (b & 0x0Fu) : (b >> 4));
    } else *b0 = resid;
    *b1 = buf[0x200u + cell];
}

static int run_case(const uint8_t *buf, const uint8_t *tab, uint8_t map, uint8_t x, uint8_t y, uint8_t f, unsigned bit)
{
    uint8_t win[WIN1_SIZE];
    pr_u68_step_state st;
    pr_u68_step_report rep;
    pr_u68_step_hooks hk;
    rec rc;
    uint8_t ex, ey, e0, e1;
    memset(win, 0xA5, sizeof win);
    win[0x594] = (uint8_t)(0xFEu | bit);    /* only bit 0 matters */
    win[0x595] = 0xFF;
    memset(&rc, 0, sizeof rc);
    hk.resident_8ae_00dc = on_resident; hk.u7d_entry1 = on_u7d; hk.ctx = &rc;
    memset(&st, 0, sizeof st);
    st.party_x = x; st.party_y = y; st.facing = f;
    st.lookup_b0 = 0xEE; st.lookup_b1 = 0xEE;
    st.ds_6dde[0] = st.ds_6dde[1] = st.ds_6dde[2] = 0x77;
    st.word_2628 = 0xBEEF;
    st.window1 = win; st.window1_size = sizeof win;
    st.direction_table = tab; st.direction_table_size = TABLE_SIZE;
    st.map_id = map; st.buffer = buf; st.entry2_frame_residue = 0x0B;
    st.hooks = &hk;
    CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_OK);
    ref_step(buf, tab, 0x0B, x, y, f, &ex, &ey, &e0, &e1);
    CHECK(st.party_x == ex && st.party_y == ey);
    CHECK(rep.x_out == ex && rep.y_out == ey && rep.x_in == x && rep.y_in == y);
    CHECK(st.facing == f);
    CHECK(st.lookup_b0 == e0 && st.lookup_b1 == e1);
    CHECK(st.ds_6dde[0] == 1 && st.ds_6dde[1] == 1 && st.ds_6dde[2] == 1);
    CHECK(rc.n == 2 && rc.order[0] == 1 && rc.order[1] == 2);
    CHECK(rc.resident_arg == 0xBEEF);
    CHECK(rc.u7d_a == 1 && rc.u7d_b == (bit ? 2u : 1u));
    CHECK(rep.u7d_first == 1 && rep.u7d_second == rc.u7d_b && rep.window_bit == bit);
    CHECK(rep.call_count == 5 && rep.calls[0] == PR_U68_STEP_CALL_RESIDENT &&
          rep.calls[1] == PR_U68_STEP_CALL_ENTRY2 && rep.calls[2] == PR_U68_STEP_CALL_FILL &&
          rep.calls[3] == PR_U68_STEP_CALL_ENTRY4 && rep.calls[4] == PR_U68_STEP_CALL_U7D_ENTRY1);
    CHECK(rep.stop_offset == 0x07F4u);
    CHECK(rep.dx == sb(tab[f]) && rep.dy == sb(tab[f + 9u]));
    /* window 1 untouched */
    {
        unsigned i;
        for (i = 0; i < WIN1_SIZE; ++i)
            CHECK(win[i] == (i == 0x594 ? (uint8_t)(0xFEu | bit) : i == 0x595 ? 0xFFu : 0xA5u));
    }
    (void)map;
    return 0;
}

int main(void)
{
    uint8_t buf[BUF_SIZE];
    uint8_t tab[TABLE_SIZE];
    static const uint8_t facings[] = { 0, 2, 4, 6, 1, 3, 5, 7, 8, 255 };
    static const uint8_t maps[] = { 0, 10 };
    unsigned i, x, y, m, bit;

    fill_buffer(buf);
    /* synthetic unit-step table: dx at index f (0,+1,0,-1 pattern), dy at f + 9 */
    for (i = 0; i < TABLE_SIZE; ++i) tab[i] = (uint8_t)(i * 37u + 3u);
    for (i = 0; i < 8; i += 2) {
        tab[i] = (uint8_t)(i == 2 ? 1 : i == 6 ? 0xFF : 0);
        tab[i + 9] = (uint8_t)(i == 0 ? 0xFF : i == 4 ? 1 : 0);
    }

    for (m = 0; m < sizeof maps; ++m)
        for (i = 0; i < sizeof facings; ++i)
            for (bit = 0; bit < 2; ++bit)
                for (x = 0; x < 256; ++x)
                    for (y = 0; y < 256; y += (x & 15u) == 0 ? 1 : 17)
                        if (run_case(buf, tab, maps[m], (uint8_t)x, (uint8_t)y, facings[i], bit)) return 1;

    /* pinned wrap behaviour: 15 + 1 -> 0, 0 - 1 -> 15, 127 + 1 (0x80, negative) -> 15, 15 + 2 (0x11) -> 0 */
    {
        uint8_t win[WIN1_SIZE] = {0};
        pr_u68_step_state st;
        pr_u68_step_report rep;
        pr_u68_step_hooks hk;
        rec rc;
        uint8_t t[TABLE_SIZE];
        memset(&rc, 0, sizeof rc);
        hk.resident_8ae_00dc = on_resident; hk.u7d_entry1 = on_u7d; hk.ctx = &rc;
        memset(t, 0, sizeof t);
        t[2] = 1; t[2 + 9] = 0xFF;                 /* facing 2: x+1, y-1 */
        memset(&st, 0, sizeof st);
        st.party_x = 15; st.party_y = 0; st.facing = 2;
        st.window1 = win; st.window1_size = sizeof win;
        st.direction_table = t; st.direction_table_size = sizeof t;
        st.buffer = buf; st.hooks = &hk;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_OK);
        CHECK(st.party_x == 0 && st.party_y == 15);
        CHECK(rep.wrapped_x_high == 1 && rep.wrapped_x_low == 0 && rep.wrapped_y_low == 1 && rep.wrapped_y_high == 0);
        CHECK(rep.u7d_second == 1);                /* window bit clear */
        st.party_x = 127; st.party_y = 14; st.facing = 2;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_OK);
        CHECK(rep.x_sum == 0x80 && st.party_x == 15 && rep.wrapped_x_low == 1 && rep.wrapped_x_high == 0);
        CHECK(st.party_y == 13 && !rep.wrapped_y_low && !rep.wrapped_y_high);
        st.party_x = 0x10; st.party_y = 5; st.facing = 2;      /* stored out of range byte 0x11 -> 0 */
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_OK);
        CHECK(rep.x_sum == 0x11 && st.party_x == 0);
    }

    /* Every missing/failing host boundary stops when first reached. */
    {
        uint8_t win[WIN1_SIZE] = {0};
        uint8_t t[TABLE_SIZE];
        pr_u68_step_state st;
        pr_u68_step_hooks hk;
        pr_u68_step_report rep;
        rec rc;
        setup_staged(&st, &hk, &rc, win, t, buf);
        st.hooks = NULL;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_HOOK);
        CHECK(rep.stop_offset == 0x0742u && rep.call_count == 1 && rc.n == 0);
        CHECK(st.party_x == 15 && st.party_y == 4 && st.ds_6dde[0] == 0x77);

        setup_staged(&st, &hk, &rc, win, t, buf);
        rc.fail_resident = 1;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_HOOK);
        CHECK(rep.stop_offset == 0x0742u && rc.n == 1 && rc.order[0] == 1);
        CHECK(st.party_x == 15 && st.party_y == 4 && st.lookup_b0 == 0xEE);

        setup_staged(&st, &hk, &rc, win, t, buf);
        rc.mutate_x = 1; rc.replacement_x = 10;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_OK);
        CHECK(rep.x_in == 10 && st.party_x == 9 && st.party_y == 4);

        setup_staged(&st, &hk, &rc, win, t, buf);
        rc.mutate_x = 1; rc.replacement_x = 10; rc.fail_resident = 1;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_HOOK);
        CHECK(rep.stop_offset == 0x0742u && st.party_x == 10 && st.party_y == 4);

        setup_staged(&st, &hk, &rc, win, t, buf);
        st.direction_table = NULL;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_ARGUMENT);
        CHECK(rep.stop_offset == 0x074Eu && rc.n == 1 && st.party_x == 15);
        CHECK(st.party_y == 4 && st.lookup_b0 == 0xEE);

        setup_staged(&st, &hk, &rc, win, t, buf);
        st.direction_table_size = 7; /* x index 6 exists; y index 15 does not */
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_ARGUMENT);
        CHECK(rep.stop_offset == 0x075Du && rc.n == 1 && st.party_x == 14);
        CHECK(st.party_y == 4 && st.lookup_b0 == 0xEE && st.ds_6dde[0] == 0x77);

        setup_staged(&st, &hk, &rc, win, t, buf);
        st.buffer = NULL; /* recognised selector: entry 2 fails at its read */
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_LOOKUP);
        CHECK(rep.stop_offset == 0x07A1u && rep.call_count == 2 && rc.n == 1);
        CHECK(st.party_x == 14 && st.party_y == 4 && st.lookup_b0 == 0xEE);
        CHECK(st.ds_6dde[0] == 0x77);

        setup_staged(&st, &hk, &rc, win, t, buf);
        st.facing = 1; st.buffer = NULL; /* entry 2 returns explicit residue; entry 4 needs buffer */
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_LOOKUP);
        CHECK(rep.stop_offset == 0x07C2u && rep.call_count == 4 && rc.n == 1);
        CHECK(st.lookup_b0 == 0x5A && st.lookup_b1 == 0xEE);
        CHECK(st.ds_6dde[0] == 1 && st.ds_6dde[1] == 1 && st.ds_6dde[2] == 1);

        setup_staged(&st, &hk, &rc, win, t, buf);
        st.window1 = NULL;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_ARGUMENT);
        CHECK(rep.stop_offset == 0x07CEu && rep.call_count == 4 && rc.n == 1);
        CHECK(st.party_x == 14 && st.party_y == 4);
        CHECK(st.lookup_b0 != 0xEE && st.lookup_b1 != 0xEE && st.ds_6dde[0] == 1);

        setup_staged(&st, &hk, &rc, win, t, buf);
        st.window1_size = 0x595;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_ARGUMENT);
        CHECK(rep.stop_offset == 0x07CEu && rc.n == 1 && st.party_x == 14);

        setup_staged(&st, &hk, &rc, win, t, buf);
        hk.u7d_entry1 = NULL;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_HOOK);
        CHECK(rep.stop_offset == 0x07EDu && rep.call_count == 5 && rc.n == 1);
        CHECK(st.party_x == 14 && st.party_y == 4 && st.ds_6dde[0] == 1);

        setup_staged(&st, &hk, &rc, win, t, buf);
        win[0x594] = 1;
        rc.fail_u7d = 1;
        CHECK(pr_u68_step_coordinate_writer(&st, &rep) == PR_U68_STEP_HOOK);
        CHECK(rep.stop_offset == 0x07E0u && rc.n == 2 && rc.order[1] == 2);
        CHECK(rc.u7d_a == 1 && rc.u7d_b == 2 && st.party_x == 14);

        CHECK(pr_u68_step_coordinate_writer(NULL, &rep) == PR_U68_STEP_ARGUMENT);
        CHECK(rep.stop_offset == 0x073Bu && rep.call_count == 0);
    }
    puts("overlay_u68_coordinate_writer: ok");
    return 0;
}
