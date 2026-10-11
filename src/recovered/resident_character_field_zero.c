#include "poolrad/recovered/resident_character_field_zero.h"

#include <stddef.h>

pr_rec_character_field_zero_status pr_rec_res_00ba_23b7(
    const pr_rec_character_field_zero_io *io, pr_rec_far_ptr character,
    uint8_t *is_zero) {
    uint8_t field;

    if (io == NULL || io->read_far_byte == NULL || is_zero == NULL) {
        return PR_REC_CHARACTER_FIELD_ZERO_ARGUMENT;
    }

    /* 0x2F5D..0x2F72: LES the caller's far pointer, compare its +0x10E
     * byte with zero, and assign the local Boolean for either branch. */
    if (!io->read_far_byte(io->context, character.segment,
                           (uint16_t)(character.offset + 0x10Eu), &field)) {
        return PR_REC_CHARACTER_FIELD_ZERO_MEMORY;
    }
    *is_zero = (uint8_t)(field == 0u);
    return PR_REC_CHARACTER_FIELD_ZERO_OK;
}
