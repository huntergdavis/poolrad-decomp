#include "poolrad/recovered/resident_sidecar_lookup.h"

static int nonzero(pr_rec_far_ptr pointer) {
    return (pointer.offset | pointer.segment) != 0u;
}

static int read_far(const pr_rec_lookup_io *io, pr_rec_far_ptr base,
                    uint16_t field, pr_rec_far_ptr *value) {
    uint16_t low, high;
    if (!io->read_word(io->context, base.segment,
                       (uint16_t)(base.offset + field), &low) ||
        !io->read_word(io->context, base.segment,
                       (uint16_t)(base.offset + field + 2u), &high)) {
        return 0;
    }
    value->offset = low;
    value->segment = high;
    return 1;
}

static int write_far(const pr_rec_lookup_io *io, pr_rec_far_ptr base,
                     uint16_t field, pr_rec_far_ptr value) {
    if (!io->write_word(io->context, base.segment,
                        (uint16_t)(base.offset + field), value.offset) ||
        !io->write_word(io->context, base.segment,
                        (uint16_t)(base.offset + field + 2u), value.segment)) {
        return 0;
    }
    return 1;
}

pr_rec_lookup_status pr_rec_res_00ba_2199(const pr_rec_lookup_io *io,
                                           pr_rec_far_ptr cursor_cell,
                                           uint16_t key_word,
                                           pr_rec_far_ptr character,
                                           uint8_t *found) {
    pr_rec_far_ptr head, current, next;
    uint8_t observed, matched = 0u;
    size_t visited = 0u;

    if (io == NULL || found == NULL || io->read_word == NULL ||
        io->write_word == NULL || io->read_byte == NULL ||
        io->max_nodes == 0u) {
        return PR_REC_LOOKUP_ARGUMENT;
    }

    /* 0x2D43..0x2D55: copy the character's +0x7F far-pointer head into
     * the caller's cursor cell, low word followed by high word. */
    if (!read_far(io, character, 0x7Fu, &head) ||
        !write_far(io, cursor_cell, 0u, head)) {
        return PR_REC_LOOKUP_MEMORY;
    }

    for (;;) {
        /* 0x2D59..0x2D69: re-read the output cell for both the null test
         * and the matched exit. The final successful iteration reads it
         * once more before returning AL=1. */
        if (!read_far(io, cursor_cell, 0u, &current)) {
            return PR_REC_LOOKUP_MEMORY;
        }
        if (!nonzero(current) || matched != 0u) {
            *found = matched;
            return PR_REC_LOOKUP_OK;
        }
        if (visited == io->max_nodes) {
            return PR_REC_LOOKUP_LIMIT;
        }
        visited++;

        /* 0x2D6B..0x2D7D: load the same cursor cell again, then compare
         * the node's first byte with the low byte of the key argument. */
        if (!read_far(io, cursor_cell, 0u, &current) ||
            !io->read_byte(io->context, current.segment, current.offset,
                           &observed)) {
            return PR_REC_LOOKUP_MEMORY;
        }
        if (observed == (uint8_t)key_word) {
            matched = 1u;
            continue;
        }

        /* 0x2D7F..0x2D97: on a miss, reload the cursor cell again, read
         * the 9-byte node's link at +5/+7, and overwrite the cursor. */
        if (!read_far(io, cursor_cell, 0u, &current) ||
            !read_far(io, current, 5u, &next) ||
            !write_far(io, cursor_cell, 0u, next)) {
            return PR_REC_LOOKUP_MEMORY;
        }
    }
}
