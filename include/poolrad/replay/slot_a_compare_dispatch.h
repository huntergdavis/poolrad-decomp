#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_DISPATCH_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_DISPATCH_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 3582:31A4 dispatcher frame through AL=03 selection at 2F58.
 * Caller authenticates fixed-size references, supplies disjoint host objects,
 * full RAM and immutable code. Returns first refused/reached offset; prior
 * effects remain. No COMPARE call or handler executes. */
uint16_t pr_slot_a_compare_dispatch(uint8_t *ram,size_t size,pr_u1c_3171_cpu *cpu,
 const uint8_t *prologue,const uint8_t *first_branch,const uint8_t *select);
#endif
