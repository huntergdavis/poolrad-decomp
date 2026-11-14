#ifndef POOLRAD_REPLAY_SLOT_A_RUNNER_OPCODE_FETCH_H
#define POOLRAD_REPLAY_SLOT_A_RUNNER_OPCODE_FETCH_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 319A..31A3 ADD DI,AX, opcode fetch and latch store. Caller
 * authenticates code, provides full RAM and disjoint host objects. Invalid
 * inputs or latch/code overlap refuse atomically; dispatcher is unexecuted. */
int pr_slot_a_runner_opcode_fetch(uint8_t *ram,size_t size,const uint8_t *code,
 size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
