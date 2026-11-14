#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_RETURN_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_RETURN_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates the four-byte tail and supplies disjoint host objects.
 * Invalid inputs and wrapping stack-offset frames refuse atomically. */
int pr_slot_a_compare_return(const uint8_t *ram,size_t size,const uint8_t *tail,size_t tail_size,pr_u1c_3171_cpu *cpu);
#endif
