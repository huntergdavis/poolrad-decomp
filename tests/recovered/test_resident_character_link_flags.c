/* Synthetic complete paths for START.EXE RES_00BA_2E46. No game media. */
#include "poolrad/recovered/resident_character_link_flags.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    pr_rec_far_ptr character, first_link, second_link, third_link;
    uint8_t first_index, second_index, first_table, second_table;
    unsigned word_reads, far_byte_reads, global_reads;
    unsigned fail_word_at, fail_far_byte_at, fail_global_at;
    int address_mismatch;
    char trace[20];
    size_t trace_length;
} fixture;

static void event(fixture *state, char code) {
    if (state->trace_length + 1u < sizeof state->trace) {
        state->trace[state->trace_length++] = code;
        state->trace[state->trace_length] = '\0';
    }
}

static pr_rec_far_ptr link_for_read(const fixture *state, unsigned call) {
    if (call <= 2u) return state->first_link;
    if (call <= 4u) return state->second_link;
    return state->third_link;
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
    link = link_for_read(state, state->word_reads);
    *value = (state->word_reads & 1u) ? link.offset : link.segment;
    return 1;
}

static int read_far_byte(void *context, uint16_t segment, uint16_t offset,
                         uint8_t *value) {
    fixture *state = context;
    pr_rec_far_ptr link;
    state->far_byte_reads++;
    event(state, 'B');
    link = state->far_byte_reads == 1u ? state->second_link : state->third_link;
    if (segment != link.segment ||
        offset != (uint16_t)(link.offset + 0x2Eu)) {
        state->address_mismatch = 1;
    }
    if (state->far_byte_reads == state->fail_far_byte_at) return 0;
    *value = state->far_byte_reads == 1u ? state->first_index :
             state->second_index;
    return 1;
}

static int read_global_byte(void *context, uint16_t offset, uint8_t *value) {
    fixture *state = context;
    uint8_t index;
    uint16_t base;
    state->global_reads++;
    event(state, 'G');
    index = state->global_reads == 1u ? state->first_index :
            state->second_index;
    base = state->global_reads == 1u ? 0x558Eu : 0x5590u;
    if (offset != (uint16_t)(base + ((uint16_t)index << 4))) {
        state->address_mismatch = 1;
    }
    if (state->global_reads == state->fail_global_at) return 0;
    *value = state->global_reads == 1u ? state->first_table :
             state->second_table;
    return 1;
}

static fixture make_fixture(void) {
    fixture state = {0};
    state.character.offset = 0x0100u;
    state.character.segment = 0x2000u;
    state.first_link.offset = 0x0200u;
    state.first_link.segment = 0x3000u;
    state.second_link = state.first_link;
    state.third_link = state.first_link;
    state.first_index = 3u;
    state.second_index = 3u;
    state.first_table = 2u;
    state.second_table = 0x14u;
    return state;
}

static pr_rec_character_link_check_io make_io(fixture *state) {
    pr_rec_character_link_check_io io = {
        state, read_far_word, read_far_byte, read_global_byte
    };
    return io;
}

static int test_short_circuit_paths(void) {
    fixture state = make_fixture();
    pr_rec_character_link_check_io io = make_io(&state);
    uint8_t result = 0xA5u;
    state.first_link.offset = 0u;
    state.first_link.segment = 0u;
    CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 0u && strcmp(state.trace, "WW") == 0);
    CHECK(state.word_reads == 2u && state.far_byte_reads == 0u &&
          state.global_reads == 0u && !state.address_mismatch);

    state = make_fixture();
    io = make_io(&state);
    state.first_table = 1u;
    result = 0xA5u;
    CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 0u && strcmp(state.trace, "WWWWBG") == 0);
    CHECK(state.word_reads == 4u && state.far_byte_reads == 1u &&
          state.global_reads == 1u && !state.address_mismatch);
    return 0;
}

static int test_final_mask_and_repeated_reads(void) {
    fixture state = make_fixture();
    pr_rec_character_link_check_io io = make_io(&state);
    uint8_t result = 0xA5u;
    CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 1u && strcmp(state.trace, "WWWWBGWWBG") == 0);
    CHECK(state.word_reads == 6u && state.far_byte_reads == 2u &&
          state.global_reads == 2u && !state.address_mismatch);

    state = make_fixture();
    io = make_io(&state);
    state.second_table = 0x10u; /* one mask bit is insufficient */
    CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 0u && !state.address_mismatch);

    state = make_fixture();
    io = make_io(&state);
    state.second_table = 0x04u;
    CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 0u && !state.address_mismatch);

    state = make_fixture();
    io = make_io(&state);
    state.character.offset = 0xFF40u; /* +0xCC/+0xCE wrap */
    state.first_link.offset = 0u; /* segment-only non-null */
    state.second_link.offset = 0xFFEAu; /* +0x2E wraps */
    state.second_link.segment = 0x4000u;
    state.third_link.offset = 0xFEEAu;
    state.third_link.segment = 0x5000u;
    state.first_index = 3u;
    state.second_index = 0xFFu;
    state.second_table = 0x15u; /* extra bits do not change equality */
    CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_OK);
    CHECK(result == 1u && strcmp(state.trace, "WWWWBGWWBG") == 0);
    CHECK(!state.address_mismatch);
    return 0;
}

static int test_host_failures(void) {
    fixture state;
    pr_rec_character_link_check_io io;
    uint8_t result;
    unsigned fail_at;
    for (fail_at = 1u; fail_at <= 6u; fail_at++) {
        state = make_fixture();
        io = make_io(&state);
        state.fail_word_at = fail_at;
        result = 0xA5u;
        CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
              PR_REC_CHARACTER_LINK_CHECK_MEMORY);
        CHECK(result == 0xA5u && state.word_reads == fail_at);
        CHECK(!state.address_mismatch);
    }
    for (fail_at = 1u; fail_at <= 2u; fail_at++) {
        state = make_fixture();
        io = make_io(&state);
        state.fail_far_byte_at = fail_at;
        result = 0xA5u;
        CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
              PR_REC_CHARACTER_LINK_CHECK_MEMORY);
        CHECK(result == 0xA5u && state.far_byte_reads == fail_at);
        CHECK(!state.address_mismatch);
    }
    for (fail_at = 1u; fail_at <= 2u; fail_at++) {
        state = make_fixture();
        io = make_io(&state);
        state.fail_global_at = fail_at;
        result = 0xA5u;
        CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
              PR_REC_CHARACTER_LINK_CHECK_MEMORY);
        CHECK(result == 0xA5u && state.global_reads == fail_at);
        CHECK(!state.address_mismatch);
    }

    state = make_fixture();
    io = make_io(&state);
    result = 0xA5u;
    CHECK(pr_rec_res_00ba_2e46(NULL, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_ARGUMENT);
    CHECK(pr_rec_res_00ba_2e46(&io, state.character, NULL) ==
          PR_REC_CHARACTER_LINK_CHECK_ARGUMENT);
    io.read_global_byte = NULL;
    CHECK(pr_rec_res_00ba_2e46(&io, state.character, &result) ==
          PR_REC_CHARACTER_LINK_CHECK_ARGUMENT);
    CHECK(result == 0xA5u && state.word_reads == 0u);
    return 0;
}

int main(void) {
    if (test_short_circuit_paths() || test_final_mask_and_repeated_reads() ||
        test_host_failures()) return 1;
    puts("resident character link flags synthetic paths passed");
    return 0;
}
