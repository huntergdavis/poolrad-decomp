#include "poolrad/recovered/slot_a_post_up_view_flush_page.h"

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

int main(int argc, char **argv) {
    uint8_t *ram = NULL;
    pr_r464_dirty_state dirty = {0};
    pr_slot_a_post_up_view_page page = {0};
    pr_r464_dirty_report report = {0};
    unsigned long parsed, ds;
    unsigned pointer_offset, pointer_segment, plane;
    size_t at, object_size, row;
    int result = 1;

    if (argc != 12) {
        fputs("usage: probe RAM DS_HEX GC0 GC1 GC3 GC5 GC8 PLANE0..3\n",
              stderr);
        return 2;
    }
    if (!parse_hex(argv[2], 0xFFFFu, &parsed) || parsed == 0 ||
        parsed * 16u + 0x5200u > RAM_BYTES)
        return 2;
    ds = parsed;
    if (!parse_hex(argv[3], 0xFFu, &parsed)) return 2;
    dirty.gc_set_reset = (uint8_t)parsed;
    if (!parse_hex(argv[4], 0xFFu, &parsed)) return 2;
    dirty.gc_enable_set_reset = (uint8_t)parsed;
    if (!parse_hex(argv[5], 0xFFu, &parsed)) return 2;
    dirty.gc_rotate = (uint8_t)parsed;
    if (!parse_hex(argv[6], 0xFFu, &parsed)) return 2;
    dirty.gc_mode = (uint8_t)parsed;
    if (!parse_hex(argv[7], 0xFFu, &parsed)) return 2;
    dirty.gc_bit_mask = (uint8_t)parsed;

    ram = malloc(RAM_BYTES);
    if (!ram || !read_exact(argv[1], ram, RAM_BYTES)) goto done;
    dirty.page_base = (uint16_t)word_at(ram, ds * 16u + 0x5060u);
    dirty.vram_origin = 0x03C3u;
    dirty.vram_size = 0x1166u - 0x03C3u;
    page.plane_bytes = PR_SLOT_A_POST_UP_PLANE_BYTES;
    for (plane = 0; plane < 4; ++plane) {
        dirty.map_mask[plane] = ram[ds * 16u + 0x25F6u + plane];
        page.plane[plane] = malloc(page.plane_bytes);
        if (!page.plane[plane] ||
            !read_exact(argv[8 + plane], page.plane[plane], page.plane_bytes))
            goto done;
    }
    for (row = 0; row < PR_R464_DIRTY_ROWS; ++row) {
        pr_r464_dirty_row *item = &dirty.row[row];
        item->dirty = ram[ds * 16u + 0x4A28u + row];
        item->first = (uint16_t)word_at(ram, ds * 16u + 0x4C20u + 2u * row);
        item->last = (uint16_t)word_at(ram, ds * 16u + 0x4D70u + 2u * row);
        item->low = (uint16_t)word_at(ram, ds * 16u + 0x4AD0u + 2u * row);
        item->stage_base = (uint16_t)word_at(
            ram, ds * 16u + 0x5050u + 2u * (row + 8u));
    }
    pointer_offset = word_at(ram, ds * 16u + 0x4A20u);
    pointer_segment = word_at(ram, ds * 16u + 0x4A22u);
    at = (size_t)pointer_segment * 16u + pointer_offset;
    if (at >= RAM_BYTES) goto done;
    object_size = RAM_BYTES - at;
    if (object_size > 0x10000u - pointer_offset)
        object_size = 0x10000u - pointer_offset;
    if (pr_slot_a_post_up_view_flush_page(&dirty, &page, ram + at,
                                          object_size, &report) !=
        PR_SLOT_A_POST_UP_FLUSH_OK)
        goto done;
    for (plane = 0; plane < 4; ++plane)
        if (fwrite(page.plane[plane], 1, page.plane_bytes, stdout) !=
            page.plane_bytes)
            goto done;
    for (row = 0; row < PR_R464_DIRTY_ROWS; ++row) {
        unsigned first = dirty.row[row].first, last = dirty.row[row].last;
        uint8_t metadata[5] = {
            dirty.row[row].dirty,
            (uint8_t)first, (uint8_t)(first >> 8),
            (uint8_t)last, (uint8_t)(last >> 8),
        };
        if (fwrite(metadata, 1, sizeof metadata, stdout) != sizeof metadata)
            goto done;
    }
    result = 0;
done:
    for (plane = 0; plane < 4; ++plane) free(page.plane[plane]);
    free(ram);
    return result;
}
