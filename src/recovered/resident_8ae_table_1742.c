#include "poolrad/recovered/resident_8ae_table_1742.h"

static uint16_t read_word(const uint8_t *segment, uint16_t offset)
{
    return (uint16_t)(segment[offset] |
                      ((uint16_t)segment[(uint16_t)(offset + 1u)] << 8));
}

static void write_word(uint8_t *segment, uint16_t offset, uint16_t word)
{
    segment[offset] = (uint8_t)word;
    segment[(uint16_t)(offset + 1u)] = (uint8_t)(word >> 8);
}

pr_rec_8ae_table_status pr_rec_res_08ae_1742(uint8_t *segment,
                                               size_t segment_size,
                                               uint16_t word_argument)
{
    uint16_t source_offset;
    uint16_t record_offset;
    uint8_t row;
    if (segment == 0 || segment_size < PR_REC_8AE_SEGMENT_BYTES)
        return PR_REC_8AE_TABLE_ARGUMENT;

    /* +1752..+1766: write the full argument, form 16-bit SI by three
     * doublings, and enter a four-row update. */
    write_word(segment, 0x01ffu, word_argument);
    source_offset = (uint16_t)((uint32_t)word_argument * 8u);
    segment[0x01d8u] = 1u;
    segment[0x01d9u] = 4u;
    record_offset = 0x022du;

    for (row = 0; row < 4u; ++row) {
        uint16_t value = read_word(
            segment, (uint16_t)(source_offset + 0x055fu));
        if (value != 0u) {
            uint16_t displacement = 0x002eu;
            /* +1775..+177C clears 24 WORDs from high to low.
             * Later source rows observe prior writes when they alias. */
            for (;;) {
                write_word(segment,
                           (uint16_t)(record_offset + displacement), 0u);
                if (displacement == 0u) break;
                displacement = (uint16_t)(displacement - 2u);
            }
            write_word(segment, (uint16_t)(record_offset + 2u), value);
            segment[0x0209u + row] = (uint8_t)word_argument;
            write_word(segment, record_offset, 1u);
        }
        record_offset = (uint16_t)(record_offset + 0x0030u);
        source_offset = (uint16_t)(source_offset + 2u);
        --segment[0x01d9u];
    }
    segment[0x01d8u] = 0u;
    return PR_REC_8AE_TABLE_OK;
}
