#include "poolrad/recovered/slot_a_printer_page.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

enum { PAGE_BYTES = 8000, TEXT_CAP = 255,
       INPUT_BYTES = PR_FONT201_BLOCK_SIZE + 4 + TEXT_CAP + 4 * PAGE_BYTES };

static uint8_t input[INPUT_BYTES];
static uint8_t plane[4][PAGE_BYTES];
static uint8_t native[4][PAGE_BYTES];

static int read_exact(const char *path, uint8_t *dst, size_t size) {
    FILE *f = fopen(path, "rb");
    int ok;
    if (f == NULL) return 0;
    ok = fread(dst, 1u, size, f) == size && fgetc(f) == EOF && !ferror(f);
    if (fclose(f) != 0) ok = 0;
    return ok;
}

static int synthetic(void) {
    pr_font201 font;
    pr_slot_a_printer_page_report report;
    uint8_t *page[4] = {plane[0], plane[1], plane[2], plane[3]};
    uint8_t glyphs[PR_FONT201_BLOCK_SIZE] = {0};
    char text[19];
    size_t row, cell, at;
    unsigned p;

    memset(text, 'A', sizeof text);
    for (row = 0; row < 8u; ++row) glyphs[8u + row] = 0xAAu;
    CHECK(pr_font201_open(glyphs, sizeof glyphs, &font) == PR_FONT201_OK);
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
        17u, 15u, 10u, text, sizeof text, &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 19u && report.plane_stores == 608u &&
          report.changed_plane_bytes == 304u);
    for (row = 0; row < 8u; ++row) {
        for (cell = 0; cell < 19u; ++cell) {
            at = (15u * 8u + row) * 40u + 17u + cell;
            for (p = 0u; p < 4u; ++p)
                CHECK(plane[p][at] == ((p == 1u || p == 3u) ? 0xAAu : 0u));
        }
    }
    at = 15u * 8u * 40u + 16u;
    for (p = 0u; p < 4u; ++p) {
        CHECK(plane[p][at] == 0u);
        CHECK(plane[p][at + 20u] == 0u);
    }
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
        17u, 15u, 10u, text, sizeof text, &report) == PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.changed_plane_bytes == 0u);
    return 0;
}

static int private_page(const char *input_path, const char *run_dir) {
    const size_t meta = PR_FONT201_BLOCK_SIZE;
    const size_t text_at = meta + 4u;
    const size_t pages_at = text_at + TEXT_CAP;
    pr_font201 font;
    pr_slot_a_printer_page_report report;
    uint8_t *page[4] = {plane[0], plane[1], plane[2], plane[3]};
    char path[1024];
    unsigned p;

    CHECK(read_exact(input_path, input, sizeof input));
    CHECK(input[meta] == 19u && input[meta + 1u] == 17u &&
          input[meta + 2u] == 15u && input[meta + 3u] == 10u);
    CHECK(pr_font201_open(input, PR_FONT201_BLOCK_SIZE, &font) == PR_FONT201_OK);
    for (p = 0u; p < 4u; ++p) {
        memcpy(plane[p], input + pages_at + (size_t)p * PAGE_BYTES, PAGE_BYTES);
        CHECK(snprintf(path, sizeof path, "%s/first-printer-entry-plane%u-private.bin",
                       run_dir, p) > 0);
        CHECK(read_exact(path, native[p], PAGE_BYTES));
        CHECK(memcmp(plane[p], native[p], PAGE_BYTES) == 0);
        CHECK(snprintf(path, sizeof path, "%s/first-printer-return-plane%u-private.bin",
                       run_dir, p) > 0);
        CHECK(read_exact(path, native[p], PAGE_BYTES));
    }
    CHECK(pr_slot_a_printer_page_apply(page, 0u, PAGE_BYTES, &font,
        input[meta + 1u], input[meta + 2u], input[meta + 3u],
        (const char *)(input + text_at), input[meta], &report) ==
          PR_SLOT_A_PRINTER_PAGE_OK);
    CHECK(report.cells == 19u && report.plane_stores == 608u &&
          report.changed_plane_bytes == 210u);
    for (p = 0u; p < 4u; ++p) CHECK(memcmp(plane[p], native[p], PAGE_BYTES) == 0);
    puts("post-Up first printer: 32000/32000 page bytes equal");
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) return synthetic();
    if (argc == 3) return private_page(argv[1], argv[2]);
    fprintf(stderr, "usage: fixture [PRIVATE_PROBE_INPUT PRIVATE_RUN_DIR]\n");
    return 2;
}
