#ifndef POOLRAD_REPLAY_SLOT_A_IF_LATER_DISPATCH_H
#define POOLRAD_REPLAY_SLOT_A_IF_LATER_DISPATCH_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates the 149-byte code reference and supplies disjoint
 * host objects and immutable guest code. Unsupported inputs refuse atomically. */
int pr_slot_a_if_later_dispatch(const uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
