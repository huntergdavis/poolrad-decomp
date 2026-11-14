#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_LOADER_CALL_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_LOADER_CALL_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 029B far call to a supplied loaded :002A stub. Caller
 * authenticates the five code bytes and supplies disjoint host objects and
 * full RAM. Code/stack aliasing is refused atomically. Stub unexecuted. */
int pr_slot_a_save_loader_call(uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu);
#endif
