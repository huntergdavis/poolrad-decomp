#ifndef POOLRAD_REPLAY_SLOT_A_SKIP_SELECTION_H
#define POOLRAD_REPLAY_SLOT_A_SKIP_SELECTION_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates 102-byte code reference, immutable guest code and
 * disjoint host objects. Unsupported opcode or input refuses atomically. */
int pr_slot_a_skip_selection(const uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
