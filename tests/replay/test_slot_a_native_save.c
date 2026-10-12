#include "poolrad/replay/slot_a_native_save.h"

#include "poolrad/formats/dos_savgam.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    const uint8_t *destination;
    size_t source_offset;
    size_t size;
} expected_transfer;

static int test_ten_transfers_and_snapshot(void) {
    uint8_t file_data[PR_DOS_SAVGAM_SIZE];
    pr_slot_a_native_state state, saved;
    pr_slot_a_native_snapshot snapshot;
    expected_transfer transfers[PR_DOS_SAVGAM_BLOCK_COUNT];
    size_t i;

    for (i = 0; i < sizeof file_data; ++i)
        file_data[i] = (uint8_t)(i * 37u + 11u);
    file_data[0x0000u] = 0x11u;
    file_data[0x01E5u] = 0x2Bu;
    file_data[0x0E25u] = 0x22u;
    file_data[0x0001u + PR_SLOT_A_CLOCK_MINUTE_OFFSET] = 0x34u;
    file_data[0x0001u + PR_SLOT_A_CLOCK_MINUTE_OFFSET + 1u] = 0x12u;
    file_data[0x0001u + PR_SLOT_A_CLOCK_TENS_OFFSET] = 0x67u;
    file_data[0x0001u + PR_SLOT_A_CLOCK_TENS_OFFSET + 1u] = 0x05u;
    file_data[0x0001u + PR_SLOT_A_CLOCK_HOUR_OFFSET] = 0x08u;
    file_data[0x0001u + PR_SLOT_A_CLOCK_HOUR_OFFSET + 1u] = 0x09u;
    file_data[0x3201u] = 9u;
    file_data[0x3202u] = 3u;
    file_data[0x3203u] = 2u;
    file_data[0x3206u] = 7u;
    file_data[0x3207u] = 4u;
    file_data[0x3208u] = 5u;

    CHECK(pr_slot_a_native_restore(file_data, sizeof file_data, &state) ==
          PR_SLOT_A_NATIVE_OK);
    transfers[0] = (expected_transfer){&state.initial_ecl_file, 0x0000u, 1u};
    transfers[1] = (expected_transfer){state.window0, 0x0001u, 0x800u};
    transfers[2] = (expected_transfer){state.window1, 0x0801u, 0x800u};
    transfers[3] = (expected_transfer){state.window2, 0x1001u, 0x400u};
    transfers[4] = (expected_transfer){state.script, 0x1401u, 0x1E00u};
    transfers[5] = (expected_transfer){state.party, 0x3201u, 5u};
    transfers[6] = (expected_transfer){&state.post_load_selector, 0x3206u, 1u};
    transfers[7] = (expected_transfer){&state.mode, 0x3207u, 1u};
    transfers[8] = (expected_transfer){&state.character_count, 0x3208u, 1u};
    transfers[9] = (expected_transfer){state.name_slots, 0x3209u, 0x148u};
    for (i = 0; i < PR_DOS_SAVGAM_BLOCK_COUNT; ++i)
        CHECK(memcmp(transfers[i].destination,
                     file_data + transfers[i].source_offset,
                     transfers[i].size) == 0);
    CHECK(state.initial_ecl_file == 0x11u);
    CHECK(state.restored_ecl_file == 0x22u);
    CHECK(state.map_id == 0x2Bu);
    CHECK(state.post_load_selector == 7u && state.mode == 4u);
    CHECK(state.character_count == 5u);

    CHECK(pr_slot_a_native_read_snapshot(&state, &snapshot) ==
          PR_SLOT_A_NATIVE_OK);
    CHECK(snapshot.x == 9u && snapshot.y == 3u && snapshot.facing == 2u);
    CHECK(snapshot.minute == 0x1234u && snapshot.tens == 0x0567u &&
          snapshot.hour == 0x0908u);

    saved = state;
    memset(file_data, 0, sizeof file_data);
    CHECK(memcmp(&state, &saved, sizeof state) == 0);
    CHECK(pr_slot_a_native_one_up(&state) ==
          PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH);
    CHECK(memcmp(&state, &saved, sizeof state) == 0);
    return 0;
}

static int test_rejected_inputs_do_not_change_output(void) {
    uint8_t file_data[PR_DOS_SAVGAM_SIZE] = {0};
    pr_slot_a_native_state state, saved;
    pr_slot_a_native_snapshot snapshot = {1u, 2u, 3u, 4u, 5u, 6u};
    pr_slot_a_native_snapshot old_snapshot = snapshot;
    memset(&state, 0xA5, sizeof state);
    saved = state;
    CHECK(pr_slot_a_native_restore(file_data, sizeof file_data - 1u, &state) ==
          PR_SLOT_A_NATIVE_BAD_FRAME);
    CHECK(memcmp(&state, &saved, sizeof state) == 0);
    CHECK(pr_slot_a_native_restore(NULL, sizeof file_data, &state) ==
          PR_SLOT_A_NATIVE_ARGUMENT);
    CHECK(memcmp(&state, &saved, sizeof state) == 0);
    CHECK(pr_slot_a_native_restore(file_data, sizeof file_data, NULL) ==
          PR_SLOT_A_NATIVE_ARGUMENT);
    CHECK(pr_slot_a_native_read_snapshot(NULL, &snapshot) ==
          PR_SLOT_A_NATIVE_ARGUMENT);
    CHECK(memcmp(&snapshot, &old_snapshot, sizeof snapshot) == 0);
    CHECK(pr_slot_a_native_read_snapshot(&state, NULL) ==
          PR_SLOT_A_NATIVE_ARGUMENT);
    CHECK(pr_slot_a_native_one_up(NULL) == PR_SLOT_A_NATIVE_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_ten_transfers_and_snapshot() ||
        test_rejected_inputs_do_not_change_output()) return 1;
    puts("synthetic slot-A save adapter and explicit Up blocker passed");
    return 0;
}
