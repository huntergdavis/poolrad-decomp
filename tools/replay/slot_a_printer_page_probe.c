#include "poolrad/recovered/slot_a_printer_page.h"

#include <stdio.h>
#include <string.h>

#ifndef PR_PAGE_BASE
#define PR_PAGE_BASE 0x3C0
#endif
#ifndef PR_PAGE_BYTES
#define PR_PAGE_BYTES 3520
#endif

enum {
    TEXT_CAP = 255,
    PAGE_BASE = PR_PAGE_BASE,
    PAGE_BYTES = PR_PAGE_BYTES,
    INPUT_BYTES = PR_FONT201_BLOCK_SIZE + 4 + TEXT_CAP + 4 * PAGE_BYTES
};

static uint8_t input[INPUT_BYTES];
static uint8_t storage[4][PAGE_BYTES];

int main(int argc, char **argv) {
    pr_font201 font;
    pr_slot_a_printer_page_report report;
    uint8_t *page[4] = {storage[0], storage[1], storage[2], storage[3]};
    const size_t meta = PR_FONT201_BLOCK_SIZE;
    const size_t text_at = meta + 4u;
    const size_t planes_at = text_at + TEXT_CAP;
    unsigned plane;
    FILE *source, *output;

    if (argc != 3) {
        fprintf(stderr, "usage: probe PRIVATE_INPUT PRIVATE_OUTPUT\n");
        return 2;
    }
    source = fopen(argv[1], "rb");
    if (source == NULL) {
        perror("input");
        return 2;
    }
    if (fread(input, 1u, sizeof input, source) != sizeof input ||
        fgetc(source) != EOF || ferror(source) != 0) {
        fclose(source);
        fprintf(stderr, "invalid private input size\n");
        return 2;
    }
    if (fclose(source) != 0 ||
        pr_font201_open(input, PR_FONT201_BLOCK_SIZE, &font) != PR_FONT201_OK) {
        fprintf(stderr, "invalid private font\n");
        return 2;
    }
    for (plane = 0u; plane < 4u; ++plane) {
        memcpy(storage[plane], input + planes_at + (size_t)plane * PAGE_BYTES,
               PAGE_BYTES);
    }
    if (pr_slot_a_printer_page_apply(
            page, PAGE_BASE, PAGE_BYTES, &font,
            input[meta + 1u], input[meta + 2u], input[meta + 3u],
            (const char *)(input + text_at), input[meta], &report) !=
        PR_SLOT_A_PRINTER_PAGE_OK) {
        fprintf(stderr, "printer page model rejected private input\n");
        return 2;
    }
    output = fopen(argv[2], "wb");
    if (output == NULL) {
        perror("output");
        return 2;
    }
    for (plane = 0u; plane < 4u; ++plane) {
        if (fwrite(storage[plane], 1u, PAGE_BYTES, output) != PAGE_BYTES) {
            fclose(output);
            fprintf(stderr, "output write failed\n");
            return 2;
        }
    }
    if (fclose(output) != 0) {
        fprintf(stderr, "output close failed\n");
        return 2;
    }
    printf("cells=%zu stores=%zu changed=%zu\n",
           report.cells, report.plane_stores, report.changed_plane_bytes);
    return 0;
}
