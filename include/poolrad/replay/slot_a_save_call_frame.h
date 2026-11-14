#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_CALL_FRAME_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_CALL_FRAME_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional 2F85 PUSH CS; 2F86 CALL 0292. The caller authenticates
 * code and supplies disjoint host objects and full RAM. Refusal is atomic;
 * success stops at handler entry 0292 without executing its effects. */
int pr_slot_a_save_call_frame(uint8_t *ram, size_t size,
    const uint8_t *code, size_t code_size, pr_u1c_3171_cpu *cpu);
#endif
