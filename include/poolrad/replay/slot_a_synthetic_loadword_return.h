#ifndef POOLRAD_REPLAY_SLOT_A_SYNTHETIC_LOADWORD_RETURN_H
#define POOLRAD_REPLAY_SLOT_A_SYNTHETIC_LOADWORD_RETURN_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates nine code bytes and synthetic result, keeps code
 * immutable and host objects disjoint. Actual return frame is exposed. */
int pr_slot_a_synthetic_loadword_return(const uint8_t *ram,size_t size,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *cpu);
#endif
