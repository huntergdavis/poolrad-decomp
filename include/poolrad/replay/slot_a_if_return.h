#ifndef POOLRAD_REPLAY_SLOT_A_IF_RETURN_H
#define POOLRAD_REPLAY_SLOT_A_IF_RETURN_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates the two-byte tail, keeps guest code immutable and
 * supplies disjoint host objects. Invalid input refuses atomically; the
 * actual BP and far return destination are exposed. */
int pr_slot_a_if_return(const uint8_t *ram,size_t size,const uint8_t *tail,size_t tail_size,pr_u1c_3171_cpu *cpu);
#endif
