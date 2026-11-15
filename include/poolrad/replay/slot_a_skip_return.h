#ifndef POOLRAD_REPLAY_SLOT_A_SKIP_RETURN_H
#define POOLRAD_REPLAY_SLOT_A_SKIP_RETURN_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates three-byte jump and two-byte tail, with immutable
 * code and disjoint host objects. Invalid frames refuse atomically; the
 * actual BP and far return destination are exposed. */
int pr_slot_a_skip_return(const uint8_t *ram,size_t size,const uint8_t *jump,size_t jump_size,const uint8_t *tail,size_t tail_size,pr_u1c_3171_cpu *cpu);
#endif
