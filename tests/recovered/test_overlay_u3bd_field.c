/* Synthetic tests for the unit 0x3bd entry 3 translation: buffer, map id
 * and the nested entry 2 residue are fabricated; an independent restatement
 * of the rule on raw bytes is compared with the translation over every
 * (x, y) pair for a set of selectors, map ids and residues. */
#include "poolrad/recovered/overlay_u3bd_field.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* Independent restatement on raw bytes. Entry 2's nibble is restated too
 * (selector bit 2 = quarter, bit 1 = low nibble; other selectors give the
 * residue). Result: zero gate -> 0; nibble 0 -> 1; selector 6/4/2/0 ->
 * the 2-bit field at bit position selector of the quarter-3 byte; else 1. */
static int ref_wrap(uint8_t b) { return b <= 0x0Fu ? b : (b <= 0x7Fu ? 0 : 15); }
static int ref_in(uint8_t b) { return b <= 0x0Fu; }
static int ref_field(uint8_t map, uint8_t x, uint8_t y, uint8_t sel, uint8_t residue, const uint8_t *buf)
{
    unsigned cell, nib, b;
    if (!(ref_in(x) && ref_in(y)) && (map == 0u || map == 10u)) return 0;
    cell = 16u * (unsigned)ref_wrap(y) + (unsigned)ref_wrap(x);
    if (sel == 0u || sel == 2u || sel == 4u || sel == 6u) {
        b = buf[(sel & 4u ? 0x100u : 0u) + cell];
        nib = (sel & 2u) ? (b & 0x0Fu) : (b >> 4);
    } else nib = residue;
    if (nib == 0u) return 1;
    if (sel == 0u || sel == 2u || sel == 4u || sel == 6u) return (buf[0x300u + cell] >> sel) & 3u;
    return 1;
}

static void fill(uint8_t *buf)
{
    unsigned i;
    for (i = 0; i < PR_U3BD_FIELD_BUFFER_SIZE; ++i) buf[i] = (uint8_t)(i * 13u + 5u);
    /* make sure both zero and non-zero nibbles occur in quarters 0 and 1 at known cells */
    buf[0x00] = 0x0F; buf[0x01] = 0xF0; buf[0x02] = 0x00; buf[0x100] = 0x0F; buf[0x101] = 0xF0; buf[0x102] = 0x00;
    buf[0x300] = 0xE4;   /* fields 3,2,1,0 from bit 6 down */
    buf[0x301] = 0x1B;   /* fields 0,1,2,3 */
    buf[0x302] = 0xFF;
}

static int test_exhaustive(void)
{
    static const uint8_t maps[] = {0, 10, 1};
    static const uint8_t sels[] = {0, 2, 4, 6, 1, 3, 5, 7, 0xFF};
    static const uint8_t residues[] = {0x00, 0xA5, 0xFF};
    uint8_t buf[PR_U3BD_FIELD_BUFFER_SIZE]; pr_u3bd_field_state s; pr_u3bd_field_report rep;
    unsigned m, k, q, x, y;
    fill(buf); s.buffer = buf;
    for (m = 0; m < sizeof maps; ++m) for (k = 0; k < sizeof sels; ++k) for (q = 0; q < sizeof residues; ++q) {
        s.map_id = maps[m]; s.entry2_frame_residue = residues[q];
        for (x = 0; x < 256u; ++x) for (y = 0; y < 256u; ++y) {
            uint8_t got = 0xA5; int want = ref_field(maps[m], (uint8_t)x, (uint8_t)y, sels[k], residues[q], buf);
            CHECK(pr_u3bd_entry3(&s, (uint8_t)x, (uint8_t)y, sels[k], &got, &rep) == PR_U3BD_FIELD_OK);
            CHECK(got == (uint8_t)want);
            CHECK(rep.in_range == (unsigned)(ref_in((uint8_t)x) && ref_in((uint8_t)y)));
            if (rep.returned_zero) {
                CHECK(got == 0 && rep.read == 0 && rep.wrapped_x == 0 && rep.wrapped_y == 0);
            } else {
                CHECK(rep.x_used == ref_wrap((uint8_t)x) && rep.y_used == ref_wrap((uint8_t)y));
                CHECK(rep.wrapped_x == (unsigned)!ref_in((uint8_t)x) && rep.wrapped_y == (unsigned)!ref_in((uint8_t)y));
                CHECK(rep.nibble_from_residue == (unsigned)(sels[k] != 0u && sels[k] != 2u && sels[k] != 4u && sels[k] != 6u));
                if (rep.nibble_from_residue) CHECK(rep.nibble == residues[q] && got == 1 && rep.defaulted == 1 && rep.read == 0);
                if (rep.read) {
                    CHECK(rep.offset >= 0x300u && rep.offset <= 0x3FFu && rep.offset == 0x300u + 16u * rep.y_used + rep.x_used);
                    CHECK(rep.shift == sels[k] && got <= 3u && rep.defaulted == 0 && rep.nibble != 0);
                } else {
                    CHECK(got == 1 && rep.defaulted == 1);
                }
            }
        }
    }
    return 0;
}

static int test_pinned(void)
{
    uint8_t buf[PR_U3BD_FIELD_BUFFER_SIZE]; pr_u3bd_field_state s; pr_u3bd_field_report rep; uint8_t got;
    unsigned sel, r;
    fill(buf); s.buffer = buf; s.map_id = 1; s.entry2_frame_residue = 0;
    /* cell (0,0): q0 byte 0x0F (high nibble 0, low F), q1 byte 0x0F; q3 byte 0xE4 = 11 10 01 00 */
    CHECK(pr_u3bd_entry3(&s, 0, 0, 0, &got, &rep) == PR_U3BD_FIELD_OK && got == 1 && rep.nibble == 0 && rep.defaulted == 1);   /* high nibble 0 -> default */
    CHECK(pr_u3bd_entry3(&s, 0, 0, 2, &got, &rep) == PR_U3BD_FIELD_OK && got == 1 && rep.nibble == 0xF && rep.shift == 2);     /* bits 3..2 of 0xE4 = 01 */
    CHECK(pr_u3bd_entry3(&s, 0, 0, 4, &got, &rep) == PR_U3BD_FIELD_OK && got == 1 && rep.nibble == 0);
    CHECK(pr_u3bd_entry3(&s, 0, 0, 6, &got, &rep) == PR_U3BD_FIELD_OK && got == 3 && rep.nibble == 0xF && rep.shift == 6);     /* bits 7..6 = 11 */
    /* cell (1,0): q0/q1 bytes 0xF0 (high F, low 0); q3 byte 0x1B = 00 01 10 11 */
    CHECK(pr_u3bd_entry3(&s, 1, 0, 0, &got, &rep) == PR_U3BD_FIELD_OK && got == 3 && rep.offset == 0x301 && rep.shift == 0);
    CHECK(pr_u3bd_entry3(&s, 1, 0, 2, &got, &rep) == PR_U3BD_FIELD_OK && got == 1 && rep.nibble == 0 && rep.read == 0);
    CHECK(pr_u3bd_entry3(&s, 1, 0, 4, &got, &rep) == PR_U3BD_FIELD_OK && got == 1 && rep.nibble == 0xF && rep.shift == 4);     /* bits 5..4 = 01 */
    CHECK(pr_u3bd_entry3(&s, 1, 0, 6, &got, &rep) == PR_U3BD_FIELD_OK && got == 1 && rep.nibble == 0);
    /* cell (2,0): nibbles all zero -> 1 for every recognised selector, q3 byte 0xFF never read */
    for (sel = 0; sel < 8u; sel += 2u) CHECK(pr_u3bd_entry3(&s, 2, 0, (uint8_t)sel, &got, &rep) == PR_U3BD_FIELD_OK && got == 1 && rep.read == 0);
    /* every selector 0..255 at cell (0,0): only 0,2,4,6 can read; all others return 1 for every residue */
    for (r = 0; r < 256u; r += 51u) {
        s.entry2_frame_residue = (uint8_t)r;
        for (sel = 0; sel < 256u; ++sel) {
            got = 0xEE;
            CHECK(pr_u3bd_entry3(&s, 0, 0, (uint8_t)sel, &got, &rep) == PR_U3BD_FIELD_OK);
            if (sel == 0u || sel == 2u || sel == 4u || sel == 6u) CHECK(rep.nibble_from_residue == 0);
            else CHECK(got == 1 && rep.nibble_from_residue == 1 && rep.nibble == r && rep.read == 0);
        }
    }
    /* wraps: (16, 3) -> (0, 3); (0xFF, 3) -> (15, 3); (0x80, 0x80) -> (15, 15) */
    s.entry2_frame_residue = 0; buf[0x030] = 0xFF; buf[0x330] = 0x80; buf[0x13F] = 0xFF; buf[0x33F] = 0x0C; buf[0x1FF] = 0xFF; buf[0x3FF] = 0x30;
    CHECK(pr_u3bd_entry3(&s, 16, 3, 0, &got, &rep) == PR_U3BD_FIELD_OK && rep.offset == 0x330 && got == 0);
    CHECK(pr_u3bd_entry3(&s, 16, 3, 6, &got, &rep) == PR_U3BD_FIELD_OK && rep.offset == 0x330 && got == 2);
    CHECK(pr_u3bd_entry3(&s, 0xFF, 3, 4, &got, &rep) == PR_U3BD_FIELD_OK && rep.offset == 0x33F && got == 0);
    CHECK(pr_u3bd_entry3(&s, 0xFF, 3, 2, &got, &rep) == PR_U3BD_FIELD_OK && rep.offset == 0x33F && got == 3);
    CHECK(pr_u3bd_entry3(&s, 0x80, 0x80, 6, &got, &rep) == PR_U3BD_FIELD_OK && rep.offset == 0x3FF && got == 0);
    CHECK(pr_u3bd_entry3(&s, 0x80, 0x80, 4, &got, &rep) == PR_U3BD_FIELD_OK && rep.offset == 0x3FF && got == 3);
    /* map gate: every map id at (16, 0); only 0 and 10 return 0 and they do so before entry 2 is called */
    for (r = 0; r < 256u; ++r) {
        s.map_id = (uint8_t)r; got = 0xEE;
        CHECK(pr_u3bd_entry3(&s, 16, 0, 1, &got, &rep) == PR_U3BD_FIELD_OK);
        if (r == 0u || r == 10u) CHECK(got == 0 && rep.returned_zero == 1 && rep.nibble_from_residue == 0);
        else CHECK(got == 1 && rep.returned_zero == 0 && rep.nibble_from_residue == 1);
        CHECK(pr_u3bd_entry3(&s, 0, 0, 6, &got, &rep) == PR_U3BD_FIELD_OK && got == 3);   /* in range: map id irrelevant */
    }
    /* only quarters 0, 1 and 3 are read: inverting quarter 2 changes nothing */
    { unsigned x, y; uint8_t alt[PR_U3BD_FIELD_BUFFER_SIZE]; pr_u3bd_field_state t;
      s.map_id = 3; memcpy(alt, buf, sizeof alt); t = s; t.buffer = alt;
      for (r = 0x200u; r < 0x300u; ++r) alt[r] ^= 0xFFu;
      for (sel = 0; sel < 8u; sel += 2u) for (x = 0; x < 256u; x += 17u) for (y = 0; y < 256u; y += 17u) {
          uint8_t a, b;
          CHECK(pr_u3bd_entry3(&s, (uint8_t)x, (uint8_t)y, (uint8_t)sel, &a, NULL) == PR_U3BD_FIELD_OK);
          CHECK(pr_u3bd_entry3(&t, (uint8_t)x, (uint8_t)y, (uint8_t)sel, &b, NULL) == PR_U3BD_FIELD_OK);
          CHECK(a == b);
      } }
    return 0;
}

static int test_bounds(void)
{
    uint8_t buf[PR_U3BD_FIELD_BUFFER_SIZE]; pr_u3bd_field_state s; pr_u3bd_field_report rep; uint8_t got = 0xA5;
    fill(buf); s.buffer = buf; s.map_id = 1; s.entry2_frame_residue = 0x77;
    CHECK(pr_u3bd_entry3(NULL, 0, 0, 0, &got, &rep) == PR_U3BD_FIELD_ARGUMENT && got == 0xA5);
    CHECK(pr_u3bd_entry3(&s, 0, 0, 0, NULL, &rep) == PR_U3BD_FIELD_ARGUMENT);
    /* a null buffer: the zero path and the unmatched-selector path never touch [0x6A5C] */
    s.buffer = NULL; s.map_id = 0;
    CHECK(pr_u3bd_entry3(&s, 16, 0, 0, &got, &rep) == PR_U3BD_FIELD_OK && got == 0 && rep.returned_zero == 1);
    CHECK(pr_u3bd_entry3(&s, 1, 1, 1, &got, &rep) == PR_U3BD_FIELD_OK && got == 1 && rep.nibble_from_residue == 1);
    s.entry2_frame_residue = 0;
    CHECK(pr_u3bd_entry3(&s, 1, 1, 1, &got, &rep) == PR_U3BD_FIELD_OK && got == 1 && rep.nibble == 0);
    /* a recognised selector reads inside entry 2 first: ARGUMENT there, result untouched */
    got = 0xA5;
    CHECK(pr_u3bd_entry3(&s, 1, 1, 4, &got, &rep) == PR_U3BD_FIELD_ARGUMENT && got == 0xA5 && rep.read == 0);
    s.map_id = 1; got = 0xA5;
    CHECK(pr_u3bd_entry3(&s, 16, 0, 2, &got, &rep) == PR_U3BD_FIELD_ARGUMENT && got == 0xA5 && rep.wrapped_x == 1);
    s.buffer = buf;
    CHECK(pr_u3bd_entry3(&s, 0, 0, 6, &got, NULL) == PR_U3BD_FIELD_OK && got == 3);   /* report optional */
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_exhaustive();
    rc |= test_pinned();
    rc |= test_bounds();
    if (rc == 0) puts("unit 0x3bd entry 3 translation tests passed");
    return rc;
}
