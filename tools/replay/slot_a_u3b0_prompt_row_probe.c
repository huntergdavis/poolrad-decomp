#include "poolrad/recovered/slot_a_u3b0_prompt_row.h"

#include <stdio.h>
#include <string.h>

enum {
    SET_BYTES = 32,
    HEADER_BYTES = 6,
    TEXT_CAP = 255,
    PLANE_BYTES = 8000,
    FONT_AT = 0,
    SET_AT = FONT_AT + PR_FONT201_BLOCK_SIZE,
    HEADER_AT = SET_AT + SET_BYTES,
    TEXT_AT = HEADER_AT + HEADER_BYTES,
    PAGE_AT = TEXT_AT + TEXT_CAP,
    INPUT_BYTES = PAGE_AT + 4 * PLANE_BYTES
};

static uint8_t input[INPUT_BYTES];
static uint8_t planes[4][PLANE_BYTES];

int main(int argc, char **argv) {
    FILE *source, *output;
    pr_font201 font;
    pr_slot_a_printer_page_report report;
    pr_slot_a_u3b0_row_input args;
    uint8_t *page[4] = {planes[0], planes[1], planes[2], planes[3]};
    size_t p;
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
        fprintf(stderr, "invalid input size\n");
        return 2;
    }
    if (fclose(source) != 0 ||
        pr_font201_open(input + FONT_AT, PR_FONT201_BLOCK_SIZE, &font) !=
            PR_FONT201_OK) {
        fprintf(stderr, "invalid font\n");
        return 2;
    }
    for (p = 0u; p < 4u; ++p) {
        memcpy(planes[p], input + PAGE_AT + p * PLANE_BYTES, PLANE_BYTES);
    }
    args.first_column = input[HEADER_AT];
    args.length = input[HEADER_AT + 1u];
    args.characters = input + TEXT_AT;
    args.special_set = input + SET_AT;
    args.selected_first = input[HEADER_AT + 2u];
    args.selected_last = input[HEADER_AT + 3u];
    args.selected_color = input[HEADER_AT + 4u];
    args.ordinary_color = input[HEADER_AT + 5u];
    if (pr_slot_a_u3b0_prompt_row_apply(page, &font, &args, &report) !=
        PR_SLOT_A_U3B0_ROW_OK) {
        fprintf(stderr, "prompt row model rejected private input\n");
        return 2;
    }
    output = fopen(argv[2], "wb");
    if (output == NULL) {
        perror("output");
        return 2;
    }
    for (p = 0u; p < 4u; ++p) {
        if (fwrite(planes[p], 1u, PLANE_BYTES, output) != PLANE_BYTES) {
            fclose(output);
            fprintf(stderr, "output write failed\n");
            return 2;
        }
    }
    if (fclose(output) != 0) {
        fprintf(stderr, "output close failed\n");
        return 2;
    }
    printf("cells=%zu stores=%zu changed=%zu\n", report.cells,
           report.plane_stores, report.changed_plane_bytes);
    return 0;
}
