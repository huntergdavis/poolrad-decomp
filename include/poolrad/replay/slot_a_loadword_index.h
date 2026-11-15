#ifndef POOLRAD_REPLAY_SLOT_A_LOADWORD_INDEX_H
#define POOLRAD_REPLAY_SLOT_A_LOADWORD_INDEX_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates 148 code bytes, keeps code immutable and host objects disjoint. */
int pr_slot_a_loadword_index(uint8_t *ram,size_t size,const uint8_t *code,pr_u1c_3171_cpu *cpu);
#endif
