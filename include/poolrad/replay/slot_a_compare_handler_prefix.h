#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_HANDLER_PREFIX_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_HANDLER_PREFIX_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 2F58 call and COMPARE 0126..012F prologue. Caller authenticates
 * code, supplies disjoint host objects and full RAM. Unsupported stack/code
 * aliases and invalid inputs refuse atomically; loader call remains unexecuted. */
int pr_slot_a_compare_handler_prefix(uint8_t *ram,size_t size,const uint8_t *call,
 size_t call_size,const uint8_t *body,size_t body_size,pr_u1c_3171_cpu *cpu);
#endif
