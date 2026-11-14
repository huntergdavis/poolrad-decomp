#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_HANDLER_PREFIX_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_HANDLER_PREFIX_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 0292..029A SAVE prefix, stopping before operand-loader LCALL.
 * Caller authenticates code and provides disjoint host objects and full RAM.
 * Invalid inputs refuse without CPU or RAM changes. */
int pr_slot_a_save_handler_prefix(uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu);
#endif
