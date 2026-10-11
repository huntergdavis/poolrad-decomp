/* Synthetic branches for START.EXE RES_00BA_2E03; no DOS character data. */
#include "poolrad/recovered/resident_character_link_check.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    pr_rec_far_ptr character, first_link, second_link;
    uint8_t raw_index, table_value;
    unsigned word_reads, far_byte_reads, global_reads, fail_word_at;
    int fail_far_byte, fail_global, address_mismatch;
    char trace[16];
    size_t trace_length;
} fixture;

static void event(fixture *state, char code) {
    if (state->trace_length + 1u < sizeof state->trace) {
        state->trace[state->trace_length++] = code;
        state->trace[state->trace_length] = '\0';
    }
}

static int read_far_word(void *context, uint16_t segment, uint16_t offset,
                         uint16_t *value) {
    fixture *state = context;
    pr_rec_far_ptr link;
    uint16_t expected;
    state->word_reads++;
    event(state, 'W');
    expected = (uint16_t)(state->character.offset +
                          ((state->word_reads & 1u) ? 0xCCu : 0xCEu));
    if (segment != state->character.segment || offset != expected) {
        state->address_mismatch = 1;
    }
    if (state->word_reads == state->fail_word_at) return 0;
    link = state->word_reads <= 2u ? state->first_link : state->second_link;
    *value = (state->word_reads & 1u) ? link.offset : link.segment;
    return 1;
}

static int read_far_byte(void *context, uint16_t segment, uint16_t offset,
                         uint8_t *value) {
    fixture *state = context;
    state->far_byte_reads++;
    event(state, 'B');
    if (segment != state->second_link.segment ||
        offset != (uint16_t)(state->second_link.offset + 0x2Eu)) {
        state->address_mismatch = 1;
    }
    if (state->fail_far_byte) return 0;
    *value = state->raw_index;
    return 1;
}

static int read_global_byte(void *context, uint16_t offset, uint8_t *value) {
    fixture *state = context;
    state->global_reads++;
    event(state, 'G');
    if (offset != (uint16_t)(0x558Eu +
                             ((uint16_t)state->raw_index << 4))) {
        state->address_mismatch = 1;
    }
    if (state->fail_global) return 0;
    *value = state->table_value;
    return 1;
}

static pr_rec_character_link_check_io make_io(fixture *state) {
    pr_rec_character_link_check_io io = {
        state, read_far_word, read_far_byte, read_global_byte
    };
    return io;
}

static fixture make_fixture(void) {
    fixture state = {0};
    state.character.offset = 0x0100u;
    state.character.segment = 0x2000u;
    state.first_link.offset = 0x0200u;
    state.first_link.segment = 0x3000u;
    state.second_link = state.first_link;
    state.raw_index = 3u;
    state.table_value = 2u;
    return state;
}

static int test_null_and_table_branches(void) {
    fixture state = make_fixture();
    pr_rec_character_link_check_io io = make_io(&state);
    uint8_t result = 0xA5u;
    state.first_link.offset = 0u;
    state.first_link.segment = 0u;
    CHECK(pr_rec_res_00ba_2e03(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 0u && strcmp(state.trace, "WW") == 0);
    CHECK(state.word_reads == 2u && state.far_byte_reads == 0u &&
          state.global_reads == 0u && !state.address_mismatch);

    state = make_fixture();
    io = make_io(&state);
    state.first_link.offset = 0u; /* high word alone is non-null */
    state.table_value = 1u;
    CHECK(pr_rec_res_00ba_2e03(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 0u && strcmp(state.trace, "WWWWBG") == 0);
    CHECK(state.word_reads == 4u && state.far_byte_reads == 1u &&
          state.global_reads == 1u && !state.address_mismatch);

    state = make_fixture();
    io = make_io(&state);
    state.first_link.segment = 0u; /* low word alone is non-null */
    state.table_value = 0u;
    CHECK(pr_rec_res_00ba_2e03(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 0u && !state.address_mismatch);

    state = make_fixture();
    io = make_io(&state);
    state.table_value = 2u;
    CHECK(pr_rec_res_00ba_2e03(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 1u && strcmp(state.trace, "WWWWBG") == 0);
    CHECK(!state.address_mismatch);
    return 0;
}

static int test_reread_and_wrapped_offsets(void) {
    fixture state = make_fixture();
    pr_rec_character_link_check_io io = make_io(&state);
    uint8_t result = 0xA5u;
    state.character.offset = 0xFF40u; /* +0xCC/+0xCE wrap */
    state.second_link.offset = 0xFFEAu; /* +0x2E wraps */
    state.second_link.segment = 0x4000u;
    state.raw_index = 0xFFu; /* table address is 0x657E */
    state.table_value = 0xFFu;
    CHECK(pr_rec_res_00ba_2e03(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 1u && strcmp(state.trace, "WWWWBG") == 0);
    CHECK(state.word_reads == 4u && state.far_byte_reads == 1u &&
          state.global_reads == 1u && !state.address_mismatch);
    return 0;
}

static int test_host_failures(void) {
    fixture state;
    pr_rec_character_link_check_io io;
    uint8_t result;
    unsigned fail_at;
    for (fail_at = 1u; fail_at <= 4u; fail_at++) {
        state = make_fixture();
        io = make_io(&state);
        state.fail_word_at = fail_at;
        result = 0xA5u;
        CHECK(pr_rec_res_00ba_2e03(&io, state.character, &result) ==
              PR_REC_CHARACTER_LINK_CHECK_MEMORY);
        CHECK(result == 0xA5u && state.word_reads == fail_at);
        CHECK(state.far_byte_reads == 0u && state.global_reads == 0u);
        CHECK(!state.address_mismatch);
    }

    state = make_fixture();
    io = make_io(&state);
    state.fail_far_byte = 1;
    result = 0xA5u;
    CHECK(pr_rec_res_00ba_2e03(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_MEMORY);
    CHECK(result == 0xA5u && strcmp(state.trace, "WWWWB") == 0);

    state = make_fixture();
    io = make_io(&state);
    state.fail_global = 1;
    result = 0xA5u;
    CHECK(pr_rec_res_00ba_2e03(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_MEMORY);
    CHECK(result == 0xA5u && strcmp(state.trace, "WWWWBG") == 0);

    state = make_fixture();
    io = make_io(&state);
    CHECK(pr_rec_res_00ba_2e03(NULL, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_ARGUMENT);
    CHECK(pr_rec_res_00ba_2e03(&io, state.character, NULL) ==
          PR_REC_CHARACTER_LINK_CHECK_ARGUMENT);
    io.read_far_word = NULL;
    CHECK(pr_rec_res_00ba_2e03(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_ARGUMENT);
    CHECK(state.word_reads == 0u);
    return 0;
}

int main(void) {
    if (test_null_and_table_branches() ||
        test_reread_and_wrapped_offsets() || test_host_failures()) return 1;
    puts("resident character link predicate synthetic paths passed");
    return 0;
}
