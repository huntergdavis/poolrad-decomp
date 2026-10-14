#ifndef POOLRAD_REPLAY_SLOT_A_KEY_SCRIPT_JOIN_H
#define POOLRAD_REPLAY_SLOT_A_KEY_SCRIPT_JOIN_H

#include "poolrad/replay/slot_a_key_dispatch_bridge.h"

/* Dependency at the reached native near-call instruction. This is a call
 * request, never a claim that the script or its caller has completed. */
typedef struct {
    uint16_t unit;
    uint16_t call_pc;
    uint16_t callee_pc;
    uint16_t return_pc;
    uint16_t entry;
} pr_slot_a_script_dependency;

typedef struct {
    const uint8_t *window1; /* same live invocation as the return state */
    size_t window1_size;
    uint8_t exit_flag_49ff;
    uint16_t script_entry_49e3;
} pr_slot_a_key_join_gate;

typedef struct {
    unsigned return_applied;
    uint16_t next_pc;
    unsigned dependency_valid;
    pr_slot_a_script_dependency dependency;
} pr_slot_a_key_join_report;

typedef enum {
    PR_SLOT_A_KEY_JOIN_BLOCKED_SCRIPT_3171 = 0,
    PR_SLOT_A_KEY_JOIN_ARGUMENT,
    PR_SLOT_A_KEY_JOIN_OUTSIDE_SLICE
} pr_slot_a_key_join_status;

/* Compose the existing +339A return step and +33BD Up-gate query. Input
 * returned_al must come from the actual unit-0x68 entry-2 return. The gate
 * consumes the newly stored local key; no second host key is accepted.
 *
 * All buffers/fields must describe that live invocation, after loader and
 * gameplay initialization. Raw save transfers alone do not satisfy this
 * contract. Objects must not overlap. The caller owns the live buffers.
 *
 * This is a state projection, not a CPU/stack emulator. next_pc is a reached
 * boundary only when dependency_valid is set; on gate rejection it remains
 * +33BD (start of the unsupported query). Earlier return effects persist if
 * the later gate rejects its window or route. NULL top-level arguments have
 * no effect. A valid report is initialized on every other return. */
pr_slot_a_key_join_status pr_slot_a_key_script_join(
    pr_slot_a_key_return_state *returned, uint8_t returned_al,
    const pr_slot_a_key_join_gate *gate, pr_slot_a_key_join_report *report);

#endif
