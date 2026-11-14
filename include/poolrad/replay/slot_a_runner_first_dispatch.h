#ifndef POOLRAD_REPLAY_SLOT_A_RUNNER_FIRST_DISPATCH_H
#define POOLRAD_REPLAY_SLOT_A_RUNNER_FIRST_DISPATCH_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 3171 CPU slice; caller authenticates code and supplies 1 MiB RAM.
 * Stops after the dispatcher call frame is pushed, before dispatcher effects.
 * Objects must not overlap. Refusal leaves CPU and RAM unchanged. */
typedef enum { PR_SLOT_A_RUNNER_FIRST_DISPATCH = 0, PR_SLOT_A_RUNNER_REFUSED = 1 } pr_slot_a_runner_prefix_status;
pr_slot_a_runner_prefix_status pr_slot_a_runner_first_dispatch(
    uint8_t *ram, size_t size, const uint8_t *code, size_t code_size,
    pr_u1c_3171_cpu *cpu);
#endif
