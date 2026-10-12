#include "poolrad/replay/slot_a_native_save.h"

#include "poolrad/formats/dos_savgam.h"

#include <string.h>

static uint16_t read_word(const uint8_t *bytes) {
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

pr_slot_a_native_status pr_slot_a_native_restore(const uint8_t *data,
                                                  size_t size,
                                                  pr_slot_a_native_state *out) {
    static const size_t widths[PR_DOS_SAVGAM_BLOCK_COUNT] = {
        1u, PR_SLOT_A_WINDOW0_SIZE, PR_SLOT_A_WINDOW1_SIZE,
        PR_SLOT_A_WINDOW2_SIZE, PR_SLOT_A_SCRIPT_SIZE,
        PR_SLOT_A_PARTY_SIZE, 1u, 1u, 1u, PR_SLOT_A_NAMES_SIZE
    };
    pr_dos_savgam_record record;
    pr_dos_savgam_core core;
    pr_dos_savgam_span span;
    pr_slot_a_native_state result;
    uint8_t *destinations[PR_DOS_SAVGAM_BLOCK_COUNT];
    size_t i;

    if (!data || !out) return PR_SLOT_A_NATIVE_ARGUMENT;
    if (pr_dos_savgam_open(data, size, &record) != PR_DOS_SAVGAM_OK)
        return PR_SLOT_A_NATIVE_BAD_FRAME;
    if (pr_dos_savgam_read_core(&record, &core) != PR_DOS_SAVGAM_OK)
        return PR_SLOT_A_NATIVE_LAYOUT;

    memset(&result, 0, sizeof result);
    destinations[0] = &result.initial_ecl_file;
    destinations[1] = result.window0;
    destinations[2] = result.window1;
    destinations[3] = result.window2;
    destinations[4] = result.script;
    destinations[5] = result.party;
    destinations[6] = &result.post_load_selector;
    destinations[7] = &result.mode;
    destinations[8] = &result.character_count;
    destinations[9] = result.name_slots;
    for (i = 0; i < PR_DOS_SAVGAM_BLOCK_COUNT; ++i) {
        if (pr_dos_savgam_block(&record, i, &span) != PR_DOS_SAVGAM_OK ||
            span.end_offset < span.source_offset ||
            span.end_offset - span.source_offset != widths[i])
            return PR_SLOT_A_NATIVE_LAYOUT;
        memcpy(destinations[i], data + span.source_offset, widths[i]);
    }

    if (result.initial_ecl_file != core.saved_ecl_file ||
        result.window1[0x624u] != core.restored_ecl_file ||
        result.window0[0x1E4u] != core.map_id ||
        result.party[0] != core.x || result.party[1] != core.y ||
        result.party[2] != core.facing || result.mode != core.mode ||
        result.character_count != core.character_count)
        return PR_SLOT_A_NATIVE_LAYOUT;
    result.restored_ecl_file = core.restored_ecl_file;
    result.map_id = core.map_id;
    *out = result;
    return PR_SLOT_A_NATIVE_OK;
}

pr_slot_a_native_status pr_slot_a_native_read_snapshot(
    const pr_slot_a_native_state *state, pr_slot_a_native_snapshot *out) {
    pr_slot_a_native_snapshot result;
    if (!state || !out) return PR_SLOT_A_NATIVE_ARGUMENT;
    result.x = state->party[0];
    result.y = state->party[1];
    result.facing = state->party[2];
    result.minute = read_word(state->window0 + PR_SLOT_A_CLOCK_MINUTE_OFFSET);
    result.tens = read_word(state->window0 + PR_SLOT_A_CLOCK_TENS_OFFSET);
    result.hour = read_word(state->window0 + PR_SLOT_A_CLOCK_HOUR_OFFSET);
    *out = result;
    return PR_SLOT_A_NATIVE_OK;
}

pr_slot_a_native_status pr_slot_a_native_one_up(pr_slot_a_native_state *state) {
    if (!state) return PR_SLOT_A_NATIVE_ARGUMENT;
    return PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH;
}

const char *pr_slot_a_native_status_string(pr_slot_a_native_status status) {
    switch (status) {
    case PR_SLOT_A_NATIVE_OK: return "ok";
    case PR_SLOT_A_NATIVE_ARGUMENT: return "invalid argument";
    case PR_SLOT_A_NATIVE_BAD_FRAME: return "invalid DOS world-save frame";
    case PR_SLOT_A_NATIVE_LAYOUT: return "world-save transfer layout mismatch";
    case PR_SLOT_A_NATIVE_BLOCKED_KEY_DISPATCH:
        return "blocked at unit 0x1c:+0x3395 input loop requiring unit 0x68 entry 2 +0x0989";
    default: return "unknown status";
    }
}
