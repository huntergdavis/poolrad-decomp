#include "poolrad/recovered/resident_character_target_select.h"

#include <stddef.h>

static bool read_pair(const pr_rec_target_select_io *io,
                      pr_rec_select_far_ptr base, uint16_t field,
                      pr_rec_select_far_ptr *value)
{
    return io->read_far_word(io->context, base.segment,
                             (uint16_t)(base.offset + field),
                             &value->offset) &&
           io->read_far_word(io->context, base.segment,
                             (uint16_t)(base.offset + field + 2u),
                             &value->segment);
}

static bool write_pair(const pr_rec_target_select_io *io,
                       pr_rec_select_far_ptr slot,
                       pr_rec_select_far_ptr value)
{
    return io->write_far_word(io->context, slot.segment, slot.offset,
                              value.offset) &&
           io->write_far_word(io->context, slot.segment,
                              (uint16_t)(slot.offset + 2u), value.segment);
}

pr_rec_target_select_status pr_rec_res_00ba_2e8b(
    const pr_rec_target_select_io *io, pr_rec_select_far_ptr character,
    pr_rec_select_far_ptr output_slot, uint16_t *ax)
{
    pr_rec_select_far_ptr link, candidate, final_pointer;
    uint8_t flags = 0, index;
    uint16_t combined;

    if (io == NULL || io->read_far_word == NULL ||
        io->read_far_byte == NULL || io->read_global_byte == NULL ||
        io->write_far_word == NULL || ax == NULL)
        return PR_REC_TARGET_SELECT_ARGUMENT;

    /* 0x3A35..0x3A54: save the character link before clearing the
     * caller's output slot, preserving alias effects and word order. */
    if (!read_pair(io, character, 0xccu, &link) ||
        !write_pair(io, output_slot, (pr_rec_select_far_ptr){0, 0}))
        return PR_REC_TARGET_SELECT_MEMORY;

    if ((uint16_t)(link.offset | link.segment) != 0) {
        /* 0x3A60..0x3A73: linked byte +0x2E selects a 16-byte DS entry. */
        if (!io->read_far_byte(io->context, link.segment,
                               (uint16_t)(link.offset + 0x2eu), &index) ||
            !io->read_global_byte(io->context,
                                  (uint16_t)(0x5590u + ((uint16_t)index << 4)),
                                  &flags))
            return PR_REC_TARGET_SELECT_MEMORY;

        /* 0x3A76..0x3A8F: bit 0x10 selects the original link. */
        if ((flags & 0x10u) != 0 && !write_pair(io, output_slot, link))
            return PR_REC_TARGET_SELECT_MEMORY;

        if ((flags & 0x08u) != 0) {
            /* 0x3A98..0x3AB8: bit 0x01 replaces it with character +0xF8. */
            if ((flags & 0x01u) != 0) {
                if (!read_pair(io, character, 0xf8u, &candidate) ||
                    !write_pair(io, output_slot, candidate))
                    return PR_REC_TARGET_SELECT_MEMORY;
            }
            /* 0x3AB8..0x3AD8: bit 0x80 has the last-write priority. */
            if ((flags & 0x80u) != 0) {
                if (!read_pair(io, character, 0xfcu, &candidate) ||
                    !write_pair(io, output_slot, candidate))
                    return PR_REC_TARGET_SELECT_MEMORY;
            }
        }
    }

    /* 0x3AD8..0x3AF7: reread both output words. AL is true for a
     * non-null output or exactly flags 0x0A. AH is the high byte of OR. */
    if (!read_pair(io, output_slot, 0, &final_pointer))
        return PR_REC_TARGET_SELECT_MEMORY;
    combined = (uint16_t)(final_pointer.offset | final_pointer.segment);
    *ax = (uint16_t)((combined & 0xff00u) |
                     (combined != 0 || flags == 0x0au ? 1u : 0u));
    return PR_REC_TARGET_SELECT_OK;
}
