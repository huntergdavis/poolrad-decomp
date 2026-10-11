/* Synthetic paths for START.EXE RES_00BA_15F7. No DOS record or text bytes. */
#include "poolrad/recovered/resident_sidecar_append.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint8_t memory[3][65536];
    char trace[128];
    size_t trace_length;
    pr_rec_far_ptr block, copied_source, copied_destination;
    uint16_t allocated_bytes, copied_bytes;
    unsigned read_count, write_count;
    unsigned fail_read_at, fail_write_at;
    int fail_allocate, fail_copy;
} fixture;

static pr_rec_far_ptr ptr(uint16_t offset, uint16_t segment) {
    pr_rec_far_ptr result = {offset, segment};
    return result;
}

static uint8_t *segment_bytes(fixture *state, uint16_t segment) {
    if (segment == 0x2000u) return state->memory[0];
    if (segment == 0x3000u) return state->memory[1];
    if (segment == 0x4000u) return state->memory[2];
    return NULL;
}

static void event(fixture *state, char code) {
    if (state->trace_length + 1u < sizeof state->trace) {
        state->trace[state->trace_length++] = code;
        state->trace[state->trace_length] = '\0';
    }
}

static void set_far(fixture *state, pr_rec_far_ptr base, uint16_t field,
                    pr_rec_far_ptr value) {
    uint8_t *bytes = segment_bytes(state, base.segment);
    uint16_t at = (uint16_t)(base.offset + field);
    uint16_t high = (uint16_t)(at + 2u);
    bytes[at] = (uint8_t)value.offset;
    bytes[(uint16_t)(at + 1u)] = (uint8_t)(value.offset >> 8);
    bytes[high] = (uint8_t)value.segment;
    bytes[(uint16_t)(high + 1u)] = (uint8_t)(value.segment >> 8);
}

static pr_rec_far_ptr get_far(fixture *state, pr_rec_far_ptr base,
                              uint16_t field) {
    uint8_t *bytes = segment_bytes(state, base.segment);
    uint16_t at = (uint16_t)(base.offset + field);
    uint16_t high = (uint16_t)(at + 2u);
    return ptr((uint16_t)(bytes[at] |
                          ((uint16_t)bytes[(uint16_t)(at + 1u)] << 8)),
               (uint16_t)(bytes[high] |
                          ((uint16_t)bytes[(uint16_t)(high + 1u)] << 8)));
}

static int same(pr_rec_far_ptr a, pr_rec_far_ptr b) {
    return a.offset == b.offset && a.segment == b.segment;
}

static int on_allocate(void *context, uint16_t bytes,
                       pr_rec_far_ptr *block) {
    fixture *state = context;
    event(state, 'A');
    state->allocated_bytes = bytes;
    if (state->fail_allocate) return 0;
    *block = state->block;
    return 1;
}

static int on_copy(void *context, pr_rec_far_ptr source,
                   pr_rec_far_ptr destination, uint16_t bytes) {
    fixture *state = context;
    uint8_t *source_bytes = segment_bytes(state, source.segment);
    uint8_t *destination_bytes = segment_bytes(state, destination.segment);
    uint8_t temporary[PR_REC_APPEND_BLOCK_BYTES];
    uint16_t i;
    event(state, 'C');
    state->copied_source = source;
    state->copied_destination = destination;
    state->copied_bytes = bytes;
    if (state->fail_copy || source_bytes == NULL ||
        destination_bytes == NULL || bytes != sizeof temporary) {
        return 0;
    }
    for (i = 0u; i < bytes; ++i) {
        temporary[i] = source_bytes[(uint16_t)(source.offset + i)];
    }
    for (i = 0u; i < bytes; ++i) {
        destination_bytes[(uint16_t)(destination.offset + i)] = temporary[i];
    }
    return 1;
}

static int on_read(void *context, uint16_t segment, uint16_t offset,
                   uint16_t *value) {
    fixture *state = context;
    uint8_t *bytes = segment_bytes(state, segment);
    event(state, 'R');
    state->read_count++;
    if (bytes == NULL || state->read_count == state->fail_read_at) return 0;
    *value = (uint16_t)(bytes[offset] |
                        ((uint16_t)bytes[(uint16_t)(offset + 1u)] << 8));
    return 1;
}

static int on_write(void *context, uint16_t segment, uint16_t offset,
                    uint16_t value) {
    fixture *state = context;
    uint8_t *bytes = segment_bytes(state, segment);
    event(state, 'W');
    state->write_count++;
    if (bytes == NULL || state->write_count == state->fail_write_at) return 0;
    bytes[offset] = (uint8_t)value;
    bytes[(uint16_t)(offset + 1u)] = (uint8_t)(value >> 8);
    return 1;
}

static pr_rec_append_io make_io(fixture *state, size_t max_nodes) {
    pr_rec_append_io io = {state, on_allocate, on_copy, on_read, on_write,
                           max_nodes};
    return io;
}

static void seed_source(fixture *state, pr_rec_far_ptr source) {
    uint8_t *bytes = segment_bytes(state, source.segment);
    uint16_t i;
    for (i = 0u; i < PR_REC_APPEND_BLOCK_BYTES; ++i) {
        bytes[(uint16_t)(source.offset + i)] = (uint8_t)(i + 1u);
    }
    set_far(state, source, 0x2Au, ptr(0x7777u, 0x8888u));
}

static int copied_except_link(fixture *state, pr_rec_far_ptr source,
                              pr_rec_far_ptr block) {
    uint8_t *source_bytes = segment_bytes(state, source.segment);
    uint8_t *block_bytes = segment_bytes(state, block.segment);
    uint16_t i;
    for (i = 0u; i < PR_REC_APPEND_BLOCK_BYTES; ++i) {
        uint8_t got = block_bytes[(uint16_t)(block.offset + i)];
        if (i >= 0x2Au && i <= 0x2Du) {
            if (got != 0u) return 0;
        } else if (got != source_bytes[(uint16_t)(source.offset + i)]) {
            return 0;
        }
    }
    return 1;
}

static int test_empty_head_and_wrap(void) {
    fixture state = {0};
    pr_rec_append_io io = make_io(&state, 3u);
    pr_rec_far_ptr source = ptr(0x0100u, 0x3000u);
    pr_rec_far_ptr owner = ptr(0xFF50u, 0x2000u);
    state.block = ptr(0x0300u, 0x4000u);
    seed_source(&state, source);
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) == PR_REC_APPEND_OK);
    CHECK(strcmp(state.trace, "ACWWRRWW") == 0);
    CHECK(state.allocated_bytes == 0x3Fu && state.copied_bytes == 0x3Fu);
    CHECK(same(state.copied_source, source));
    CHECK(same(state.copied_destination, state.block));
    CHECK(same(get_far(&state, owner, 0xC8u), state.block));
    CHECK(same(get_far(&state, state.block, 0x2Au), ptr(0u, 0u)));
    CHECK(same(get_far(&state, source, 0x2Au), ptr(0x7777u, 0x8888u)));
    CHECK(copied_except_link(&state, source, state.block));
    return 0;
}

static int test_one_and_two_node_tails(void) {
    fixture state = {0};
    pr_rec_append_io io = make_io(&state, 2u);
    pr_rec_far_ptr source = ptr(0x0100u, 0x3000u);
    pr_rec_far_ptr owner = ptr(0x0200u, 0x2000u);
    pr_rec_far_ptr first = ptr(0x0500u, 0x4000u);
    pr_rec_far_ptr second = ptr(0x0600u, 0x4000u);
    state.block = ptr(0x0300u, 0x4000u);
    seed_source(&state, source);
    set_far(&state, owner, 0xC8u, first);
    set_far(&state, first, 0x2Au, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) == PR_REC_APPEND_OK);
    CHECK(strcmp(state.trace, "ACWWRRRRWW") == 0);
    CHECK(same(get_far(&state, owner, 0xC8u), first));
    CHECK(same(get_far(&state, first, 0x2Au), state.block));
    CHECK(copied_except_link(&state, source, state.block));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 2u);
    state.block = ptr(0x0300u, 0x4000u);
    seed_source(&state, source);
    set_far(&state, owner, 0xC8u, first);
    set_far(&state, first, 0x2Au, second);
    set_far(&state, second, 0x2Au, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) == PR_REC_APPEND_OK);
    CHECK(strcmp(state.trace, "ACWWRRRRRRRRWW") == 0);
    CHECK(same(get_far(&state, first, 0x2Au), second));
    CHECK(same(get_far(&state, second, 0x2Au), state.block));
    return 0;
}

static int test_half_zero_pointer_and_cycle(void) {
    fixture state = {0};
    pr_rec_append_io io = make_io(&state, 2u);
    pr_rec_far_ptr source = ptr(0x0100u, 0x3000u);
    pr_rec_far_ptr owner = ptr(0x0200u, 0x2000u);
    pr_rec_far_ptr first = ptr(0u, 0x4000u);
    pr_rec_far_ptr second = ptr(0x0600u, 0x4000u);
    state.block = ptr(0x0300u, 0x4000u);
    seed_source(&state, source);
    set_far(&state, owner, 0xC8u, first);
    set_far(&state, first, 0x2Au, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) == PR_REC_APPEND_OK);
    CHECK(strcmp(state.trace, "ACWWRRRRWW") == 0);
    CHECK(same(get_far(&state, first, 0x2Au), state.block));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 2u);
    state.block = ptr(0x0300u, 0x4000u);
    seed_source(&state, source);
    set_far(&state, owner, 0xC8u, first);
    set_far(&state, first, 0x2Au, second);
    set_far(&state, second, 0x2Au, first);
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) == PR_REC_APPEND_LIMIT);
    CHECK(strcmp(state.trace, "ACWWRRRRRRRRRR") == 0);
    CHECK(same(get_far(&state, second, 0x2Au), first));
    CHECK(same(get_far(&state, state.block, 0x2Au), ptr(0u, 0u)));
    return 0;
}

static int test_host_failures(void) {
    fixture state = {0};
    pr_rec_append_io io = make_io(&state, 1u);
    pr_rec_far_ptr source = ptr(0x0100u, 0x3000u);
    pr_rec_far_ptr owner = ptr(0x0200u, 0x2000u);
    state.block = ptr(0x0300u, 0x4000u);
    state.fail_allocate = 1;
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) ==
          PR_REC_APPEND_ALLOCATE_ABORT);
    CHECK(strcmp(state.trace, "A") == 0);

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) ==
          PR_REC_APPEND_ALLOCATE_ABORT);
    CHECK(strcmp(state.trace, "A") == 0);

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    state.block = ptr(0x0300u, 0x4000u);
    state.fail_copy = 1;
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) ==
          PR_REC_APPEND_COPY_FAILURE);
    CHECK(strcmp(state.trace, "AC") == 0);

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    state.block = ptr(0x0300u, 0x4000u);
    state.fail_read_at = 1u;
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) == PR_REC_APPEND_MEMORY);
    CHECK(strcmp(state.trace, "ACWWR") == 0);

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    state.block = ptr(0x0300u, 0x4000u);
    state.fail_write_at = 1u;
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) == PR_REC_APPEND_MEMORY);
    CHECK(strcmp(state.trace, "ACW") == 0);

    CHECK(pr_rec_res_00ba_15f7(NULL, source, owner) == PR_REC_APPEND_ARGUMENT);
    io.max_nodes = 0u;
    CHECK(pr_rec_res_00ba_15f7(&io, source, owner) == PR_REC_APPEND_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_empty_head_and_wrap() || test_one_and_two_node_tails() ||
        test_half_zero_pointer_and_cycle() || test_host_failures()) {
        return 1;
    }
    puts("resident sidecar append synthetic paths passed");
    return 0;
}
