/* Synthetic paths for START.EXE RES_00BA_23B7. No DOS character bytes. */
#include "poolrad/recovered/resident_character_field_zero.h"

#include <stdio.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint16_t expected_segment;
    uint16_t expected_offset;
    uint8_t value;
    unsigned calls;
    int fail;
    int address_mismatch;
} fixture;

static int read_far_byte(void *context, uint16_t segment, uint16_t offset,
                         uint8_t *value) {
    fixture *state = context;
    state->calls++;
    if (segment != state->expected_segment ||
        offset != state->expected_offset) {
        state->address_mismatch = 1;
    }
    if (state->fail) return 0;
    *value = state->value;
    return 1;
}

static int test_result_paths(void) {
    fixture state = {0x2000u, 0x020Eu, 0u, 0u, 0, 0};
    pr_rec_character_field_zero_io io = {&state, read_far_byte};
    pr_rec_far_ptr character = {0x0100u, 0x2000u};
    uint8_t result = 0xA5u;

    CHECK(pr_rec_res_00ba_23b7(&io, character, &result) ==
          PR_REC_CHARACTER_FIELD_ZERO_OK);
    CHECK(result == 1u && state.calls == 1u && !state.address_mismatch);

    state.value = 1u;
    result = 0xA5u;
    CHECK(pr_rec_res_00ba_23b7(&io, character, &result) ==
          PR_REC_CHARACTER_FIELD_ZERO_OK);
    CHECK(result == 0u && state.calls == 2u && !state.address_mismatch);

    state.value = 0xFFu;
    result = 0xA5u;
    CHECK(pr_rec_res_00ba_23b7(&io, character, &result) ==
          PR_REC_CHARACTER_FIELD_ZERO_OK);
    CHECK(result == 0u && state.calls == 3u && !state.address_mismatch);
    return 0;
}

static int test_far_address_and_failure(void) {
    fixture state = {0u, 0x000Eu, 0u, 0u, 0, 0};
    pr_rec_character_field_zero_io io = {&state, read_far_byte};
    pr_rec_far_ptr character = {0xFF00u, 0u};
    uint8_t result = 0xA5u;

    /* Both words may be zero independently; DOS does not test the pointer. */
    CHECK(pr_rec_res_00ba_23b7(&io, character, &result) ==
          PR_REC_CHARACTER_FIELD_ZERO_OK);
    CHECK(result == 1u && state.calls == 1u && !state.address_mismatch);

    character.offset = 0u;
    character.segment = 0x3000u;
    state.expected_segment = 0x3000u;
    state.expected_offset = 0x010Eu;
    state.fail = 1;
    result = 0xA5u;
    CHECK(pr_rec_res_00ba_23b7(&io, character, &result) ==
          PR_REC_CHARACTER_FIELD_ZERO_MEMORY);
    CHECK(result == 0xA5u && state.calls == 2u && !state.address_mismatch);

    CHECK(pr_rec_res_00ba_23b7(NULL, character, &result) ==
          PR_REC_CHARACTER_FIELD_ZERO_ARGUMENT);
    CHECK(pr_rec_res_00ba_23b7(&io, character, NULL) ==
          PR_REC_CHARACTER_FIELD_ZERO_ARGUMENT);
    CHECK(state.calls == 2u);
    io.read_far_byte = NULL;
    CHECK(pr_rec_res_00ba_23b7(&io, character, &result) ==
          PR_REC_CHARACTER_FIELD_ZERO_ARGUMENT);
    CHECK(state.calls == 2u && result == 0xA5u);
    return 0;
}

int main(void) {
    if (test_result_paths() || test_far_address_and_failure()) return 1;
    puts("resident character field zero synthetic paths passed");
    return 0;
}
