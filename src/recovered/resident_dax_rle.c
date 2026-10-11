#include "poolrad/recovered/resident_dax_rle.h"

static pr_rec_dax_far add_offset(pr_rec_dax_far p, uint16_t delta) {
    p.offset = (uint16_t)(p.offset + delta);
    return p;
}

static int read_word(const pr_rec_dax_memory *memory, pr_rec_dax_far address,
                     uint16_t *value) {
    uint8_t lo, hi;
    if (memory->read8(memory->ctx, address, &lo) ||
        memory->read8(memory->ctx, add_offset(address, 1), &hi)) return 1;
    *value = (uint16_t)((uint16_t)lo | ((uint16_t)hi << 8));
    return 0;
}

static int read_far(const pr_rec_dax_memory *memory, pr_rec_dax_far address,
                    pr_rec_dax_far *value) {
    if (read_word(memory, address, &value->offset) ||
        read_word(memory, add_offset(address, 2), &value->segment)) return 1;
    return 0;
}

/* The audited 0xAF8:0x1685 Move compares SI and DI offsets, regardless of
 * segment. It copies backward when SI < DI, leaving DF set in that case. */
static int move_bytes(const pr_rec_dax_memory *memory, pr_rec_dax_far source,
                      pr_rec_dax_far destination, uint16_t count,
                      pr_rec_dax_rle_state *state) {
    uint16_t n;
    int backward = source.offset < destination.offset;
    state->direction_flag = (uint8_t)backward;
    for (n = 0; n < count; ++n) {
        uint16_t delta = backward ? (uint16_t)(count - 1u - n) : n;
        uint8_t value;
        if (memory->read8(memory->ctx, add_offset(source, delta), &value) ||
            memory->write8(memory->ctx, add_offset(destination, delta), value))
            return 1;
    }
    return 0;
}

/* The audited 0xAF8:0x16A8 FillChar clears DF and writes forward. */
static int fill_bytes(const pr_rec_dax_memory *memory, pr_rec_dax_far destination,
                      uint8_t value, uint16_t count,
                      pr_rec_dax_rle_state *state) {
    uint16_t n;
    state->direction_flag = 0;
    for (n = 0; n < count; ++n)
        if (memory->write8(memory->ctx, add_offset(destination, n), value))
            return 1;
    return 0;
}

pr_rec_dax_rle_status pr_rec_res_0802_050f(
    const pr_rec_dax_memory *memory, const pr_rec_dax_rle_args *args,
    size_t max_iterations, pr_rec_dax_rle_state *state) {
    size_t iteration;
    if (!memory || !memory->read8 || !memory->write8 || !args || !state ||
        !max_iterations || state->direction_flag > 1)
        return PR_REC_DAX_RLE_ARGUMENT;
    state->source_index = 0;
    state->destination_index = 0;
    state->iterations = 0;

    for (iteration = 0; iteration < max_iterations; ++iteration) {
        pr_rec_dax_far source;
        uint16_t packed_size;
        uint8_t control;
        int stalled = 0;
        ++state->iterations;
        if (read_far(memory, args->source_pointer, &source) ||
            memory->read8(memory->ctx,
                          add_offset(source, state->source_index), &control))
            return PR_REC_DAX_RLE_MEMORY;
        if (control <= 0x7E) {
            pr_rec_dax_far destination;
            uint16_t count = (uint16_t)control + 1u;
            if (read_far(memory, args->source_pointer, &source) ||
                read_far(memory, args->destination_pointer, &destination))
                return PR_REC_DAX_RLE_MEMORY;
            source = add_offset(source, (uint16_t)(state->source_index + 1u));
            destination = add_offset(destination, state->destination_index);
            if (move_bytes(memory, source, destination, count, state))
                return PR_REC_DAX_RLE_MEMORY;
            state->source_index = (uint16_t)(state->source_index + count + 1u);
            state->destination_index =
                (uint16_t)(state->destination_index + count);
        } else if (control >= 0x81) {
            pr_rec_dax_far destination;
            uint8_t value;
            uint16_t count = 256u - (uint16_t)control;
            if (read_far(memory, args->destination_pointer, &destination) ||
                read_far(memory, args->source_pointer, &source))
                return PR_REC_DAX_RLE_MEMORY;
            destination = add_offset(destination, state->destination_index);
            source = add_offset(source, (uint16_t)(state->source_index + 1u));
            if (memory->read8(memory->ctx, source, &value) ||
                fill_bytes(memory, destination, value, count, state))
                return PR_REC_DAX_RLE_MEMORY;
            state->source_index = (uint16_t)(state->source_index + 2u);
            state->destination_index =
                (uint16_t)(state->destination_index + count);
        } else {
            stalled = 1; /* 0x7F and 0x80 change neither index nor DF. */
        }
        if (read_word(memory, args->packed_size, &packed_size))
            return PR_REC_DAX_RLE_MEMORY;
        if (state->source_index >= packed_size) return PR_REC_DAX_RLE_OK;
        if (stalled) return PR_REC_DAX_RLE_STALLED;
    }
    return PR_REC_DAX_RLE_ITERATION_LIMIT;
}
