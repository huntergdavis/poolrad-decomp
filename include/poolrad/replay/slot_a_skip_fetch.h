#ifndef POOLRAD_REPLAY_SLOT_A_SKIP_FETCH_H
#define POOLRAD_REPLAY_SLOT_A_SKIP_FETCH_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates the 23-byte code span, keeps guest code immutable,
 * and supplies disjoint host objects. Unsupported aliases refuse atomically. */
int pr_slot_a_skip_fetch(uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
