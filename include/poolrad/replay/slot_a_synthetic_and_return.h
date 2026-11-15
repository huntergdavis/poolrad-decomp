#ifndef POOLRAD_REPLAY_SLOT_A_SYNTHETIC_AND_RETURN_H
#define POOLRAD_REPLAY_SLOT_A_SYNTHETIC_AND_RETURN_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates immutable 4-byte handler tail and keeps host objects disjoint. */
int pr_slot_a_synthetic_and_return(uint8_t *,size_t,const uint8_t *,uint8_t,pr_u1c_3171_cpu *);
#endif
