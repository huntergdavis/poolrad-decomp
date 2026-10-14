#ifndef POOLRAD_REPLAY_SLOT_A_CALLER_TO_ENTRY1_H
#define POOLRAD_REPLAY_SLOT_A_CALLER_TO_ENTRY1_H

#include <stddef.h>
#include <stdint.h>

/* Live caller state immediately after the +0x3458 script call returns.
 * The host must supply actual post-script fields. The step stops before
 * any reached callee at +0x3462 or +0x348D. */
typedef struct {
    uint8_t flag_442f;
    uint8_t flag_49ff;
    uint8_t party_x_6aad;
    uint8_t party_y_6aae;
    uint8_t *window0;
    size_t window0_size;
} pr_slot_a_caller_to_entry1_state;

typedef enum {
    PR_SLOT_A_CALLER_ENTRY1 = 0, /* next PC +0x348D; entry 1 unexecuted */
    PR_SLOT_A_CALLER_31B4,      /* next PC +0x3462; callee unexecuted */
    PR_SLOT_A_CALLER_TAIL,      /* next PC +0x34F5 */
    PR_SLOT_A_CALLER_ARGUMENT   /* invalid reached window access */
} pr_slot_a_caller_to_entry1_status;

typedef struct {
    uint16_t next_pc;
    unsigned window0_words_written;
} pr_slot_a_caller_to_entry1_report;

/* Execute the caller's +0x345B..+0x348C gate and two sign-extended writes.
 * A short window can stop after the first write, as in the original order.
 * report is required; it records the reached instruction on every return. */
pr_slot_a_caller_to_entry1_status pr_slot_a_caller_to_entry1(
    pr_slot_a_caller_to_entry1_state *state,
    pr_slot_a_caller_to_entry1_report *report);

#endif
