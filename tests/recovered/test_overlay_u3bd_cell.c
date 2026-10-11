/* Synthetic tests for the unit 0x3bd entry 4 translation: the buffer and
 * the map id are fabricated; an independent restatement of the rule on raw
 * bytes is compared with the translation over every input. */
#include "poolrad/recovered/overlay_u3bd_cell.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* Independent restatement on raw bytes: 0x00..0x0F is in range; 0x10..0x7F
 * wraps to 0 (positive and above 15); 0x80..0xFF wraps to 15 (negative). */
static int ref_wrap(uint8_t b) { return b <= 0x0Fu ? b : (b <= 0x7Fu ? 0 : 15); }
static int ref_in(uint8_t b) { return b <= 0x0Fu; }
static uint8_t ref_cell(uint8_t map, uint8_t x, uint8_t y, const uint8_t *buf, int *read)
{
    *read = 1;
    if (!(ref_in(x) && ref_in(y)) && (map == 0u || map == 10u)) { *read = 0; return 0; }
    return buf[0x200u + 16u * (unsigned)ref_wrap(y) + (unsigned)ref_wrap(x)];
}

static void fill(uint8_t *buf)
{
    unsigned i;
    for (i = 0; i < PR_U3BD_CELL_BUFFER_SIZE; ++i) buf[i] = (uint8_t)(i * 13u + 5u);
}

static int test_exhaustive(void)
{
    static const uint8_t maps[] = {0, 10, 1, 9, 11, 0x7F, 0x80, 0xFF};
    uint8_t buf[PR_U3BD_CELL_BUFFER_SIZE]; pr_u3bd_cell_state s; pr_u3bd_cell_report rep;
    unsigned m, x, y;
    fill(buf); s.buffer = buf;
    for (m = 0; m < sizeof maps; ++m) {
        s.map_id = maps[m];
        for (x = 0; x < 256u; ++x) for (y = 0; y < 256u; ++y) {
            uint8_t got = 0xA5, want; int read;
            want = ref_cell(maps[m], (uint8_t)x, (uint8_t)y, buf, &read);
            CHECK(pr_u3bd_entry4(&s, (uint8_t)x, (uint8_t)y, &got, &rep) == PR_U3BD_CELL_OK);
            CHECK(got == want);
            CHECK(rep.in_range == (unsigned)(ref_in((uint8_t)x) && ref_in((uint8_t)y)));
            CHECK(rep.returned_zero == (unsigned)!read);
            if (read) {
                CHECK(rep.offset >= 0x200u && rep.offset <= 0x2FFu);           /* third quarter only */
                CHECK(rep.x_used == ref_wrap((uint8_t)x) && rep.y_used == ref_wrap((uint8_t)y));
                CHECK(rep.wrapped_x == (unsigned)!ref_in((uint8_t)x) && rep.wrapped_y == (unsigned)!ref_in((uint8_t)y));
                CHECK(rep.offset == 0x200u + 16u * rep.y_used + rep.x_used);
            } else {
                CHECK(rep.wrapped_x == 0 && rep.wrapped_y == 0 && rep.offset == 0);
            }
        }
    }
    return 0;
}

static int test_map_gate(void)
{
    uint8_t buf[PR_U3BD_CELL_BUFFER_SIZE]; pr_u3bd_cell_state s; pr_u3bd_cell_report rep;
    unsigned m;
    fill(buf); s.buffer = buf;
    for (m = 0; m < 256u; ++m) {
        uint8_t got = 0xA5;
        s.map_id = (uint8_t)m;
        CHECK(pr_u3bd_entry4(&s, 16, 0, &got, &rep) == PR_U3BD_CELL_OK);
        if (m == 0u || m == 10u) CHECK(got == 0 && rep.returned_zero == 1);
        else CHECK(got == buf[0x200] && rep.returned_zero == 0 && rep.wrapped_x == 1 && rep.x_used == 0);
        /* in range: the map id never matters and nothing wraps */
        CHECK(pr_u3bd_entry4(&s, 7, 9, &got, &rep) == PR_U3BD_CELL_OK);
        CHECK(got == buf[0x200 + 16 * 9 + 7] && rep.in_range == 1 && rep.wrapped_x == 0 && rep.wrapped_y == 0);
    }
    return 0;
}

static int test_pinned(void)
{
    uint8_t buf[PR_U3BD_CELL_BUFFER_SIZE]; pr_u3bd_cell_state s; pr_u3bd_cell_report rep; uint8_t got;
    fill(buf); s.buffer = buf; s.map_id = 1;
    CHECK(pr_u3bd_entry4(&s, 0, 0, &got, &rep) == PR_U3BD_CELL_OK && got == buf[0x200] && rep.offset == 0x200);
    CHECK(pr_u3bd_entry4(&s, 15, 15, &got, &rep) == PR_U3BD_CELL_OK && got == buf[0x2FF] && rep.offset == 0x2FF);
    CHECK(pr_u3bd_entry4(&s, 3, 2, &got, &rep) == PR_U3BD_CELL_OK && got == buf[0x200 + 35]);   /* x is the column */
    CHECK(pr_u3bd_entry4(&s, 16, 3, &got, &rep) == PR_U3BD_CELL_OK && got == buf[0x200 + 48]);   /* 16 wraps to 0 */
    CHECK(pr_u3bd_entry4(&s, 0xFF, 3, &got, &rep) == PR_U3BD_CELL_OK && got == buf[0x200 + 48 + 15]); /* -1 wraps to 15 */
    CHECK(pr_u3bd_entry4(&s, 0x80, 0x80, &got, &rep) == PR_U3BD_CELL_OK && got == buf[0x2FF]);  /* -128 wraps to 15 */
    CHECK(pr_u3bd_entry4(&s, 0x7F, 0x10, &got, &rep) == PR_U3BD_CELL_OK && got == buf[0x200]);  /* 127 and 16 wrap to 0 */
    CHECK(pr_u3bd_entry4(&s, 5, 0xF0, &got, &rep) == PR_U3BD_CELL_OK && got == buf[0x2F5] && rep.wrapped_x == 0 && rep.wrapped_y == 1);
    s.map_id = 0;
    CHECK(pr_u3bd_entry4(&s, 16, 3, &got, &rep) == PR_U3BD_CELL_OK && got == 0 && rep.returned_zero == 1);
    CHECK(pr_u3bd_entry4(&s, 15, 15, &got, &rep) == PR_U3BD_CELL_OK && got == buf[0x2FF]);      /* in range reads on map 0 too */
    s.map_id = 10;
    CHECK(pr_u3bd_entry4(&s, 0xFF, 0, &got, &rep) == PR_U3BD_CELL_OK && got == 0 && rep.returned_zero == 1);
    /* only the third quarter is ever read: changing the other 768 bytes changes nothing */
    { unsigned i, x, y; uint8_t alt[PR_U3BD_CELL_BUFFER_SIZE];
      memcpy(alt, buf, sizeof alt);
      for (i = 0; i < 0x200u; ++i) alt[i] ^= 0xFFu;
      for (i = 0x300u; i < 0x400u; ++i) alt[i] ^= 0xFFu;
      s.map_id = 3;
      for (x = 0; x < 256u; x += 17u) for (y = 0; y < 256u; y += 17u) {
          uint8_t a, b; pr_u3bd_cell_state t = s; t.buffer = alt;
          CHECK(pr_u3bd_entry4(&s, (uint8_t)x, (uint8_t)y, &a, NULL) == PR_U3BD_CELL_OK);
          CHECK(pr_u3bd_entry4(&t, (uint8_t)x, (uint8_t)y, &b, NULL) == PR_U3BD_CELL_OK);
          CHECK(a == b);
      } }
    return 0;
}

static int test_bounds(void)
{
    uint8_t buf[PR_U3BD_CELL_BUFFER_SIZE]; pr_u3bd_cell_state s; pr_u3bd_cell_report rep; uint8_t got = 0xA5;
    fill(buf); s.buffer = buf; s.map_id = 1;
    CHECK(pr_u3bd_entry4(NULL, 0, 0, &got, &rep) == PR_U3BD_CELL_ARGUMENT && got == 0xA5);
    CHECK(pr_u3bd_entry4(&s, 0, 0, NULL, &rep) == PR_U3BD_CELL_ARGUMENT);
    /* a null buffer: the zero path never touches [0x6A5C]; every read path does */
    s.buffer = NULL; s.map_id = 0;
    CHECK(pr_u3bd_entry4(&s, 16, 0, &got, &rep) == PR_U3BD_CELL_OK && got == 0 && rep.returned_zero == 1);
    got = 0xA5;
    CHECK(pr_u3bd_entry4(&s, 1, 1, &got, &rep) == PR_U3BD_CELL_ARGUMENT && got == 0xA5 && rep.in_range == 1 && rep.offset == 0x211);
    s.map_id = 1;
    CHECK(pr_u3bd_entry4(&s, 16, 0, &got, &rep) == PR_U3BD_CELL_ARGUMENT && got == 0xA5 && rep.wrapped_x == 1);
    s.buffer = buf;
    CHECK(pr_u3bd_entry4(&s, 1, 1, &got, NULL) == PR_U3BD_CELL_OK && got == buf[0x211]);   /* report optional */
    return 0;
}

int main(void)
{
    int rc = 0;
    rc |= test_exhaustive();
    rc |= test_map_gate();
    rc |= test_pinned();
    rc |= test_bounds();
    if (rc == 0) puts("unit 0x3bd entry 4 translation tests passed");
    return rc;
}
