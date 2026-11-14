#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_DISPATCH_RETURN_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_DISPATCH_RETURN_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional SAVE dispatcher 2F89 jump and 316F..3170 return. Caller
 * authenticates code and supplies disjoint host objects and full RAM.
 * SP above FFFA or invalid inputs refuse atomically; actual return is exposed. */
int pr_slot_a_save_dispatch_return(const uint8_t *ram,size_t size,const uint8_t *jump,
 size_t jump_size,const uint8_t *tail,size_t tail_size,pr_u1c_3171_cpu *cpu);
#endif
