#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_WORD_ARGS_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_WORD_ARGS_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional SAVE 02A0..02A7, stopping before helper LCALL. Caller
 * authenticates code, provides full RAM and disjoint host objects.
 * Stack/code overlap and invalid inputs refuse before changes. */
int pr_slot_a_save_word_args(uint8_t *ram,size_t size,const uint8_t *code,
    size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
