#include "poolrad/recovered/resident_464_row56_transfer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_BYTES 0xA0000u

static unsigned word_at(const uint8_t *ram, size_t at) {
    return (unsigned)ram[at] | ((unsigned)ram[at + 1u] << 8);
}

static int parse_byte(const char *text, uint8_t *value) {
    char *end;
    unsigned long parsed = strtoul(text, &end, 16);
    if (*text == '\0' || *end != '\0' || parsed > 0xFFu) return 0;
    *value = (uint8_t)parsed;
    return 1;
}

static int read_exact(const char *path, uint8_t *out, size_t size) {
    FILE *file = fopen(path, "rb");
    int extra, error, closed;
    size_t read;
    if (!file) return 0;
    read = fread(out, 1, size, file);
    extra = fgetc(file);
    error = ferror(file);
    closed = fclose(file);
    return read == size && extra == EOF && !error && !closed;
}

int main(int argc, char **argv) {
    uint8_t *ram;
    pr_r464_row56_state state = {0};
    unsigned ds, ptr_offset, ptr_segment, plane;
    unsigned long parsed_ds;
    size_t ptr_at, object_size;
    uint16_t output_offset = 0;
    char *end;
    if (argc != 16) {
        fputs("usage: row56_probe PRE_RAM DS_HEX GC0 GC1 GC3 GC5 GC8 STAGE0..3 PAGE0..3\n",
              stderr);
        return 2;
    }
    parsed_ds = strtoul(argv[2], &end, 16);
    if (*argv[2] == '\0' || *end != '\0' || parsed_ds > 0xFFFFu ||
        parsed_ds == 0 ||
        parsed_ds * 16u + 0x5062u > RAM_BYTES)
        return 2;
    ds = (unsigned)parsed_ds;
    if (!parse_byte(argv[3], &state.gc_set_reset) ||
        !parse_byte(argv[4], &state.gc_enable_set_reset) ||
        !parse_byte(argv[5], &state.gc_rotate) ||
        !parse_byte(argv[6], &state.gc_mode) ||
        !parse_byte(argv[7], &state.gc_bit_mask))
        return 2;
    ram = malloc(RAM_BYTES);
    if (!ram) return 1;
    if (!read_exact(argv[1], ram, RAM_BYTES)) {
        free(ram);
        return 1;
    }
    state.dirty = ram[(size_t)ds * 16u + 0x4A28u + 56u];
    state.first = (uint16_t)word_at(ram, (size_t)ds * 16u + 0x4C20u + 112u);
    state.last = (uint16_t)word_at(ram, (size_t)ds * 16u + 0x4D70u + 112u);
    state.row_base = (uint16_t)word_at(ram, (size_t)ds * 16u + 0x5050u + 128u);
    state.low = (uint16_t)word_at(ram, (size_t)ds * 16u + 0x4AD0u + 112u);
    state.page_base = (uint16_t)word_at(ram, (size_t)ds * 16u + 0x5060u);
    for (plane = 0; plane < 4; ++plane) {
        state.map_mask[plane] = ram[(size_t)ds * 16u + 0x25F6u + plane];
        if (!read_exact(argv[8 + plane], state.staging.plane[plane],
                        PR_R464_ROW56_BYTES) ||
            !read_exact(argv[12 + plane], state.page.plane[plane],
                        PR_R464_ROW56_BYTES)) {
            free(ram);
            return 1;
        }
    }
    ptr_offset = word_at(ram, (size_t)ds * 16u + 0x4A20u);
    ptr_segment = word_at(ram, (size_t)ds * 16u + 0x4A22u);
    ptr_at = (size_t)ptr_segment * 16u + ptr_offset;
    if (ptr_at >= RAM_BYTES) {
        free(ram);
        return 1;
    }
    object_size = RAM_BYTES - ptr_at;
    if (object_size > 0x10000u - ptr_offset)
        object_size = 0x10000u - ptr_offset;
    if (pr_r464_row56_transfer(&state, ram + ptr_at, object_size,
                               &output_offset) != PR_R464_ROW56_OK ||
        output_offset != 0x0A03u ||
        fwrite(state.staging.plane, 1, sizeof state.staging.plane, stdout)
            != sizeof state.staging.plane ||
        fwrite(state.page.plane, 1, sizeof state.page.plane, stdout)
            != sizeof state.page.plane) {
        free(ram);
        return 1;
    }
    free(ram);
    return 0;
}
