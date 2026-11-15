#ifndef POOLRAD_REPLAY_SLOT_A_AND_RETURN_ARGS_H
#define POOLRAD_REPLAY_SLOT_A_AND_RETURN_ARGS_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Explicit synthetic value; caller authenticates 11 code bytes, keeps code
 * immutable and host objects disjoint. */
int pr_slot_a_and_return_args(uint8_t *ram,size_t size,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *cpu);
#endif
