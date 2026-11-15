#ifndef POOLRAD_REPLAY_SLOT_A_AND_DISPATCH_H
#define POOLRAD_REPLAY_SLOT_A_AND_DISPATCH_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Selector 437 bytes and call 4 bytes are independently authenticated.
 * Guest code is immutable; host objects are disjoint. Returns reached IP. */
uint16_t pr_slot_a_and_dispatch(uint8_t *ram,size_t size,const uint8_t *selector,const uint8_t *call,pr_u1c_3171_cpu *cpu);
#endif
