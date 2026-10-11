#include "poolrad/recovered/resident_sidecar_remove.h"

static int nonzero(pr_rec_far_ptr pointer) {
    return (pointer.offset | pointer.segment) != 0u;
}

static int same(pr_rec_far_ptr a, pr_rec_far_ptr b) {
    return a.offset == b.offset && a.segment == b.segment;
}

static int read_far(const pr_rec_remove_io *io, pr_rec_far_ptr base,
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

static int write_far(const pr_rec_remove_io *io, pr_rec_far_ptr base,
                     uint16_t field, pr_rec_far_ptr value) {
    if (!io->write_word(io->context, base.segment,
                        (uint16_t)(base.offset + field), value.offset) ||
        !io->write_word(io->context, base.segment,
                        (uint16_t)(base.offset + field + 2u), value.segment)) {
        return 0;
    }
    return 1;
}

static void report(const pr_rec_remove_io *io, uint16_t literal_offset) {
    io->copy_literal(io->context, 0x00BAu, literal_offset);
    io->present_message(io->context, PR_REC_REMOVE_MESSAGE_PARAMETER);
}

pr_rec_remove_status pr_rec_res_00ba_1509(const pr_rec_remove_io *io,
                                           pr_rec_far_ptr target,
                                           pr_rec_far_ptr character) {
    pr_rec_far_ptr current, next;
    size_t visited = 0u;
    const pr_rec_far_ptr null_pointer = {0u, 0u};

    if (io == NULL || io->read_word == NULL || io->write_word == NULL ||
        io->copy_literal == NULL || io->present_message == NULL ||
        io->release_block == NULL || io->max_nodes == 0u) {
        return PR_REC_REMOVE_ARGUMENT;
    }

    /* 0x20AF..0x20CE: reject only the all-zero target before dereferencing
     * the character. The original copies a local Pascal string and presents
     * it, then returns; the callbacks preserve that order. */
    if (!nonzero(target)) {
        report(io, PR_REC_REMOVE_NULL_LITERAL);
        return PR_REC_REMOVE_REPORTED_NULL;
    }

    /* 0x20D1..0x210C: read the character's head. A head match updates the
     * head before the shared success block runs. */
    if (!read_far(io, character, 0xC8u, &current)) {
        return PR_REC_REMOVE_MEMORY;
    }
    if (same(current, target)) {
        if (!read_far(io, target, 0x2Au, &next) ||
            !write_far(io, character, 0xC8u, next)) {
            return PR_REC_REMOVE_MEMORY;
        }
    } else {
        /* 0x210E..0x213C: compare each current->next with target. The
         * original reads current->next a second time on a non-match before
         * the loop repeats; keep that access order. */
        while (nonzero(current)) {
            if (visited == io->max_nodes) {
                return PR_REC_REMOVE_LIMIT;
            }
            visited++;
            if (!read_far(io, current, 0x2Au, &next)) {
                return PR_REC_REMOVE_MEMORY;
            }
            if (same(next, target)) {
                break;
            }
            if (!read_far(io, current, 0x2Au, &current)) {
                return PR_REC_REMOVE_MEMORY;
            }
        }
    }

    /* 0x213E..0x215D: an empty head or exhausted chain takes the second
     * message path. No link or allocation state has been changed there. */
    if (!nonzero(current)) {
        report(io, PR_REC_REMOVE_MISSING_LITERAL);
        return PR_REC_REMOVE_REPORTED_MISSING;
    }

    /* 0x215F..0x218C: splice target out, clear its link, then FreeMem the
     * original target pointer with the 0x3F-byte size. When current==target
     * (head case), the first link write is a redundant self-assignment in
     * the DOS instructions; preserve it before the clear. */
    if (!read_far(io, target, 0x2Au, &next) ||
        !write_far(io, current, 0x2Au, next) ||
        !write_far(io, target, 0x2Au, null_pointer)) {
        return PR_REC_REMOVE_MEMORY;
    }
    if (!io->release_block(io->context, target, PR_REC_REMOVE_BLOCK_BYTES)) {
        return PR_REC_REMOVE_RELEASE_ABORT;
    }
    return PR_REC_REMOVE_OK;
}
