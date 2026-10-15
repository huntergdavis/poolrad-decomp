#include "poolrad/recovered/resident_464_2632_post_up_43row.h"

enum {
    DS_SEGMENT = 0x14A0u,
    STACK_SEGMENT = 0x1D16u,
    OBJECT_SEGMENT = 0x3DE4u,
    OBJECT_OFFSET = 0x0008u,
    SOURCE_OFFSET = 0x260Au,
    OBJECT_BYTES = 14135u,
    OBJECT_HEADER = 23u,
    DIRTY_OFFSET = 0x4A28u,
    LEFT_OFFSET = 0x4AD0u,
    START_OFFSET = 0x4C20u,
    END_OFFSET = 0x4D70u,
    MASK_TABLE_OFFSET = 0x25F6u,
    LOOP_COUNTER_OFFSET = 0x70C7u
};

static size_t physical(uint16_t segment, uint16_t offset)
{
    return ((size_t)segment << 4) + offset;
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[at + 1u] << 8));
}

static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8);
}

pr_r464_2632_post_up_status pr_r464_2632_post_up_43row(
    uint8_t *ram, size_t ram_size, const pr_r464_2632_post_up_frame *frame,
    pr_r464_2632_post_up_result *result)
{
    size_t stack, ds, object, source;
    uint16_t object_off, object_seg, source_off, source_seg;
    uint16_t mask, rows, count, first_row, x, width, height;
    uint8_t planes[4];
    size_t changed = 0u;
    unsigned row, column, plane;

    if (ram == NULL || frame == NULL || result == NULL)
        return PR_R464_2632_POST_UP_ARGUMENT;
    if (ram_size < PR_R464_2632_POST_UP_RAM_BYTES ||
        frame->cs != 0x2D14u || frame->ip != 0x0198u ||
        frame->ss != STACK_SEGMENT || frame->sp != 0x3EB0u ||
        frame->ds != DS_SEGMENT)
        return PR_R464_2632_POST_UP_BOUNDARY;

    stack = physical(frame->ss, frame->sp);
    ds = physical(frame->ds, 0u);
    if (stack + 18u > ram_size || ds + LOOP_COUNTER_OFFSET >= ram_size ||
        ds + END_OFFSET + 336u > ram_size)
        return PR_R464_2632_POST_UP_BOUNDARY;

    object_off = word_at(ram, stack);
    object_seg = word_at(ram, stack + 2u);
    source_off = word_at(ram, stack + 4u);
    source_seg = word_at(ram, stack + 6u);
    mask = word_at(ram, stack + 8u);
    rows = word_at(ram, stack + 10u);
    count = word_at(ram, stack + 12u);
    first_row = word_at(ram, stack + 14u);
    x = word_at(ram, stack + 16u);
    object = physical(object_seg, object_off);
    source = physical(source_seg, source_off);
    if (object_seg != OBJECT_SEGMENT || object_off != OBJECT_OFFSET ||
        source_seg != DS_SEGMENT || source_off != SOURCE_OFFSET ||
        mask != 0x000Bu || rows != 43u || count != 11u ||
        first_row != 16u || x != 2u ||
        object + OBJECT_BYTES > ram_size || source + 8u > ram_size ||
        ram[ds + 0x4A1Eu] != 1u)
        return PR_R464_2632_POST_UP_BOUNDARY;

    height = word_at(ram, object);
    width = word_at(ram, object + 2u);
    if (height != 168u || width != 21u ||
        (unsigned)first_row + rows > height ||
        (unsigned)x + count > width ||
        (size_t)object_off + OBJECT_HEADER +
            (((size_t)first_row + rows - 1u) * width + x + count) * 4u >
            0x10000u)
        return PR_R464_2632_POST_UP_BOUNDARY;

    /* +2910..+294D: FillChar zeros the four-byte local; each table/mask
     * test sets its selected byte to FF. The copied source local is unused
     * by this reached mode-one successor. */
    for (plane = 0u; plane < 4u; ++plane)
        planes[plane] = (ram[ds + MASK_TABLE_OFFSET + plane] & (uint8_t)mask)
                            ? 0xFFu : 0u;

    /* +294D..+2A07: the reached no-clipping path processes 43 rows. */
    for (row = first_row; row < (unsigned)first_row + rows; ++row) {
        size_t extent = ds + 2u * row;
        uint16_t start = (uint16_t)((row * width + x) * 4u);
        uint16_t end = (uint16_t)(start + count * 4u - 1u);

        ram[ds + DIRTY_OFFSET + row] = 1u;
        if (word_at(ram, extent + START_OFFSET) > start) {
            put_word(ram, extent + LEFT_OFFSET, x);
            put_word(ram, extent + START_OFFSET, start);
        }
        for (column = 0u; column < count; ++column) {
            size_t target = object + OBJECT_HEADER + start + 4u * column;
            for (plane = 0u; plane < 4u; ++plane) {
                if (ram[target + plane] != planes[plane]) ++changed;
                ram[target + plane] = planes[plane];
            }
            ram[ds + LOOP_COUNTER_OFFSET] = (uint8_t)(column + 1u);
        }
        if (end > word_at(ram, extent + END_OFFSET))
            put_word(ram, extent + END_OFFSET, end);
    }

    result->first_row = first_row;
    result->last_row = (uint16_t)(first_row + rows - 1u);
    result->offscreen_value_changes = changed;
    return PR_R464_2632_POST_UP_OK;
}
