#include "poolrad/formats/ega.h"

#include <stdio.h>

#define EGA_ROW_BYTES 40u
#define EGA_ROW_PIXELS (EGA_ROW_BYTES * 8u)

int main(int argc, char **argv) {
    uint8_t rows[4][EGA_ROW_BYTES], indices[EGA_ROW_PIXELS];
    size_t plane, column;
    if (argc != 5) {
        fputs("usage: ega_planar_row_probe PLANE0 PLANE1 PLANE2 PLANE3\n", stderr);
        return 2;
    }
    for (plane = 0; plane < 4; ++plane) {
        FILE *input = fopen(argv[plane + 1], "rb");
        size_t count;
        int extra, error, closed;
        if (!input) return 1;
        count = fread(rows[plane], 1, EGA_ROW_BYTES, input);
        extra = fgetc(input);
        error = ferror(input);
        closed = fclose(input);
        if (count != EGA_ROW_BYTES || extra != EOF || error || closed) return 1;
    }
    for (column = 0; column < EGA_ROW_BYTES; ++column) {
        uint8_t planes[4];
        for (plane = 0; plane < 4; ++plane) planes[plane] = rows[plane][column];
        if (pr_ega_planar_byte(planes, indices + column * 8u) != PR_EGA_OK)
            return 1;
    }
    if (fwrite(indices, 1, sizeof indices, stdout) != sizeof indices)
        return 1;
    return 0;
}
