#ifndef POOLRAD_REPLAY_SLOT_A_KEY_EXIT_HANDOFF_H
#define POOLRAD_REPLAY_SLOT_A_KEY_EXIT_HANDOFF_H

#include "poolrad/replay/slot_a_first_exit.h"
#include "poolrad/replay/slot_a_key_script_join.h"

/* A caller-supplied live +3171 entry image and the reached call frame. The
 * caller must establish that the image and dependency are from the same
 * invocation; the checks below do not establish provenance. The RAM must
 * outlive the returned ECL state, and all objects must be nonoverlapping. */
typedef struct {
    const uint8_t *ram;
    size_t ram_size;
    uint16_t ds, ss, sp, caller_cs;
} pr_slot_a_script_entry_image;

typedef enum {
    PR_SLOT_A_KEY_EXIT_PROJECTED_RETURN = 0,
    PR_SLOT_A_KEY_EXIT_ARGUMENT,
    PR_SLOT_A_KEY_EXIT_DEPENDENCY,
    PR_SLOT_A_KEY_EXIT_FRAME,
    PR_SLOT_A_KEY_EXIT_BIND,
    PR_SLOT_A_KEY_EXIT_BLOCKED
} pr_slot_a_key_exit_status;

typedef struct {
    unsigned runner_called;
    uint16_t projected_return_pc;
    pr_slot_a_first_exit_report script;
} pr_slot_a_key_exit_report;

/* Consume the Up-gate dependency only with matching DS entry and stack words
 * (return IP, caller CS, argument), then run the existing first-EXIT model.
 * The RAM is read-only; state/env are projected host outputs, not writes to
 * guest memory or a raw CPU return. Binder/runner/handler detail remains in
 * report. On pre-run rejection state/env are untouched; a blocked runner may
 * publish its partial state. The caller must not infer same-run or full-Up
 * parity from matching fields or a projected return. */
pr_slot_a_key_exit_status pr_slot_a_key_exit_handoff(
    const pr_slot_a_script_dependency *dependency,
    const pr_slot_a_script_entry_image *entry,
    unsigned max_steps, pr_rec_ecl_loader_state *state,
    pr_u1c_exit_env *env, pr_slot_a_key_exit_report *report);

#endif
