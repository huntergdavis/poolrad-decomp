#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_STORE_ARGS_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_STORE_ARGS_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional SAVE 02BF..02C7 local store and argument pushes. Caller
 * authenticates code, supplies full RAM and disjoint host objects. Stack/code
 * overlap and invalid input refuse atomically; helper remains unexecuted. */
int pr_slot_a_save_store_args(uint8_t *ram,size_t size,const uint8_t *code,
    size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
