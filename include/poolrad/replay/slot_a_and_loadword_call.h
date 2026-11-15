#ifndef POOLRAD_REPLAY_SLOT_A_AND_LOADWORD_CALL_H
#define POOLRAD_REPLAY_SLOT_A_AND_LOADWORD_CALL_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates five code bytes, keeps code immutable and host objects disjoint. */
int pr_slot_a_and_loadword_call(uint8_t *ram,size_t size,const uint8_t *code,pr_u1c_3171_cpu *cpu);
#endif
