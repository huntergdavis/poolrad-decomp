#include "poolrad/recovered/resident_party_count.h"

static int nonzero(pr_rec_far_ptr pointer) {
    return (pointer.offset | pointer.segment) != 0u;
}

static int read_far(const pr_rec_party_count_io *io, pr_rec_far_ptr base,
                    uint16_t field, pr_rec_far_ptr *value) {
    uint16_t low, high;
    if (!io->read_far_word(io->context, base.segment,
                           (uint16_t)(base.offset + field), &low) ||
        !io->read_far_word(io->context, base.segment,
                           (uint16_t)(base.offset + field + 2u), &high)) {
        return 0;
    }
    value->offset = low;
    value->segment = high;
    return 1;
}

pr_rec_party_count_status pr_rec_res_00ba_23db(const pr_rec_party_count_io *io) {
    pr_rec_far_ptr current, next;
    uint8_t selected, raw_index, counter;
    uint16_t low, high, counter_offset;
    size_t visited = 0u;

    if (io == NULL || io->read_global_word == NULL ||
        io->read_global_byte == NULL || io->write_global_byte == NULL ||
        io->read_far_word == NULL || io->read_far_byte == NULL ||
        io->max_nodes == 0u) {
        return PR_REC_PARTY_COUNT_ARGUMENT;
    }

    /* 0x2F81..0x2F95: clear only the two observed globals, then load the
     * party head from two separate DS words. */
    if (!io->write_global_byte(io->context, 0x6814u, 0u) ||
        !io->write_global_byte(io->context, 0x6815u, 0u) ||
        !io->read_global_word(io->context, 0x5D96u, &low) ||
        !io->read_global_word(io->context, 0x5D98u, &high)) {
        return PR_REC_PARTY_COUNT_MEMORY;
    }
    current.offset = low;
    current.segment = high;

    /* 0x2F98..0x2FCD: the all-zero far pointer ends the walk. The local
     * +0x10E byte is sign-extended before the DS counter address is formed;
     * the DOS code does not constrain the index to the two cleared bytes. */
    while (nonzero(current)) {
        if (visited == io->max_nodes) {
            return PR_REC_PARTY_COUNT_LIMIT;
        }
        visited++;
        if (!io->read_far_byte(io->context, current.segment,
                               (uint16_t)(current.offset + 0x10Du),
                               &selected)) {
            return PR_REC_PARTY_COUNT_MEMORY;
        }
        if (selected != 0u) {
            if (!io->read_far_byte(io->context, current.segment,
                                   (uint16_t)(current.offset + 0x10Eu),
                                   &raw_index)) {
                return PR_REC_PARTY_COUNT_MEMORY;
            }
            counter_offset = (uint16_t)(0x6814u + (int16_t)(int8_t)raw_index);
            if (!io->read_global_byte(io->context, counter_offset,
                                      &counter) ||
                !io->write_global_byte(io->context, counter_offset,
                                       (uint8_t)(counter + 1u))) {
                return PR_REC_PARTY_COUNT_MEMORY;
            }
        }
        if (!read_far(io, current, 0x104u, &next)) {
            return PR_REC_PARTY_COUNT_MEMORY;
        }
        current = next;
    }
    return PR_REC_PARTY_COUNT_OK;
}
