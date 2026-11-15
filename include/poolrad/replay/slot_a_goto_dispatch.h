#ifndef POOLRAD_REPLAY_SLOT_A_GOTO_DISPATCH_H
#define POOLRAD_REPLAY_SLOT_A_GOTO_DISPATCH_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates four-byte selector and call references, keeps guest
 * code immutable and host objects disjoint. Returns first refused/reached IP;
 * a malformed near call retains the CS push at 2F43. */
uint16_t pr_slot_a_goto_dispatch(uint8_t *ram,size_t size,const uint8_t *selector,const uint8_t *call,pr_u1c_3171_cpu *cpu);
#endif
