#ifndef POOLRAD_REPLAY_SLOT_A_FIRST_EXIT_H
#define POOLRAD_REPLAY_SLOT_A_FIRST_EXIT_H

#include "poolrad/recovered/overlay_u1c_exit_0001.h"
#include "poolrad/replay/slot_a_ecl_live.h"

#include <stddef.h>
#include <stdint.h>

#define PR_SLOT_A_FIRST_EXIT_ENTRY 0x9914u
#define PR_SLOT_A_FIRST_EXIT_TRACE_MAX 16u

typedef struct {
    pr_slot_a_ecl_bind_status bind_status;
    pr_rec_ecl_run_status run_status;
    pr_rec_ecl_handler_status handler_status;
    unsigned steps;
    unsigned callback_count;
    uint16_t callback_pc[PR_SLOT_A_FIRST_EXIT_TRACE_MAX];
    unsigned exit_reached;
    pr_u1c_exit_status exit_status;
    pr_u1c_exit_report exit_report;
} pr_slot_a_first_exit_report;

/* Run only the saved Up entry 0x9914 with the already merged SAVE, COMPARE,
 * IF, GOTO, AND, and EXIT models. The live DS environment is read from the
 * supplied native RAM; no resident box hook is invented. Non-OK runner and
 * handler statuses stay in report unchanged. A binding error leaves state
 * and env unchanged. Caller retains RAM for the lifetime of state. This
 * does not run the gameplay caller after +0x3171 returns. */
pr_slot_a_ecl_bind_status pr_slot_a_first_exit_run_ram(
    const uint8_t *ram, size_t ram_size, uint16_t ds,
    unsigned max_steps, pr_rec_ecl_loader_state *state,
    pr_u1c_exit_env *env, pr_slot_a_first_exit_report *report);

#endif
