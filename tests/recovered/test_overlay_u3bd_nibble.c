/* Synthetic tests for the unit 0x3bd entry 2 translation: the buffer and
 * the map id are fabricated; an independent restatement of the rule on raw
 * bytes is compared with the translation over every (x, y) pair for a set
 * of selectors and map ids. */
#include "poolrad/recovered/overlay_u3bd_nibble.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* Independent restatement on raw bytes: 0x00..0x0F is in range; 0x10..0x7F
 * wraps to 0; 0x80..0xFF wraps to 15. Out of range on map 0 or 10 returns 0
 * before the selector is looked at. Selector: bit 2 picks the quarter, bit 1
 * picks the low nibble; only 0, 2, 4, 6 are specified. */
static int ref_wrap(uint8_t b) { return b <= 0x0Fu ? b : (b <= 0x7Fu ? 0 : 15); }
static int ref_in(uint8_t b) { return b <= 0x0Fu; }
/* returns -1 for the unspecified path, -2 for the zero path, else the nibble */
static int ref_nibble(uint8_t map, uint8_t x, uint8_t y, uint8_t sel, const uint8_t *buf)
{
    unsigned off; uint8_t b;
    if (!(ref_in(x) && ref_in(y)) && (map == 0u || map == 10u)) return -2;
    if (sel != 0u && sel != 2u && sel != 4u && sel != 6u) return -1;
    off = (sel & 4u ? 0x100u : 0u) + 16u * (unsigned)ref_wrap(y) + (unsigned)ref_wrap(x);
    b = buf[off];
    return (sel & 2u) ? (b & 0x0F) : (b >> 4);
}

static void fill(uint8_t *buf)
{
    unsigned i;
    for (i = 0; i < PR_U3BD_NIBBLE_BUFFER_SIZE; ++i) buf[i] = (uint8_t)(i * 13u + 5u);
}

static int test_exhaustive(void)
{
    static const uint8_t maps[] = {0, 10, 1, 0x80};
    static const uint8_t sels[] = {0, 2, 4, 6, 1, 3, 5, 7, 8, 0xFF};
    uint8_t buf[PR_U3BD_NIBBLE_BUFFER_SIZE]; pr_u3bd_nibble_state s; pr_u3bd_nibble_report rep;
    unsigned m, k, x, y;
    fill(buf); s.buffer = buf; s.frame_residue = 0x5A;
    for (m = 0; m < sizeof maps; ++m) for (k = 0; k < sizeof sels; ++k) {
        s.map_id = maps[m];
        for (x = 0; x < 256u; ++x) for (y = 0; y < 256u; ++y) {
            uint8_t got = 0xA5; int want = ref_nibble(maps[m], (uint8_t)x, (uint8_t)y, sels[k], buf);
            pr_u3bd_nibble_status st = pr_u3bd_entry2(&s, (uint8_t)x, (uint8_t)y, sels[k], &got, &rep);
            CHECK(rep.in_range == (unsigned)(ref_in((uint8_t)x) && ref_in((uint8_t)y)));
            if (want == -2) {
                CHECK(st == PR_U3BD_NIBBLE_OK && got == 0 && rep.returned_zero == 1 && rep.read == 0);
                CHECK(rep.wrapped_x == 0 && rep.wrapped_y == 0);
            } else if (want == -1) {
                CHECK(st == PR_U3BD_NIBBLE_UNSPECIFIED && got == 0x5A && rep.residue_returned == 1);   /* the frame byte, not 0xA5 */
                CHECK(rep.read == 0 && rep.returned_zero == 0);
                CHECK(rep.x_used == ref_wrap((uint8_t)x) && rep.y_used == ref_wrap((uint8_t)y));   /* wraps happen first */
            } else {
                CHECK(st == PR_U3BD_NIBBLE_OK && got == (uint8_t)want && got <= 15u);
                CHECK(rep.read == 1 && rep.returned_zero == 0 && rep.residue_returned == 0);
                CHECK(rep.offset <= 0x1FFu);                                               /* quarters 0 and 1 only */
                CHECK((rep.offset >= 0x100u) == ((sels[k] & 4u) != 0u));
                CHECK(rep.high_nibble == ((sels[k] & 2u) == 0u));
                CHECK(rep.x_used == ref_wrap((uint8_t)x) && rep.y_used == ref_wrap((uint8_t)y));
                CHECK(rep.wrapped_x == (unsigned)!ref_in((uint8_t)x) && rep.wrapped_y == (unsigned)!ref_in((uint8_t)y));
                CHECK(rep.offset == (sels[k] & 4u ? 0x100u : 0u) + 16u * rep.y_used + rep.x_used);
            }
        }
    }
    return 0;
}

static int test_selectors_and_maps(void)
{
    uint8_t buf[PR_U3BD_NIBBLE_BUFFER_SIZE]; pr_u3bd_nibble_state s; pr_u3bd_nibble_report rep; uint8_t got;
    unsigned sel, m;
    fill(buf); s.buffer = buf; s.map_id = 1; s.frame_residue = 0xC3;
    buf[16 * 9 + 7] = 0xA7; buf[0x100 + 16 * 9 + 7] = 0x3C;
    for (sel = 0; sel < 256u; ++sel) {
        got = 0xEE;
        if (sel == 0u)      CHECK(pr_u3bd_entry2(&s, 7, 9, (uint8_t)sel, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0xA);
        else if (sel == 2u) CHECK(pr_u3bd_entry2(&s, 7, 9, (uint8_t)sel, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0x7);
        else if (sel == 4u) CHECK(pr_u3bd_entry2(&s, 7, 9, (uint8_t)sel, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0x3);
        else if (sel == 6u) CHECK(pr_u3bd_entry2(&s, 7, 9, (uint8_t)sel, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0xC);
        else                CHECK(pr_u3bd_entry2(&s, 7, 9, (uint8_t)sel, &got, &rep) == PR_U3BD_NIBBLE_UNSPECIFIED && got == 0xC3 && rep.residue_returned == 1);
    }
    /* every map id at (16, 0): only 0 and 10 return 0; the others wrap to cell (0, 0);
     * the zero path wins over an unspecified selector */
    buf[0] = 0x5E;
    for (m = 0; m < 256u; ++m) {
        s.map_id = (uint8_t)m;
        got = 0xEE;
        CHECK(pr_u3bd_entry2(&s, 16, 0, 2, &got, &rep) == PR_U3BD_NIBBLE_OK);
        if (m == 0u || m == 10u) CHECK(got == 0 && rep.returned_zero == 1);
        else CHECK(got == 0xE && rep.wrapped_x == 1 && rep.offset == 0);
        got = 0xEE;
        if (m == 0u || m == 10u) CHECK(pr_u3bd_entry2(&s, 16, 0, 1, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0 && rep.residue_returned == 0);
        else CHECK(pr_u3bd_entry2(&s, 16, 0, 1, &got, &rep) == PR_U3BD_NIBBLE_UNSPECIFIED && got == 0xC3);
        /* in range: the map id never matters */
        CHECK(pr_u3bd_entry2(&s, 0, 0, 0, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0x5 && rep.wrapped_x == 0 && rep.wrapped_y == 0);
    }
    return 0;
}

static int test_pinned(void)
{
    uint8_t buf[PR_U3BD_NIBBLE_BUFFER_SIZE]; pr_u3bd_nibble_state s; pr_u3bd_nibble_report rep; uint8_t got;
    unsigned i;
    fill(buf); s.buffer = buf; s.map_id = 3;
    CHECK(pr_u3bd_entry2(&s, 15, 15, 0, &got, &rep) == PR_U3BD_NIBBLE_OK && rep.offset == 0xFF && got == (buf[0xFF] >> 4));
    CHECK(pr_u3bd_entry2(&s, 15, 15, 6, &got, &rep) == PR_U3BD_NIBBLE_OK && rep.offset == 0x1FF && got == (buf[0x1FF] & 0xF));
    CHECK(pr_u3bd_entry2(&s, 3, 2, 2, &got, &rep) == PR_U3BD_NIBBLE_OK && rep.offset == 0x23);          /* x is the column */
    CHECK(pr_u3bd_entry2(&s, 16, 3, 4, &got, &rep) == PR_U3BD_NIBBLE_OK && rep.offset == 0x130);        /* 16 wraps to 0 */
    CHECK(pr_u3bd_entry2(&s, 0xFF, 3, 0, &got, &rep) == PR_U3BD_NIBBLE_OK && rep.offset == 0x3F);       /* -1 wraps to 15 */
    CHECK(pr_u3bd_entry2(&s, 0x80, 0x80, 6, &got, &rep) == PR_U3BD_NIBBLE_OK && rep.offset == 0x1FF);   /* -128 wraps to 15 */
    CHECK(pr_u3bd_entry2(&s, 0x7F, 0x10, 2, &got, &rep) == PR_U3BD_NIBBLE_OK && rep.offset == 0);       /* 127 and 16 wrap to 0 */
    /* only quarters 0 and 1 are ever read: inverting the other 512 bytes changes nothing */
    { unsigned x, y, sel; uint8_t alt[PR_U3BD_NIBBLE_BUFFER_SIZE]; pr_u3bd_nibble_state t = s;
      memcpy(alt, buf, sizeof alt);
      for (i = 0x200u; i < 0x400u; ++i) alt[i] ^= 0xFFu;
      t.buffer = alt;
      for (sel = 0; sel < 8u; sel += 2u) for (x = 0; x < 256u; x += 17u) for (y = 0; y < 256u; y += 17u) {
          uint8_t a, b;
          CHECK(pr_u3bd_entry2(&s, (uint8_t)x, (uint8_t)y, (uint8_t)sel, &a, NULL) == PR_U3BD_NIBBLE_OK);
          CHECK(pr_u3bd_entry2(&t, (uint8_t)x, (uint8_t)y, (uint8_t)sel, &b, NULL) == PR_U3BD_NIBBLE_OK);
          CHECK(a == b);
      } }
    return 0;
}

/* The unmatched-selector path returns the byte already in the stack slot [bp-1]
 * (frame_residue), never the caller's initial *result; the zero-map gate and a
 * recognised selector override it. */
static int test_residue(void)
{
    static const uint8_t residues[] = {0x00, 0xA5, 0xFF, 0x10, 0x7E};
    static const uint8_t initials[] = {0x00, 0xA5, 0xFF, 0x3C};
    static const uint8_t odd_sels[] = {1, 3, 5, 7, 8, 9, 0x7F, 0x80, 0xFE, 0xFF};
    uint8_t buf[PR_U3BD_NIBBLE_BUFFER_SIZE]; pr_u3bd_nibble_state s; pr_u3bd_nibble_report rep; uint8_t got;
    unsigned i, j, k;
    fill(buf); s.buffer = buf;
    buf[16 * 9 + 7] = 0xA7; buf[0x100 + 16 * 9 + 7] = 0x3C;
    for (i = 0; i < sizeof residues; ++i) {
        s.frame_residue = residues[i];
        for (j = 0; j < sizeof initials; ++j) for (k = 0; k < sizeof odd_sels; ++k) {
            /* in range on any map: the residue comes back whatever *result held */
            s.map_id = 1; got = initials[j];
            CHECK(pr_u3bd_entry2(&s, 7, 9, odd_sels[k], &got, &rep) == PR_U3BD_NIBBLE_UNSPECIFIED);
            CHECK(got == residues[i] && rep.residue_returned == 1 && rep.read == 0 && rep.in_range == 1);
            s.map_id = 0; got = initials[j];
            CHECK(pr_u3bd_entry2(&s, 7, 9, odd_sels[k], &got, &rep) == PR_U3BD_NIBBLE_UNSPECIFIED && got == residues[i]);
            s.map_id = 10; got = initials[j];
            CHECK(pr_u3bd_entry2(&s, 7, 9, odd_sels[k], &got, &rep) == PR_U3BD_NIBBLE_UNSPECIFIED && got == residues[i]);
            /* out of range on map 0 / 10: the zero gate overrides the residue */
            s.map_id = 0; got = initials[j];
            CHECK(pr_u3bd_entry2(&s, 16, 9, odd_sels[k], &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0 && rep.residue_returned == 0);
            s.map_id = 10; got = initials[j];
            CHECK(pr_u3bd_entry2(&s, 7, 0xFF, odd_sels[k], &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0 && rep.residue_returned == 0);
            /* out of range on another map: wrap, then the residue */
            s.map_id = 5; got = initials[j];
            CHECK(pr_u3bd_entry2(&s, 16, 9, odd_sels[k], &got, &rep) == PR_U3BD_NIBBLE_UNSPECIFIED && got == residues[i] && rep.wrapped_x == 1);
            /* a null buffer does not matter on this path */
            { pr_u3bd_nibble_state t = s; t.buffer = NULL; got = initials[j];
              CHECK(pr_u3bd_entry2(&t, 7, 9, odd_sels[k], &got, &rep) == PR_U3BD_NIBBLE_UNSPECIFIED && got == residues[i]); }
        }
        /* a recognised selector overrides the residue on every map */
        for (j = 0; j < 256u; ++j) {
            s.map_id = (uint8_t)j; got = 0xEE;
            CHECK(pr_u3bd_entry2(&s, 7, 9, 0, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0xA && rep.residue_returned == 0);
            CHECK(pr_u3bd_entry2(&s, 7, 9, 2, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0x7);
            CHECK(pr_u3bd_entry2(&s, 7, 9, 4, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0x3);
            CHECK(pr_u3bd_entry2(&s, 7, 9, 6, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0xC);
        }
    }
    return 0;
}

static int test_bounds(void)
{
    uint8_t buf[PR_U3BD_NIBBLE_BUFFER_SIZE]; pr_u3bd_nibble_state s; pr_u3bd_nibble_report rep; uint8_t got = 0xA5;
    fill(buf); s.buffer = buf; s.map_id = 1; s.frame_residue = 0x77;
    CHECK(pr_u3bd_entry2(NULL, 0, 0, 0, &got, &rep) == PR_U3BD_NIBBLE_ARGUMENT && got == 0xA5);
    CHECK(pr_u3bd_entry2(&s, 0, 0, 0, NULL, &rep) == PR_U3BD_NIBBLE_ARGUMENT);
    /* a null buffer: the zero path and the unspecified path never touch [0x6A5C]; the reads do */
    s.buffer = NULL; s.map_id = 0;
    CHECK(pr_u3bd_entry2(&s, 16, 0, 0, &got, &rep) == PR_U3BD_NIBBLE_OK && got == 0 && rep.returned_zero == 1);
    got = 0xA5;
    CHECK(pr_u3bd_entry2(&s, 1, 1, 1, &got, &rep) == PR_U3BD_NIBBLE_UNSPECIFIED && got == 0x77);
    got = 0xA5;
    CHECK(pr_u3bd_entry2(&s, 1, 1, 4, &got, &rep) == PR_U3BD_NIBBLE_ARGUMENT && got == 0xA5 && rep.read == 0 && rep.offset == 0x111);
    s.map_id = 1;
    CHECK(pr_u3bd_entry2(&s, 16, 0, 2, &got, &rep) == PR_U3BD_NIBBLE_ARGUMENT && got == 0xA5 && rep.wrapped_x == 1);
    s.buffer = buf;
    CHECK(pr_u3bd_entry2(&s, 1, 1, 4, &got, &rep) == PR_U3BD_NIBBLE_OK && got == (buf[0x111] >> 4));
    CHECK(pr_u3bd_entry2(&s, 1, 1, 6, &got, NULL) == PR_U3BD_NIBBLE_OK && got == (buf[0x111] & 0xF));   /* report optional */
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_exhaustive();
    rc |= test_selectors_and_maps();
    rc |= test_pinned();
    rc |= test_residue();
    rc |= test_bounds();
    if (rc == 0) puts("unit 0x3bd entry 2 translation tests passed");
    return rc;
}
