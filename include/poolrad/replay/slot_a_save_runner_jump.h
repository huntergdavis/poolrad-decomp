#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_RUNNER_JUMP_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_RUNNER_JUMP_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Reuse the fixed loaded 3582:31A8 jump model. Invalid CPU/RAM/code refuses
 * without changes. Returns 1 on success at 317F before the runner loop test,
 * and 0 on refusal. */
int pr_slot_a_save_runner_jump(pr_u1c_3171_cpu *cpu,const uint8_t *ram,size_t size);
#endif
