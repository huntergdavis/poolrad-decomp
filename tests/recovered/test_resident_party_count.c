/* Synthetic paths for START.EXE RES_00BA_23DB. No DOS character bytes. */
#include "poolrad/recovered/resident_party_count.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint8_t globals[65536];
    uint8_t far_bytes[2][65536];
    char trace[160];
    size_t trace_length;
    unsigned global_word_reads, global_byte_reads, global_byte_writes;
    unsigned far_word_reads, far_byte_reads;
    unsigned fail_global_word_at, fail_global_byte_at;
    unsigned fail_global_write_at, fail_far_word_at, fail_far_byte_at;
} fixture;

static pr_rec_far_ptr ptr(uint16_t offset, uint16_t segment) {
    pr_rec_far_ptr result = {offset, segment};
    return result;
}

static uint8_t *far_segment(fixture *state, uint16_t segment) {
    if (segment == 0x2000u) return state->far_bytes[0];
    if (segment == 0x3000u) return state->far_bytes[1];
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

static void set_global_word(fixture *state, uint16_t offset, uint16_t value) {
    state->globals[offset] = (uint8_t)value;
    state->globals[(uint16_t)(offset + 1u)] = (uint8_t)(value >> 8);
}

static void set_head(fixture *state, pr_rec_far_ptr head) {
    set_global_word(state, 0x5D96u, head.offset);
    set_global_word(state, 0x5D98u, head.segment);
}

static void set_far(fixture *state, pr_rec_far_ptr base, uint16_t field,
                    pr_rec_far_ptr value) {
    uint8_t *bytes = far_segment(state, base.segment);
    uint16_t at = (uint16_t)(base.offset + field);
    uint16_t high = (uint16_t)(at + 2u);
    bytes[at] = (uint8_t)value.offset;
    bytes[(uint16_t)(at + 1u)] = (uint8_t)(value.offset >> 8);
    bytes[high] = (uint8_t)value.segment;
    bytes[(uint16_t)(high + 1u)] = (uint8_t)(value.segment >> 8);
}

static void set_character(fixture *state, pr_rec_far_ptr character,
                          uint8_t selected, uint8_t raw_index,
                          pr_rec_far_ptr next) {
    uint8_t *bytes = far_segment(state, character.segment);
    bytes[(uint16_t)(character.offset + 0x10Du)] = selected;
    bytes[(uint16_t)(character.offset + 0x10Eu)] = raw_index;
    set_far(state, character, 0x104u, next);
}

static int on_global_word(void *context, uint16_t offset, uint16_t *value) {
    fixture *state = context;
    event(state, 'H');
    state->global_word_reads++;
    if (state->global_word_reads == state->fail_global_word_at) return 0;
    *value = (uint16_t)(state->globals[offset] |
                        ((uint16_t)state->globals[(uint16_t)(offset + 1u)] << 8));
    return 1;
}

static int on_global_byte(void *context, uint16_t offset, uint8_t *value) {
    fixture *state = context;
    event(state, 'R');
    state->global_byte_reads++;
    if (state->global_byte_reads == state->fail_global_byte_at) return 0;
    *value = state->globals[offset];
    return 1;
}

static int on_global_write(void *context, uint16_t offset, uint8_t value) {
    fixture *state = context;
    event(state, 'W');
    state->global_byte_writes++;
    if (state->global_byte_writes == state->fail_global_write_at) return 0;
    state->globals[offset] = value;
    return 1;
}

static int on_far_word(void *context, uint16_t segment, uint16_t offset,
                       uint16_t *value) {
    fixture *state = context;
    uint8_t *bytes = far_segment(state, segment);
    event(state, 'F');
    state->far_word_reads++;
    if (bytes == NULL || state->far_word_reads == state->fail_far_word_at) {
        return 0;
    }
    *value = (uint16_t)(bytes[offset] |
                        ((uint16_t)bytes[(uint16_t)(offset + 1u)] << 8));
    return 1;
}

static int on_far_byte(void *context, uint16_t segment, uint16_t offset,
                       uint8_t *value) {
    fixture *state = context;
    uint8_t *bytes = far_segment(state, segment);
    event(state, 'B');
    state->far_byte_reads++;
    if (bytes == NULL || state->far_byte_reads == state->fail_far_byte_at) {
        return 0;
    }
    *value = bytes[offset];
    return 1;
}

static pr_rec_party_count_io make_io(fixture *state, size_t max_nodes) {
    pr_rec_party_count_io io = {state, on_global_word, on_global_byte,
                                on_global_write, on_far_word, on_far_byte,
                                max_nodes};
    return io;
}

static int test_empty_and_skipped_character(void) {
    fixture state = {0};
    pr_rec_party_count_io io = make_io(&state, 2u);
    pr_rec_far_ptr first = ptr(0x0100u, 0x2000u);
    state.globals[0x6814u] = 8u;
    state.globals[0x6815u] = 9u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_OK);
    CHECK(state.globals[0x6814u] == 0u && state.globals[0x6815u] == 0u);
    CHECK(trace_is(&state, "WWHH"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 2u);
    set_head(&state, first);
    set_character(&state, first, 0u, 1u, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_OK);
    CHECK(state.globals[0x6814u] == 0u && state.globals[0x6815u] == 0u);
    CHECK(trace_is(&state, "WWHHBFF"));
    return 0;
}

static int test_mixed_party_and_byte_wrap(void) {
    fixture state = {0};
    pr_rec_party_count_io io = make_io(&state, 3u);
    pr_rec_far_ptr first = ptr(0x0100u, 0x2000u);
    pr_rec_far_ptr second = ptr(0x0200u, 0x2000u);
    pr_rec_far_ptr third = ptr(0x0300u, 0x3000u);
    set_head(&state, first);
    set_character(&state, first, 0u, 0u, second);
    set_character(&state, second, 1u, 0u, third);
    set_character(&state, third, 2u, 1u, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_OK);
    CHECK(state.globals[0x6814u] == 1u && state.globals[0x6815u] == 1u);
    CHECK(trace_is(&state, "WWHH" "BFF" "BBRWFF" "BBRWFF"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    set_head(&state, first);
    set_character(&state, first, 1u, 2u, ptr(0u, 0u));
    state.globals[0x6816u] = 0xFFu;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_OK);
    CHECK(state.globals[0x6814u] == 0u && state.globals[0x6815u] == 0u);
    CHECK(state.globals[0x6816u] == 0u);
    CHECK(trace_is(&state, "WWHHBBRWFF"));
    return 0;
}

static int test_signed_index_and_far_wrap(void) {
    fixture state = {0};
    pr_rec_party_count_io io = make_io(&state, 1u);
    pr_rec_far_ptr first = ptr(0xFF00u, 0x2000u);
    set_head(&state, first);
    set_character(&state, first, 1u, 0xFFu, ptr(0u, 0u));
    state.globals[0x6813u] = 6u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_OK);
    CHECK(state.globals[0x6813u] == 7u);
    CHECK(state.globals[0x6814u] == 0u && state.globals[0x6815u] == 0u);
    CHECK(trace_is(&state, "WWHHBBRWFF"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    first = ptr(0u, 0x2000u); /* one zero word remains non-null */
    set_head(&state, first);
    set_character(&state, first, 1u, 1u, ptr(0u, 0u));
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_OK);
    CHECK(state.globals[0x6815u] == 1u);
    return 0;
}

static int test_cycle_and_host_failures(void) {
    fixture state = {0};
    pr_rec_party_count_io io = make_io(&state, 2u);
    pr_rec_far_ptr first = ptr(0x0100u, 0x2000u);
    pr_rec_far_ptr second = ptr(0x0200u, 0x2000u);
    set_head(&state, first);
    set_character(&state, first, 1u, 0u, second);
    set_character(&state, second, 1u, 0u, first);
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_LIMIT);
    CHECK(state.globals[0x6814u] == 2u);
    CHECK(trace_is(&state, "WWHH" "BBRWFF" "BBRWFF"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    state.fail_global_write_at = 1u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_MEMORY);
    CHECK(trace_is(&state, "W"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    state.fail_global_word_at = 1u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_MEMORY);
    CHECK(trace_is(&state, "WWH"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    set_head(&state, first);
    state.fail_far_byte_at = 1u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_MEMORY);
    CHECK(trace_is(&state, "WWHHB"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    set_head(&state, first);
    set_character(&state, first, 1u, 0u, ptr(0u, 0u));
    state.fail_far_byte_at = 2u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_MEMORY);
    CHECK(trace_is(&state, "WWHHBB"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    set_head(&state, first);
    set_character(&state, first, 1u, 0u, ptr(0u, 0u));
    state.fail_global_byte_at = 1u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_MEMORY);
    CHECK(trace_is(&state, "WWHHBBR"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    set_head(&state, first);
    set_character(&state, first, 1u, 0u, ptr(0u, 0u));
    state.fail_global_write_at = 3u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_MEMORY);
    CHECK(state.globals[0x6814u] == 0u);
    CHECK(trace_is(&state, "WWHHBBRW"));

    memset(&state, 0, sizeof state);
    io = make_io(&state, 1u);
    set_head(&state, first);
    set_character(&state, first, 0u, 0u, ptr(0u, 0u));
    state.fail_far_word_at = 1u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_MEMORY);
    CHECK(trace_is(&state, "WWHHBF"));

    CHECK(pr_rec_res_00ba_23db(NULL) == PR_REC_PARTY_COUNT_ARGUMENT);
    io.max_nodes = 0u;
    CHECK(pr_rec_res_00ba_23db(&io) == PR_REC_PARTY_COUNT_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_empty_and_skipped_character() ||
        test_mixed_party_and_byte_wrap() ||
        test_signed_index_and_far_wrap() ||
        test_cycle_and_host_failures()) {
        return 1;
    }
    puts("resident party counter synthetic paths passed");
    return 0;
}
