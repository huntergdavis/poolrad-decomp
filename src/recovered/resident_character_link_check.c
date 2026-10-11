#include "poolrad/recovered/resident_character_link_check.h"

#include <stddef.h>

static int read_link(const pr_rec_character_link_check_io *io,
                     pr_rec_far_ptr character, pr_rec_far_ptr *link) {
    uint16_t low, high;
    if (!io->read_far_word(io->context, character.segment,
                           (uint16_t)(character.offset + 0xCCu), &low) ||
        !io->read_far_word(io->context, character.segment,
                           (uint16_t)(character.offset + 0xCEu), &high)) {
        return 0;
    }
    link->offset = low;
    link->segment = high;
    return 1;
}

pr_rec_character_link_check_status pr_rec_res_00ba_2e03(
    const pr_rec_character_link_check_io *io, pr_rec_far_ptr character,
    uint8_t *result) {
    pr_rec_far_ptr link;
    uint8_t raw_index, table_value;
    uint16_t table_offset;

    if (io == NULL || io->read_far_word == NULL ||
        io->read_far_byte == NULL || io->read_global_byte == NULL ||
        result == NULL) {
        return PR_REC_CHARACTER_LINK_CHECK_ARGUMENT;
    }

    /* 0x39A9..0x39B6: both words are read before the all-zero test. */
    if (!read_link(io, character, &link)) {
        return PR_REC_CHARACTER_LINK_CHECK_MEMORY;
    }
    if ((link.offset | link.segment) == 0u) {
        *result = 0u;
        return PR_REC_CHARACTER_LINK_CHECK_OK;
    }

    /* 0x39B8..0x39D1: LES reads both character words again. The linked
     * byte is zero-extended, shifted left four, and indexed from DS 0x558E. */
    if (!read_link(io, character, &link) ||
        !io->read_far_byte(io->context, link.segment,
                           (uint16_t)(link.offset + 0x2Eu), &raw_index)) {
        return PR_REC_CHARACTER_LINK_CHECK_MEMORY;
    }
    table_offset = (uint16_t)(0x558Eu + ((uint16_t)raw_index << 4));
    if (!io->read_global_byte(io->context, table_offset, &table_value)) {
        return PR_REC_CHARACTER_LINK_CHECK_MEMORY;
    }
    *result = (uint8_t)(table_value > 1u);
    return PR_REC_CHARACTER_LINK_CHECK_OK;
}
