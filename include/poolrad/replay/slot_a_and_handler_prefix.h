#ifndef POOLRAD_REPLAY_SLOT_A_AND_HANDLER_PREFIX_H
#define POOLRAD_REPLAY_SLOT_A_AND_HANDLER_PREFIX_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Authenticated nine-byte prefix; immutable code and disjoint host objects. */
int pr_slot_a_and_handler_prefix(uint8_t *ram,size_t size,const uint8_t *code,pr_u1c_3171_cpu *cpu);
#endif
