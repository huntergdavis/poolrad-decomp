#include "poolrad/recovered/slot_a_glyph_plane_byte.h"

#include <stdio.h>
#include <stdlib.h>

static int number(const char *text, unsigned *value) {
    char *end;
    unsigned long parsed = strtoul(text, &end, 0);
    if (text == end || *end != '\0' || parsed > 255ul) {
        return 0;
    }
    *value = (unsigned)parsed;
    return 1;
}

int main(int argc, char **argv) {
    uint8_t block[PR_FONT201_BLOCK_SIZE], byte;
    pr_font201 font;
    unsigned ch, upcase, row, color, plane;
    FILE *input;
    if (argc != 6 || !number(argv[2], &ch) || !number(argv[3], &upcase) ||
        !number(argv[4], &row) || !number(argv[5], &color) || upcase > 1u) {
        fprintf(stderr, "usage: probe FONT ch upcase row color\n");
        return 2;
    }
    input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("font");
        return 2;
    }
    if (fread(block, 1u, sizeof block, input) != sizeof block ||
        fgetc(input) != EOF || ferror(input) != 0 || fclose(input) != 0 ||
        pr_font201_open(block, sizeof block, &font) != PR_FONT201_OK) {
        fprintf(stderr, "font must contain exactly 1416 bytes\n");
        return 2;
    }
    for (plane = 0u; plane < 4u; ++plane) {
        if (pr_slot_a_glyph_plane_byte(&font, (uint8_t)ch, (int)upcase,
                                       (uint8_t)row, (uint8_t)color,
                                       plane, &byte) != PR_FONT201_OK) {
            fprintf(stderr, "glyph byte unavailable\n");
            return 2;
        }
        printf("%02x%s", byte, plane == 3u ? "\n" : " ");
    }
    return 0;
}
