#ifndef POOLRAD_REPLAY_SLOT_A_AND_COMPARE_ARGS_H
#define POOLRAD_REPLAY_SLOT_A_AND_COMPARE_ARGS_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Explicit synthetic source; caller authenticates nine code bytes, keeps code
 * immutable and host objects disjoint. */
int pr_slot_a_and_compare_args(uint8_t *ram,size_t size,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *cpu);
#endif
