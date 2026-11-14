#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_LOADER_STUB_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_LOADER_STUB_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional loaded :002A far jump to offset zero. Caller authenticates the
 * five stub bytes and supplies disjoint host objects and full RAM. Invalid
 * inputs refuse without changes; success stops before the destination body. */
int pr_slot_a_save_loader_stub(const uint8_t *ram, size_t size,
    const uint8_t *stub, size_t stub_size, pr_u1c_3171_cpu *cpu);
#endif
