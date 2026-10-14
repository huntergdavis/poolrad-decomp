#include "poolrad/replay/slot_a_caller_to_entry1.h"

static void put_word(uint8_t *bytes, size_t off, uint16_t word)
{
    bytes[off] = (uint8_t)word;
    bytes[off + 1u] = (uint8_t)(word >> 8);
}

static uint16_t sign_extend_byte(uint8_t byte)
{
    return (uint16_t)(byte < 0x80u ? byte : 0xFF00u | byte);
}

pr_slot_a_caller_to_entry1_status pr_slot_a_caller_to_entry1(
    pr_slot_a_caller_to_entry1_state *state,
    pr_slot_a_caller_to_entry1_report *report)
{
    if (!state || !report)
        return PR_SLOT_A_CALLER_ARGUMENT;
    report->next_pc = 0x345Bu;
    report->window0_words_written = 0u;

    if (state->flag_442f != 0u) {
        report->next_pc = 0x3462u;
        return PR_SLOT_A_CALLER_31B4;
    }
    if (state->flag_49ff != 0u) {
        report->next_pc = 0x34F5u;
        return PR_SLOT_A_CALLER_TAIL;
    }

    report->next_pc = 0x347Bu;
    if (!state->window0 || state->window0_size < 0x1E2u)
        return PR_SLOT_A_CALLER_ARGUMENT;
    put_word(state->window0, 0x1E0u, sign_extend_byte(state->party_x_6aad));
    report->window0_words_written = 1u;

    report->next_pc = 0x3488u;
    if (state->window0_size < 0x1E4u)
        return PR_SLOT_A_CALLER_ARGUMENT;
    put_word(state->window0, 0x1E2u, sign_extend_byte(state->party_y_6aae));
    report->window0_words_written = 2u;
    report->next_pc = 0x348Du;
    return PR_SLOT_A_CALLER_ENTRY1;
}
