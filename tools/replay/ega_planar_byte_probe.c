#include "poolrad/formats/ega.h"

#include <stdio.h>

int main(int argc, char **argv) {
    uint8_t planes[4], indices[8];
    int i;
    if (argc != 5) {
        fputs("usage: ega_planar_byte_probe PLANE0 PLANE1 PLANE2 PLANE3\n", stderr);
        return 2;
    }
    for (i = 0; i < 4; ++i) {
        FILE *input = fopen(argv[i + 1], "rb");
        int byte, extra;
        if (!input) return 1;
        byte = fgetc(input);
        extra = fgetc(input);
        if (fclose(input) || byte == EOF || extra != EOF) return 1;
        planes[i] = (uint8_t)byte;
    }
    if (pr_ega_planar_byte(planes, indices) != PR_EGA_OK ||
        fwrite(indices, 1, sizeof indices, stdout) != sizeof indices)
        return 1;
    return 0;
}
