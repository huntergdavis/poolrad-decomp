#include "poolrad/recovered/resident_sidecar_append.h"

static int nonzero(pr_rec_far_ptr pointer) {
    return (pointer.offset | pointer.segment) != 0u;
}

static int read_far(const pr_rec_append_io *io, pr_rec_far_ptr base,
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

static int write_far(const pr_rec_append_io *io, pr_rec_far_ptr base,
                     uint16_t field, pr_rec_far_ptr value) {
    if (!io->write_word(io->context, base.segment,
                        (uint16_t)(base.offset + field), value.offset) ||
        !io->write_word(io->context, base.segment,
                        (uint16_t)(base.offset + field + 2u), value.segment)) {
        return 0;
    }
    return 1;
}

pr_rec_append_status pr_rec_res_00ba_15f7(const pr_rec_append_io *io,
                                           pr_rec_far_ptr source,
                                           pr_rec_far_ptr character) {
    pr_rec_far_ptr block = {0u, 0u};
    pr_rec_far_ptr current, next;
    const pr_rec_far_ptr null_pointer = {0u, 0u};
    size_t visited = 0u;

    if (io == NULL || io->allocate_block == NULL || io->copy_block == NULL ||
        io->read_word == NULL || io->write_word == NULL ||
        io->max_nodes == 0u) {
        return PR_REC_APPEND_ARGUMENT;
    }

    /* 0x219D..0x21BF: allocate 63 bytes, then copy all 63 source bytes.
     * Both calls are SYSTEM dependencies. Neither result is inspected by
     * the DOS body after a successful call. */
    if (!io->allocate_block(io->context, PR_REC_APPEND_BLOCK_BYTES, &block) ||
        !nonzero(block)) {
        return PR_REC_APPEND_ALLOCATE_ABORT;
    }
    if (!io->copy_block(io->context, source, block,
                        PR_REC_APPEND_BLOCK_BYTES)) {
        return PR_REC_APPEND_COPY_FAILURE;
    }

    /* 0x21BF..0x21CC: replace the copied next pointer with all-zero. */
    if (!write_far(io, block, 0x2Au, null_pointer)) {
        return PR_REC_APPEND_MEMORY;
    }

    /* 0x21CC..0x21FA: an empty head becomes the allocated block. */
    if (!read_far(io, character, 0xC8u, &current)) {
        return PR_REC_APPEND_MEMORY;
    }
    if (!nonzero(current)) {
        return write_far(io, character, 0xC8u, block)
                   ? PR_REC_APPEND_OK : PR_REC_APPEND_MEMORY;
    }

    /* 0x21FC..0x2229: scan to the tail. On every non-null link, the DOS
     * instructions read the same two words again to advance. Preserve
     * this access order before writing the new tail link. */
    for (;;) {
        if (visited == io->max_nodes) {
            return PR_REC_APPEND_LIMIT;
        }
        visited++;
        if (!read_far(io, current, 0x2Au, &next)) {
            return PR_REC_APPEND_MEMORY;
        }
        if (!nonzero(next)) {
            break;
        }
        if (!read_far(io, current, 0x2Au, &current)) {
            return PR_REC_APPEND_MEMORY;
        }
    }
    return write_far(io, current, 0x2Au, block)
               ? PR_REC_APPEND_OK : PR_REC_APPEND_MEMORY;
}
