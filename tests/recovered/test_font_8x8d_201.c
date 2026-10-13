#include "poolrad/recovered/font_8x8d_201.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

static uint8_t g_block[PR_FONT201_BLOCK_SIZE];
static pr_slot_a_view_frame g_frame;

static void make_font(void) {
    size_t g, r;
    for (g = 0; g < PR_FONT201_GLYPH_COUNT; ++g) {
        for (r = 0; r < 8; ++r) {
            g_block[g * 8 + r] = (uint8_t)((g * 8 + r) & 0xFF);   /* distinct, deterministic */
        }
    }
    /* glyph 1: a diagonal; glyph 32: blank; glyph 176: all set */
    for (r = 0; r < 8; ++r) { g_block[8 + r] = (uint8_t)(0x80u >> r); g_block[32 * 8 + r] = 0; g_block[176 * 8 + r] = 0xFF; }
}

static int test_open(void) {
    pr_font201 f;
    CHECK(pr_font201_open(g_block, PR_FONT201_BLOCK_SIZE, &f) == PR_FONT201_OK && f.data == g_block && f.size == 1416);
    CHECK(pr_font201_open(g_block, 1415, &f) == PR_FONT201_SIZE && f.data == NULL);
    CHECK(pr_font201_open(g_block, 1417, &f) == PR_FONT201_SIZE);
    CHECK(pr_font201_open(NULL, PR_FONT201_BLOCK_SIZE, &f) == PR_FONT201_ARGUMENT);
    CHECK(pr_font201_open(g_block, PR_FONT201_BLOCK_SIZE, NULL) == PR_FONT201_ARGUMENT);
    CHECK(PR_FONT201_GLYPH_COUNT == 177);
    return 0;
}

static int test_glyph_index(void) {
    unsigned c;
    CHECK(pr_font201_glyph_index('A', 1) == 1 && pr_font201_glyph_index('a', 1) == 1);
    CHECK(pr_font201_glyph_index('Z', 1) == 26 && pr_font201_glyph_index('z', 1) == 26);
    CHECK(pr_font201_glyph_index('@', 1) == 0 && pr_font201_glyph_index(' ', 1) == 32);
    CHECK(pr_font201_glyph_index('0', 1) == 48 && pr_font201_glyph_index('9', 1) == 57);
    CHECK(pr_font201_glyph_index('_', 1) == 31 && pr_font201_glyph_index(0x80, 1) == 0 && pr_font201_glyph_index(0xC1, 1) == 1);
    for (c = 0; c < 256; ++c) {
        CHECK(pr_font201_glyph_index((uint8_t)c, 0) == c);                  /* raw glyph index */
        CHECK(pr_font201_glyph_index((uint8_t)c, 1) < 64);
    }
    return 0;
}

static int test_glyph_row(void) {
    pr_font201 f; uint8_t b = 0;
    CHECK(pr_font201_open(g_block, PR_FONT201_BLOCK_SIZE, &f) == PR_FONT201_OK);
    CHECK(pr_font201_glyph_row(&f, 1, 0, &b) == PR_FONT201_OK && b == 0x80);
    CHECK(pr_font201_glyph_row(&f, 1, 7, &b) == PR_FONT201_OK && b == 0x01);
    CHECK(pr_font201_glyph_row(&f, 176, 3, &b) == PR_FONT201_OK && b == 0xFF);
    CHECK(pr_font201_glyph_row(&f, 177, 0, &b) == PR_FONT201_GLYPH);
    CHECK(pr_font201_glyph_row(&f, 1, 8, &b) == PR_FONT201_ARGUMENT);
    CHECK(pr_font201_glyph_row(NULL, 1, 0, &b) == PR_FONT201_ARGUMENT);
    CHECK(pr_font201_glyph_row(&f, 1, 0, NULL) == PR_FONT201_ARGUMENT);
    return 0;
}

static int cell_matches(uint8_t column, uint8_t row, size_t glyph, uint8_t color) {
    size_t r, x;
    for (r = 0; r < 8; ++r) for (x = 0; x < 8; ++x) {
        size_t t = ((size_t)row * 8 + r) * PR_SLOT_A_VIEW_WIDTH + (size_t)column * 8 + x;
        uint8_t set = (uint8_t)((g_block[glyph * 8 + r] >> (7 - x)) & 1);
        if (g_frame.index[t] != (set ? (color & 0x0F) : 0) || g_frame.covered[t] != 1) return 0;
    }
    return 1;
}

static int test_draw_glyph(void) {
    pr_font201 f; size_t covered;
    CHECK(pr_font201_open(g_block, PR_FONT201_BLOCK_SIZE, &f) == PR_FONT201_OK);
    pr_slot_a_view_clear(&g_frame);
    CHECK(pr_font201_draw_glyph_ega(&g_frame, &f, 3, 2, 14, 1) == PR_FONT201_OK);
    CHECK(cell_matches(3, 2, 1, 14) && pr_slot_a_view_covered(&g_frame) == 64);
    /* the clear bits are written as 0 (opaque), not left alone */
    CHECK(g_frame.index[(2 * 8 + 0) * PR_SLOT_A_VIEW_WIDTH + 3 * 8 + 1] == 0);
    /* color bits above 0x0F are not planes */
    CHECK(pr_font201_draw_glyph_ega(&g_frame, &f, 4, 2, 0xF7, 176) == PR_FONT201_OK && cell_matches(4, 2, 176, 0xF7));
    CHECK(g_frame.index[(2 * 8) * PR_SLOT_A_VIEW_WIDTH + 4 * 8] == 7);
    /* corners of the guard */
    CHECK(pr_font201_draw_glyph_ega(&g_frame, &f, 39, 24, 1, 1) == PR_FONT201_OK && cell_matches(39, 24, 1, 1));
    CHECK(pr_font201_draw_glyph_ega(&g_frame, &f, 40, 0, 1, 1) == PR_FONT201_BOUNDS);
    CHECK(pr_font201_draw_glyph_ega(&g_frame, &f, 0, 25, 1, 1) == PR_FONT201_BOUNDS);
    CHECK(pr_font201_draw_glyph_ega(&g_frame, &f, 0, 0, 1, 177) == PR_FONT201_GLYPH);
    covered = pr_slot_a_view_covered(&g_frame);
    CHECK(covered == 3 * 64);
    CHECK(pr_font201_draw_glyph_ega(NULL, &f, 0, 0, 1, 1) == PR_FONT201_ARGUMENT);
    CHECK(pr_font201_draw_glyph_ega(&g_frame, NULL, 0, 0, 1, 1) == PR_FONT201_ARGUMENT);
    return 0;
}

static int test_draw_count(void) {
    pr_font201 f; size_t drawn;
    CHECK(pr_font201_open(g_block, PR_FONT201_BLOCK_SIZE, &f) == PR_FONT201_OK);
    pr_slot_a_view_clear(&g_frame);
    CHECK(pr_font201_draw_ega(&g_frame, &f, 10, 5, 9, 3, 'a', 1, &drawn) == PR_FONT201_OK && drawn == 3);
    CHECK(cell_matches(10, 5, 1, 9) && cell_matches(11, 5, 1, 9) && cell_matches(12, 5, 1, 9));
    CHECK(pr_slot_a_view_covered(&g_frame) == 3 * 64);
    /* count 0 draws nothing; raw glyph without upcase; copies past column 39 are refused */
    CHECK(pr_font201_draw_ega(&g_frame, &f, 0, 0, 9, 0, 'a', 1, &drawn) == PR_FONT201_OK && drawn == 0);
    CHECK(pr_font201_draw_ega(&g_frame, &f, 0, 1, 9, 1, 176, 0, &drawn) == PR_FONT201_OK && drawn == 1 && cell_matches(0, 1, 176, 9));
    CHECK(pr_font201_draw_ega(&g_frame, &f, 38, 1, 9, 3, 'a', 1, &drawn) == PR_FONT201_BOUNDS && drawn == 2);
    CHECK(pr_font201_draw_ega(&g_frame, &f, 40, 1, 9, 1, 'a', 1, &drawn) == PR_FONT201_BOUNDS && drawn == 0);
    CHECK(pr_font201_draw_ega(&g_frame, &f, 0, 0, 9, 1, 200, 0, &drawn) == PR_FONT201_GLYPH);
    /* byte-column wrap: from column 30 with count 255 the copies at 30..39 draw, 40..255 are
     * refused, and after the byte wraps the copies at 0..28 draw again */
    pr_slot_a_view_clear(&g_frame);
    CHECK(pr_font201_draw_ega(&g_frame, &f, 30, 2, 9, 255, 'a', 1, &drawn) == PR_FONT201_BOUNDS && drawn == 39);
    CHECK(cell_matches(30, 2, 1, 9) && cell_matches(39, 2, 1, 9) && cell_matches(0, 2, 1, 9) && cell_matches(28, 2, 1, 9));
    CHECK(pr_slot_a_view_covered(&g_frame) == 39 * 64 && g_frame.covered[(2 * 8) * PR_SLOT_A_VIEW_WIDTH + 29 * 8] == 0);
    return 0;
}

static int test_print(void) {
    pr_font201 f; size_t drawn; char big[300];
    CHECK(pr_font201_open(g_block, PR_FONT201_BLOCK_SIZE, &f) == PR_FONT201_OK);
    pr_slot_a_view_clear(&g_frame);
    CHECK(pr_font201_print_ega(&g_frame, &f, 2, 3, 15, "aZ @", 4, &drawn) == PR_FONT201_OK && drawn == 4);
    CHECK(cell_matches(2, 3, 1, 15) && cell_matches(3, 3, 26, 15) && cell_matches(4, 3, 32, 15) && cell_matches(5, 3, 0, 15));
    CHECK(pr_slot_a_view_covered(&g_frame) == 4 * 64);
    /* empty string; printer guard; drawer guard on row 25..39 (accepted by the printer, skipped by the drawer) */
    CHECK(pr_font201_print_ega(&g_frame, &f, 0, 0, 15, "", 0, &drawn) == PR_FONT201_OK && drawn == 0);
    CHECK(pr_font201_print_ega(&g_frame, &f, 40, 0, 15, "A", 1, &drawn) == PR_FONT201_BOUNDS);
    CHECK(pr_font201_print_ega(&g_frame, &f, 0, 40, 15, "A", 1, &drawn) == PR_FONT201_BOUNDS);
    CHECK(pr_font201_print_ega(&g_frame, &f, 0, 30, 15, "AB", 2, &drawn) == PR_FONT201_SKIPPED && drawn == 0);
    /* a string running past column 39: the cells in range are drawn, the rest skipped */
    CHECK(pr_font201_print_ega(&g_frame, &f, 38, 0, 15, "ABCD", 4, &drawn) == PR_FONT201_SKIPPED && drawn == 2);
    CHECK(cell_matches(38, 0, 1, 15) && cell_matches(39, 0, 2, 15));
    /* the 255-character copy limit */
    memset(big, 'A', sizeof big);
    pr_slot_a_view_clear(&g_frame);
    CHECK(pr_font201_print_ega(&g_frame, &f, 0, 0, 15, big, 300, &drawn) == PR_FONT201_SKIPPED && drawn == 40);
    /* byte-column wrap in the printer: 230 characters from column 30: 30..39 drawn (10), 40..255
     * skipped (216), then columns 0..3 drawn again (4); cell 0 holds the 227th character */
    { static char run[230]; memset(run, 'A', sizeof run); run[226] = 'B';
      pr_slot_a_view_clear(&g_frame);
      CHECK(pr_font201_print_ega(&g_frame, &f, 30, 4, 15, run, 230, &drawn) == PR_FONT201_SKIPPED && drawn == 14);
      CHECK(cell_matches(0, 4, 2, 15) && cell_matches(3, 4, 1, 15) && cell_matches(39, 4, 1, 15));
      CHECK(pr_slot_a_view_covered(&g_frame) == 14 * 64); }
    CHECK(pr_font201_print_ega(&g_frame, &f, 0, 0, 15, NULL, 1, &drawn) == PR_FONT201_ARGUMENT);
    CHECK(pr_font201_print_ega(NULL, &f, 0, 0, 15, "A", 1, &drawn) == PR_FONT201_ARGUMENT);
    return 0;
}

int main(void) {
    int rc = 0;
    make_font();
    rc |= test_open();
    rc |= test_glyph_index();
    rc |= test_glyph_row();
    rc |= test_draw_glyph();
    rc |= test_draw_count();
    rc |= test_print();
    if (rc == 0) {
        puts("font 8X8D block 201 tests passed");
    }
    return rc;
}
