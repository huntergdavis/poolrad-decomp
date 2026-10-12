/* Synthetic parity test for the unit 0x68 +0x068D Up preflight model. The
 * [0x6A5C] buffer, the map id, window 1 and the DS direction tables are all
 * fabricated. An independent restatement on raw bytes (including entry 3's
 * rule, restated as in test_overlay_u3bd_field.c) is compared with the model
 * over every (x, y) byte pair for a set of facings and map ids. */
#include "poolrad/recovered/overlay_u68_up_preflight.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

#define TABLE_SIZE 265u   /* facing 255 reads index 264 */
#define WIN1_SIZE 0x800u

static int sb(uint8_t b) { return b < 0x80u ? b : (int)b - 256; }
static int ref_wrap(uint8_t b) { return b <= 0x0Fu ? b : (b <= 0x7Fu ? 0 : 15); }
static int ref_in(uint8_t b) { return b <= 0x0Fu; }
/* entry 3's result on raw bytes (map gate, wrap, entry 2 nibble, 2-bit field) */
static int ref_field(uint8_t map, uint8_t x, uint8_t y, uint8_t sel, const uint8_t *buf)
{
    unsigned cell, nib, b;
    if (!(ref_in(x) && ref_in(y)) && (map == 0u || map == 10u)) return 0;
    cell = 16u * (unsigned)ref_wrap(y) + (unsigned)ref_wrap(x);
    if (sel == 0u || sel == 2u || sel == 4u || sel == 6u) {
        b = buf[(sel & 4u ? 0x100u : 0u) + cell];
        nib = (sel & 2u) ? (b & 0x0Fu) : (b >> 4);
    } else nib = 0x5Au;   /* residue: any non-zero value gives 1 below, zero gives 1 too */
    if (nib == 0u) return 1;
    if (sel == 0u || sel == 2u || sel == 4u || sel == 6u) return (buf[0x300u + cell] >> sel) & 3u;
    return 1;
}

/* The whole routine restated: returns the expected x, y bytes and flag. */
static void ref_preflight(uint8_t map, const uint8_t *buf, const uint8_t *tab,
                          uint8_t x, uint8_t y, uint8_t f, uint8_t *ox, uint8_t *oy, unsigned *flag)
{
    int cx = sb(x), cy = sb(y);
    *ox = x; *oy = y; *flag = 0;
    if (ref_field(map, x, y, f, buf) == 0) return;
    cx += sb(tab[f]); cy += sb(tab[f + 9u]);
    if (cx > 15) { *ox = 15; *flag = 1; }
    if (cx < 0)  { *ox = 0;  *flag = 1; }
    if (cy > 15) { *oy = 15; *flag = 1; }
    if (cy < 0)  { *oy = 0;  *flag = 1; }
}

static void fill_buffer(uint8_t *buf)
{
    unsigned i;
    for (i = 0; i < PR_U3BD_FIELD_BUFFER_SIZE; ++i) buf[i] = (uint8_t)(i * 13u + 5u);
    buf[0x00] = 0x0F; buf[0x01] = 0xF0; buf[0x02] = 0x00; buf[0x100] = 0x0F; buf[0x101] = 0xF0; buf[0x102] = 0x00;
    buf[0x300] = 0xE4; buf[0x301] = 0x1B; buf[0x302] = 0xFF;
    buf[0x3F4] = 0x00;   /* cell (4,15): field 0 for every selector -> lookup 0 when the nibble is non-zero */
    buf[0x34F] = 0xFF; buf[0x303] = 0xFF; buf[0x37F] = 0xFF;   /* pinned cells (15,4), (3,0), (15,7): all fields 3 */
    buf[0x0F4] = 0xFF; buf[0x1F4] = 0xFF;
}

static void fill_table(uint8_t *tab)
{
    unsigned i;
    for (i = 0; i < TABLE_SIZE; ++i) tab[i] = (uint8_t)(i * 37u + 11u);
    /* dx at [f], dy at [f + 9] for the even facings: synthetic unit steps */
    tab[0] = 0x00; tab[9]  = 0xFF;   /* f=0: (0, -1) */
    tab[2] = 0x01; tab[11] = 0x00;   /* f=2: (+1, 0) */
    tab[4] = 0x00; tab[13] = 0x01;   /* f=4: (0, +1) */
    tab[6] = 0xFF; tab[15] = 0x00;   /* f=6: (-1, 0) */
}

static int run(pr_u68_up_state *s, uint8_t *win1, const uint8_t *buf, const uint8_t *tab, uint8_t map,
               uint8_t x, uint8_t y, uint8_t f, pr_u68_up_report *rep)
{
    memset(s, 0, sizeof *s);
    s->party_x = x; s->party_y = y; s->facing = f;
    s->window1 = win1; s->window1_size = WIN1_SIZE;
    s->direction_table = tab; s->direction_table_size = TABLE_SIZE;
    s->field.map_id = map; s->field.buffer = buf; s->field.entry2_frame_residue = 0x5A;
    return pr_u68_up_preflight(s, rep);
}

static int test_exhaustive(void)
{
    static const uint8_t maps[] = {0, 10, 1};
    static const uint8_t facings[] = {0, 2, 4, 6, 1, 3, 5, 7, 8, 0xFF};
    uint8_t buf[PR_U3BD_FIELD_BUFFER_SIZE], tab[TABLE_SIZE], win1[WIN1_SIZE], tpl[WIN1_SIZE];
    pr_u68_up_state s; pr_u68_up_report rep;
    unsigned m, k, x, y;
    fill_buffer(buf); fill_table(tab);
    for (k = 0; k < WIN1_SIZE; ++k) tpl[k] = (uint8_t)(k * 3u + 1u);
    for (m = 0; m < sizeof maps; ++m) for (k = 0; k < sizeof facings; ++k)
    for (x = 0; x < 256u; ++x) for (y = 0; y < 256u; ++y) {
        uint8_t ex, ey; unsigned ef;
        memcpy(win1, tpl, sizeof win1);
        ref_preflight(maps[m], buf, tab, (uint8_t)x, (uint8_t)y, facings[k], &ex, &ey, &ef);
        CHECK(run(&s, win1, buf, tab, maps[m], (uint8_t)x, (uint8_t)y, facings[k], &rep) == PR_U68_UP_OK);
        CHECK(s.party_x == ex && s.party_y == ey && s.facing == facings[k]);
        CHECK(rep.flag_set == ef && win1[0x5AA] == ef && win1[0x5AB] == 0);
        CHECK(memcmp(win1, tpl, 0x5AA) == 0 && memcmp(win1 + 0x5AC, tpl + 0x5AC, WIN1_SIZE - 0x5AC) == 0);
        CHECK(rep.x_in == sb((uint8_t)x) && rep.y_in == sb((uint8_t)y) && rep.facing == facings[k]);
        CHECK(rep.lookup == (uint8_t)ref_field(maps[m], (uint8_t)x, (uint8_t)y, facings[k], buf));
        CHECK(rep.skipped == (rep.lookup == 0));
        if (!rep.skipped) {
            CHECK(rep.dx == sb(tab[facings[k]]) && rep.dy == sb(tab[facings[k] + 9u]));
            CHECK(rep.x_candidate == rep.x_in + rep.dx && rep.y_candidate == rep.y_in + rep.dy);
            CHECK(rep.clamped_x_high == (rep.x_candidate > 15) && rep.clamped_x_low == (rep.x_candidate < 0));
            CHECK(rep.clamped_y_high == (rep.y_candidate > 15) && rep.clamped_y_low == (rep.y_candidate < 0));
            /* in range: the candidate is never stored */
            if (!rep.clamped_x_high && !rep.clamped_x_low) CHECK(s.party_x == (uint8_t)x);
            if (!rep.clamped_y_high && !rep.clamped_y_low) CHECK(s.party_y == (uint8_t)y);
        } else {
            CHECK(s.party_x == (uint8_t)x && s.party_y == (uint8_t)y && rep.flag_set == 0);
        }
    }
    return 0;
}

static int test_pinned(void)
{
    uint8_t buf[PR_U3BD_FIELD_BUFFER_SIZE], tab[TABLE_SIZE], win1[WIN1_SIZE];
    pr_u68_up_state s; pr_u68_up_report rep;
    fill_buffer(buf); fill_table(tab); memset(win1, 0xCC, sizeof win1);
    /* cell (15,4), facing 6, synthetic step (-1, 0): candidate (14,4) is in range -> nothing stored,
     * flag cleared from its previous 0xCCCC to 0 */
    CHECK(run(&s, win1, buf, tab, 1, 15, 4, 6, &rep) == PR_U68_UP_OK);
    CHECK(rep.lookup != 0 && rep.x_candidate == 14 && rep.y_candidate == 4 && s.party_x == 15 && s.party_y == 4);
    CHECK(rep.flag_set == 0 && win1[0x5AA] == 0 && win1[0x5AB] == 0);
    /* facing 0 with step (0, -1) at y = 0: candidate y = -1 -> [0x6AAE] = 0 (unchanged value), flag 1 */
    CHECK(run(&s, win1, buf, tab, 1, 3, 0, 0, &rep) == PR_U68_UP_OK);
    CHECK(rep.lookup != 0 && rep.y_candidate == -1 && rep.clamped_y_low == 1 && s.party_y == 0 && rep.flag_set == 1);
    /* facing 2 with step (+1, 0) at x = 15: candidate 16 -> [0x6AAD] = 15, flag 1 */
    CHECK(run(&s, win1, buf, tab, 1, 15, 7, 2, &rep) == PR_U68_UP_OK);
    CHECK(rep.x_candidate == 16 && rep.clamped_x_high == 1 && s.party_x == 15 && rep.flag_set == 1);
    /* a negative stored byte: x = 0xF0 (-16) on map 1 wraps inside entry 3 to 15, lookup non-zero;
     * candidate -16 + dx -> clamp low writes 0 */
    CHECK(run(&s, win1, buf, tab, 1, 0xF0, 7, 2, &rep) == PR_U68_UP_OK);
    CHECK(rep.x_in == -16 && rep.x_candidate == -15 && rep.clamped_x_low == 1 && s.party_x == 0 && rep.flag_set == 1);
    /* lookup 0: map 0 and x out of range -> entry 3's gate returns 0; nothing stored, flag 0 */
    CHECK(run(&s, win1, buf, tab, 0, 16, 4, 6, &rep) == PR_U68_UP_OK);
    CHECK(rep.skipped == 1 && rep.lookup == 0 && s.party_x == 16 && s.party_y == 4 && rep.flag_set == 0);
    /* lookup 0 from a zero field: cell (4,15) quarter-3 byte 0 with non-zero nibble, selector 0 */
    CHECK(run(&s, win1, buf, tab, 1, 4, 15, 0, &rep) == PR_U68_UP_OK);
    CHECK(rep.skipped == 1 && rep.field.read == 1 && rep.field.offset == 0x3F4 && s.party_y == 15);
    /* an unrecognised facing (7): entry 3 returns 1, the table is read at 7 and 16 */
    CHECK(run(&s, win1, buf, tab, 1, 8, 8, 7, &rep) == PR_U68_UP_OK);
    CHECK(rep.lookup == 1 && rep.dx == sb(tab[7]) && rep.dy == sb(tab[16]));
    /* report optional */
    CHECK(run(&s, win1, buf, tab, 1, 15, 4, 6, NULL) == PR_U68_UP_OK && s.party_x == 15);
    return 0;
}

static int test_bounds(void)
{
    uint8_t buf[PR_U3BD_FIELD_BUFFER_SIZE], tab[TABLE_SIZE], win1[WIN1_SIZE];
    pr_u68_up_state s; pr_u68_up_report rep;
    fill_buffer(buf); fill_table(tab); memset(win1, 0xCC, sizeof win1);
    CHECK(pr_u68_up_preflight(NULL, &rep) == PR_U68_UP_ARGUMENT);
    /* window 1 too short: nothing written */
    CHECK(run(&s, win1, buf, tab, 1, 15, 4, 6, &rep) == PR_U68_UP_OK);
    memset(win1, 0xCC, sizeof win1);
    memset(&s, 0, sizeof s); s.party_x = 15; s.party_y = 4; s.facing = 6; s.window1 = win1; s.window1_size = 0x5AB;
    s.direction_table = tab; s.direction_table_size = TABLE_SIZE; s.field.map_id = 1; s.field.buffer = buf;
    CHECK(pr_u68_up_preflight(&s, &rep) == PR_U68_UP_ARGUMENT && win1[0x5AA] == 0xCC && win1[0x5AB] == 0xCC);
    s.window1 = NULL; s.window1_size = WIN1_SIZE;
    CHECK(pr_u68_up_preflight(&s, &rep) == PR_U68_UP_ARGUMENT);
    /* table too short for the facing: flag already cleared, lookup done, position untouched */
    s.window1 = win1; s.direction_table_size = 15;   /* facing 6 needs index 15 */
    CHECK(pr_u68_up_preflight(&s, &rep) == PR_U68_UP_ARGUMENT && win1[0x5AA] == 0 && win1[0x5AB] == 0);
    CHECK(rep.lookup != 0 && s.party_x == 15 && s.party_y == 4);
    s.direction_table_size = 16;
    CHECK(pr_u68_up_preflight(&s, &rep) == PR_U68_UP_OK);
    s.direction_table = NULL;
    CHECK(pr_u68_up_preflight(&s, &rep) == PR_U68_UP_ARGUMENT);
    /* lookup 0 never reaches the table: a null table is fine on the map gate */
    s.field.map_id = 0; s.party_x = 16;
    CHECK(pr_u68_up_preflight(&s, &rep) == PR_U68_UP_OK && rep.skipped == 1);
    /* null [0x6A5C] buffer on a recognised selector: ARGUMENT from entry 3 after the flag was cleared */
    memset(win1, 0xCC, sizeof win1);
    s.field.map_id = 1; s.party_x = 3; s.field.buffer = NULL; s.direction_table = tab;
    CHECK(pr_u68_up_preflight(&s, &rep) == PR_U68_UP_ARGUMENT && win1[0x5AA] == 0 && win1[0x5AB] == 0 && s.party_x == 3);
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_exhaustive();
    rc |= test_pinned();
    rc |= test_bounds();
    if (rc == 0) puts("unit 0x68 up preflight model tests passed");
    return rc;
}
