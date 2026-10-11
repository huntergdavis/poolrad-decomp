#include "poolrad/recovered/resident_character_target_reset.h"

#include <stddef.h>

pr_rec_target_reset_status pr_rec_res_00ba_2637(
    const pr_rec_target_reset_io *io, pr_rec_reset_far_ptr character,
    uint16_t *ax)
{
    static const uint8_t target_fields[4] = {3u, 0u, 7u, 6u};
    unsigned i;

    if (io == NULL || io->read_far_word == NULL ||
        io->write_far_byte == NULL || ax == NULL)
        return PR_REC_TARGET_RESET_ARGUMENT;

    for (i = 0; i < 4; ++i) {
        uint16_t target_offset, target_segment;
        if (!io->read_far_word(io->context, character.segment,
                               (uint16_t)(character.offset + 0x108u),
                               &target_offset) ||
            !io->read_far_word(io->context, character.segment,
                               (uint16_t)(character.offset + 0x10au),
                               &target_segment))
            return PR_REC_TARGET_RESET_MEMORY;
        if (!io->write_far_byte(io->context, target_segment,
                                (uint16_t)(target_offset + target_fields[i]),
                                0u))
            return PR_REC_TARGET_RESET_MEMORY;
    }

    *ax = (uint16_t)((*ax & 0xff00u) | 1u);
    return PR_REC_TARGET_RESET_OK;
}
