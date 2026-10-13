#include "poolrad/recovered/resident_464_dirty_transfer.h"

#include <stdio.h>
#include <stdlib.h>

#define RAM_BYTES 0xA0000u

static unsigned word_at(const uint8_t *ram, size_t at) {
    return (unsigned)ram[at] | ((unsigned)ram[at + 1u] << 8);
}

static int parse_hex(const char *text, unsigned long limit,
                     unsigned long *value) {
    char *end;
    unsigned long parsed = strtoul(text, &end, 16);
    if (*text == '\0' || *end != '\0' || parsed > limit) return 0;
    *value = parsed;
    return 1;
}

static int read_exact(const char *path, uint8_t *out, size_t size) {
    FILE *file = fopen(path, "rb");
    size_t read;
    int extra, error, closed;
    if (!file) return 0;
    read = fread(out, 1, size, file);
    extra = fgetc(file);
    error = ferror(file);
    closed = fclose(file);
    return read == size && extra == EOF && !error && !closed;
}

static void release_planes(pr_r464_dirty_state *state) {
    unsigned plane;
    for (plane = 0; plane < 4; ++plane) {
        free(state->staging[plane]);
        free(state->page[plane]);
    }
}

int main(int argc, char **argv) {
    uint8_t *ram;
    pr_r464_dirty_state state = {0};
    pr_r464_dirty_report report = {0};
    unsigned long parsed, ds;
    unsigned ptr_offset, ptr_segment, plane;
    size_t ptr_at, object_size, row, origin = 0x10000u, end = 0;
    if (argc != 16) {
        fputs("usage: all_dirty_probe PRE_RAM DS_HEX GC0 GC1 GC3 GC5 GC8 STAGE0..3 PAGE0..3\n",
              stderr);
        return 2;
    }
    if (!parse_hex(argv[2], 0xFFFFu, &parsed) || parsed == 0 ||
        parsed * 16u + 0x5200u > RAM_BYTES)
        return 2;
    ds = parsed;
    if (!parse_hex(argv[3], 0xFFu, &parsed)) return 2;
    state.gc_set_reset = (uint8_t)parsed;
    if (!parse_hex(argv[4], 0xFFu, &parsed)) return 2;
    state.gc_enable_set_reset = (uint8_t)parsed;
    if (!parse_hex(argv[5], 0xFFu, &parsed)) return 2;
    state.gc_rotate = (uint8_t)parsed;
    if (!parse_hex(argv[6], 0xFFu, &parsed)) return 2;
    state.gc_mode = (uint8_t)parsed;
    if (!parse_hex(argv[7], 0xFFu, &parsed)) return 2;
    state.gc_bit_mask = (uint8_t)parsed;
    ram = malloc(RAM_BYTES);
    if (!ram) return 1;
    if (!read_exact(argv[1], ram, RAM_BYTES)) {
        free(ram);
        return 1;
    }
    state.page_base = (uint16_t)word_at(ram, ds * 16u + 0x5060u);
    for (plane = 0; plane < 4; ++plane)
        state.map_mask[plane] = ram[ds * 16u + 0x25F6u + plane];
    for (row = 0; row < PR_R464_DIRTY_ROWS; ++row) {
        pr_r464_dirty_row *item = &state.row[row];
        size_t stage_at, page_at, count;
        item->dirty = ram[ds * 16u + 0x4A28u + row];
        item->first =
            (uint16_t)word_at(ram, ds * 16u + 0x4C20u + 2u * row);
        item->last =
            (uint16_t)word_at(ram, ds * 16u + 0x4D70u + 2u * row);
        item->low =
            (uint16_t)word_at(ram, ds * 16u + 0x4AD0u + 2u * row);
        item->stage_base = (uint16_t)word_at(
            ram, ds * 16u + 0x5050u + 2u * (row + 8u));
        if (!item->dirty) continue;
        if (item->last < item->first) {
            free(ram);
            return 1;
        }
        count = ((size_t)item->last - item->first) / 4u + 1u;
        stage_at = (size_t)item->stage_base + item->low + 1u;
        page_at = (size_t)state.page_base + 1u + row * 40u + item->low;
        if (stage_at < origin) origin = stage_at;
        if (page_at < origin) origin = page_at;
        if (stage_at + count > end) end = stage_at + count;
        if (page_at + count > end) end = page_at + count;
    }
    if (origin >= end || end > 0x10000u) {
        free(ram);
        return 1;
    }
    state.vram_origin = (uint16_t)origin;
    state.vram_size = end - origin;
    for (plane = 0; plane < 4; ++plane) {
        state.staging[plane] = malloc(state.vram_size);
        state.page[plane] = malloc(state.vram_size);
        if (!state.staging[plane] || !state.page[plane] ||
            !read_exact(argv[8 + plane], state.staging[plane],
                        state.vram_size) ||
            !read_exact(argv[12 + plane], state.page[plane],
                        state.vram_size)) {
            release_planes(&state);
            free(ram);
            return 1;
        }
    }
    ptr_offset = word_at(ram, ds * 16u + 0x4A20u);
    ptr_segment = word_at(ram, ds * 16u + 0x4A22u);
    ptr_at = (size_t)ptr_segment * 16u + ptr_offset;
    if (ptr_at >= RAM_BYTES) {
        release_planes(&state);
        free(ram);
        return 1;
    }
    object_size = RAM_BYTES - ptr_at;
    if (object_size > 0x10000u - ptr_offset)
        object_size = 0x10000u - ptr_offset;
    if (pr_r464_dirty_transfer(&state, ram + ptr_at, object_size,
                               &report) != PR_R464_DIRTY_OK) {
        release_planes(&state);
        free(ram);
        return 1;
    }
    for (plane = 0; plane < 4; ++plane)
        if (fwrite(state.staging[plane], 1, state.vram_size, stdout)
            != state.vram_size) {
            release_planes(&state);
            free(ram);
            return 1;
        }
    for (plane = 0; plane < 4; ++plane)
        if (fwrite(state.page[plane], 1, state.vram_size, stdout)
            != state.vram_size) {
            release_planes(&state);
            free(ram);
            return 1;
        }
    for (row = 0; row < PR_R464_DIRTY_ROWS; ++row) {
        unsigned first = state.row[row].first;
        unsigned last = state.row[row].last;
        uint8_t metadata[5] = {
            state.row[row].dirty,
            (uint8_t)first, (uint8_t)(first >> 8),
            (uint8_t)last, (uint8_t)(last >> 8),
        };
        if (fwrite(metadata, 1, sizeof metadata, stdout) !=
            sizeof metadata) {
            release_planes(&state);
            free(ram);
            return 1;
        }
    }
    release_planes(&state);
    free(ram);
    return 0;
}
