#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_RUNNER_LOOP_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_RUNNER_LOOP_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 317F..318C two ordered stop tests. Caller authenticates code
 * and supplies full RAM and disjoint host objects. Invalid inputs refuse
 * atomically. Success stops at 31AA (exit) or 318D (next opcode load). */
int pr_slot_a_save_runner_loop(const uint8_t *ram,size_t size,const uint8_t *code,
 size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
