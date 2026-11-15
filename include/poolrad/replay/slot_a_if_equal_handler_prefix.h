#ifndef POOLRAD_REPLAY_SLOT_A_IF_EQUAL_HANDLER_PREFIX_H
#define POOLRAD_REPLAY_SLOT_A_IF_EQUAL_HANDLER_PREFIX_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates 111-byte code span, immutable guest code and disjoint
 * host objects. Only latch 16 / predicate byte 1 is accepted; refusal is atomic. */
int pr_slot_a_if_equal_handler_prefix(uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
