#include "poolrad/recovered/resident_character_link_flags.h"

#include <stddef.h>

pr_rec_character_link_check_status pr_rec_res_00ba_2e46(
    const pr_rec_character_link_check_io *io, pr_rec_far_ptr character,
    uint8_t *result) {
    pr_rec_character_link_check_status status;
    pr_rec_far_ptr link;
    uint16_t low, high, table_offset;
    uint8_t initial, raw_index, table_value;

    if (result == NULL) {
        return PR_REC_CHARACTER_LINK_CHECK_ARGUMENT;
    }

    /* 0x39EC..0x39F8: the only call is the same-segment predicate at
     * 0x39A3. Its complete read path and AL result are translated there. */
    status = pr_rec_res_00ba_2e03(io, character, &initial);
    if (status != PR_REC_CHARACTER_LINK_CHECK_OK) {
        return status;
    }
    if (initial == 0u) {
        *result = 0u;
        return PR_REC_CHARACTER_LINK_CHECK_OK;
    }

    /* 0x39FA..0x3A16: LES reads the character's +0xCC/+0xCE pointer
     * again after the callee returns. The linked byte selects a 16-byte
     * DS table entry, whose +2 byte is masked by 0x14. */
    if (!io->read_far_word(io->context, character.segment,
                           (uint16_t)(character.offset + 0xCCu), &low) ||
        !io->read_far_word(io->context, character.segment,
                           (uint16_t)(character.offset + 0xCEu), &high)) {
        return PR_REC_CHARACTER_LINK_CHECK_MEMORY;
    }
    link.offset = low;
    link.segment = high;
    if (!io->read_far_byte(io->context, link.segment,
                           (uint16_t)(link.offset + 0x2Eu), &raw_index)) {
        return PR_REC_CHARACTER_LINK_CHECK_MEMORY;
    }
    table_offset = (uint16_t)(0x5590u + ((uint16_t)raw_index << 4));
    if (!io->read_global_byte(io->context, table_offset, &table_value)) {
        return PR_REC_CHARACTER_LINK_CHECK_MEMORY;
    }
    *result = (uint8_t)((table_value & 0x14u) == 0x14u);
    return PR_REC_CHARACTER_LINK_CHECK_OK;
}
