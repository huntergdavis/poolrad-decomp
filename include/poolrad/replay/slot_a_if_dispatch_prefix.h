#ifndef POOLRAD_REPLAY_SLOT_A_IF_DISPATCH_PREFIX_H
#define POOLRAD_REPLAY_SLOT_A_IF_DISPATCH_PREFIX_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Authenticated prologue(6), first branch(4), and selector(78) spans;
 * disjoint host objects and immutable guest code are caller requirements.
 * Returns the first refused/reached IP; earlier effects remain. */
uint16_t pr_slot_a_if_dispatch_prefix(uint8_t *ram,size_t size,pr_u1c_3171_cpu *cpu,const uint8_t *prologue,const uint8_t *first,const uint8_t *selector);
#endif
