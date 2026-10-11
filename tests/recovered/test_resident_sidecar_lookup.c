/* Synthetic paths for START.EXE RES_00BA_2199. No game record/text bytes. */
#include "poolrad/recovered/resident_sidecar_lookup.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint8_t memory[3][65536];
    char trace[192];
    size_t trace_length;
    unsigned word_reads, word_writes, byte_reads;
    unsigned fail_word_read_at, fail_word_write_at, fail_byte_read_at;
} fixture;

static pr_rec_far_ptr ptr(uint16_t offset, uint16_t segment) {
    pr_rec_far_ptr result = {offset, segment};
    return result;
}

static int same(pr_rec_far_ptr a, pr_rec_far_ptr b) {
    return a.offset == b.offset && a.segment == b.segment;
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

static int trace_is(fixture *state, const char *expected) {
    if (strcmp(state->trace, expected) != 0) {
        fprintf(stderr, "trace got %s expected %s\n", state->trace, expected);
        return 0;
    }
    return 1;
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

static void set_node(fixture *state, pr_rec_far_ptr node, uint8_t key,
                     pr_rec_far_ptr next) {
    segment_bytes(state, node.segment)[node.offset] = key;
    set_far(state, node, 5u, next);
}

static int on_read_word(void *context, uint16_t segment, uint16_t offset,
                        uint16_t *value) {
    fixture *state = context;
    uint8_t *bytes = segment_bytes(state, segment);
    event(state, 'R');
    state->word_reads++;
    if (bytes == NULL || state->word_reads == state->fail_word_read_at) {
        return 0;
    }
    *value = (uint16_t)(bytes[offset] |
                        ((uint16_t)bytes[(uint16_t)(offset + 1u)] << 8));
    return 1;
}

static int on_write_word(void *context, uint16_t segment, uint16_t offset,
                         uint16_t value) {
    fixture *state = context;
    uint8_t *bytes = segment_bytes(state, segment);
    event(state, 'W');
    state->word_writes++;
    if (bytes == NULL || state->word_writes == state->fail_word_write_at) {
        return 0;
    }
    bytes[offset] = (uint8_t)value;
    bytes[(uint16_t)(offset + 1u)] = (uint8_t)(value >> 8);
    return 1;
}

static int on_read_byte(void *context, uint16_t segment, uint16_t offset,
                        uint8_t *value) {
    fixture *state = context;
    uint8_t *bytes = segment_bytes(state, segment);
    event(state, 'B');
    state->byte_reads++;
    if (bytes == NULL || state->byte_reads == state->fail_byte_read_at) {
        return 0;
    }
    *value = bytes[offset];
    return 1;
}

static pr_rec_lookup_io make_io(fixture *state, size_t max_nodes) {
    pr_rec_lookup_io io = {state, on_read_word, on_write_word, on_read_byte,
                           max_nodes};
    return io;
}

static int test_empty_and_head_match(void) {
    fixture state = {0};
    pr_rec_lookup_io io = make_io(&state, 3u);
    pr_rec_far_ptr owner = ptr(0x0200u, 0x2000u);
    pr_rec_far_ptr cursor = ptr(0xFFFEu, 0x3000u);
    pr_rec_far_ptr head = ptr(0x0500u, 0x4000u);
    uint8_t found = 0xA5u;

    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_OK);
    CHECK(found == 0u);
    CHECK(same(get_far(&state, cursor, 0u), ptr(0u, 0u)));
    CHECK(trace_is(&state, "RRWWRR"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 3u);
    found = 0xA5u;
    set_far(&state, owner, 0x7Fu, head);
    set_node(&state, head, 7u, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 0xAB07u, owner, &found) ==
          PR_REC_LOOKUP_OK);
    CHECK(found == 1u);
    CHECK(same(get_far(&state, cursor, 0u), head));
    CHECK(same(get_far(&state, owner, 0x7Fu), head));
    CHECK(trace_is(&state, "RRWWRRRRBRR"));
    return 0;
}

static int test_later_match_and_exhaustion(void) {
    fixture state = {0};
    pr_rec_lookup_io io = make_io(&state, 3u);
    pr_rec_far_ptr owner = ptr(0x0200u, 0x2000u);
    pr_rec_far_ptr cursor = ptr(0x0100u, 0x3000u);
    pr_rec_far_ptr first = ptr(0x0500u, 0x4000u);
    pr_rec_far_ptr second = ptr(0x0600u, 0x4000u);
    uint8_t found = 0xA5u;
    set_far(&state, owner, 0x7Fu, first);
    set_node(&state, first, 4u, second);
    set_node(&state, second, 7u, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_OK);
    CHECK(found == 1u);
    CHECK(same(get_far(&state, cursor, 0u), second));
    CHECK(same(get_far(&state, owner, 0x7Fu), first));
    CHECK(same(get_far(&state, first, 5u), second));
    CHECK(trace_is(&state, "RRWW" "RRRRBRRRRWW" "RRRRBRR"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 3u);
    found = 0xA5u;
    set_far(&state, owner, 0x7Fu, first);
    set_node(&state, first, 4u, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_OK);
    CHECK(found == 0u);
    CHECK(same(get_far(&state, cursor, 0u), ptr(0u, 0u)));
    CHECK(same(get_far(&state, owner, 0x7Fu), first));
    CHECK(trace_is(&state, "RRWW" "RRRRBRRRRWW" "RR"));
    return 0;
}

static int test_half_zero_and_alias(void) {
    fixture state = {0};
    pr_rec_lookup_io io = make_io(&state, 2u);
    pr_rec_far_ptr owner = ptr(0x0200u, 0x2000u);
    pr_rec_far_ptr cursor = ptr(0x0100u, 0x3000u);
    pr_rec_far_ptr head = ptr(0u, 0x4000u);
    uint8_t found = 0xA5u;
    set_far(&state, owner, 0x7Fu, head);
    set_node(&state, head, 0u, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 0u, owner, &found) ==
          PR_REC_LOOKUP_OK);
    CHECK(found == 1u);
    CHECK(same(get_far(&state, cursor, 0u), head));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 2u);
    cursor = ptr((uint16_t)(owner.offset + 0x7Fu), owner.segment);
    found = 0xA5u;
    set_far(&state, owner, 0x7Fu, head);
    set_node(&state, head, 4u, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_OK);
    CHECK(found == 0u);
    CHECK(same(get_far(&state, owner, 0x7Fu), ptr(0u, 0u)));
    CHECK(trace_is(&state, "RRWW" "RRRRBRRRRWW" "RR"));
    return 0;
}

static int test_cycle_and_failures(void) {
    fixture state = {0};
    pr_rec_lookup_io io = make_io(&state, 2u);
    pr_rec_far_ptr owner = ptr(0x0200u, 0x2000u);
    pr_rec_far_ptr cursor = ptr(0x0100u, 0x3000u);
    pr_rec_far_ptr first = ptr(0x0500u, 0x4000u);
    pr_rec_far_ptr second = ptr(0x0600u, 0x4000u);
    uint8_t found = 0xA5u;
    set_far(&state, owner, 0x7Fu, first);
    set_node(&state, first, 4u, second);
    set_node(&state, second, 5u, first);
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_LIMIT);
    CHECK(found == 0xA5u);
    CHECK(same(get_far(&state, cursor, 0u), first));
    CHECK(trace_is(&state, "RRWW" "RRRRBRRRRWW" "RRRRBRRRRWW" "RR"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 2u);
    state.fail_word_read_at = 1u;
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_MEMORY);
    CHECK(found == 0xA5u);
    CHECK(trace_is(&state, "R"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 2u);
    state.fail_word_write_at = 1u;
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_MEMORY);
    CHECK(trace_is(&state, "RRW"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 2u);
    set_far(&state, owner, 0x7Fu, first);
    state.fail_byte_read_at = 1u;
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_MEMORY);
    CHECK(same(get_far(&state, cursor, 0u), first));
    CHECK(trace_is(&state, "RRWWRRRRB"));
    CHECK(found == 0xA5u);

    CHECK(pr_rec_res_00ba_2199(NULL, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_ARGUMENT);
    io.max_nodes = 0u;
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, &found) ==
          PR_REC_LOOKUP_ARGUMENT);
    CHECK(pr_rec_res_00ba_2199(&io, cursor, 7u, owner, NULL) ==
          PR_REC_LOOKUP_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_empty_and_head_match() || test_later_match_and_exhaustion() ||
        test_half_zero_and_alias() || test_cycle_and_failures()) {
        return 1;
    }
    puts("resident sidecar lookup synthetic paths passed");
    return 0;
}
