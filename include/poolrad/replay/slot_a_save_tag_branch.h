#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_TAG_BRANCH_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_TAG_BRANCH_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional SAVE 02AD..02B6 store and tag branch. Caller authenticates
 * code, supplies full RAM and disjoint host objects. Code/stack overlap
 * refuses atomically. Stops before the word or string helper path. */
int pr_slot_a_save_tag_branch(uint8_t *ram,size_t size,const uint8_t *code,
    size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
