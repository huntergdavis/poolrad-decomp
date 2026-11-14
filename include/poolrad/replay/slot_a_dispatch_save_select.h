#ifndef POOLRAD_REPLAY_SLOT_A_DISPATCH_SAVE_SELECT_H
#define POOLRAD_REPLAY_SLOT_A_DISPATCH_SAVE_SELECT_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional AL=09 dispatcher comparisons from 2F3E through 2F84.
 * Caller authenticates code and supplies disjoint host objects and full RAM.
 * Refusal is atomic; success stops before SAVE call setup at 2F85. */
int pr_slot_a_dispatch_save_select(const uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu);
#endif
