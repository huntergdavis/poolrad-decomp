#ifndef POOLRAD_REPLAY_SLOT_A_IF_SKIP_ENTRY_H
#define POOLRAD_REPLAY_SLOT_A_IF_SKIP_ENTRY_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates five-byte call and stub references, keeps guest code
 * immutable, and supplies disjoint host objects. Refusal is atomic. */
int pr_slot_a_if_skip_entry(uint8_t *ram,size_t size,const uint8_t *call,const uint8_t *stub,pr_u1c_3171_cpu *cpu);
#endif
