#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_EPILOGUE_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_EPILOGUE_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Conditional SAVE 02CD jump and 02DC..02DF far return. Caller authenticates
 * both code references and supplies disjoint host objects and full RAM.
 * BP above FFFA and invalid inputs refuse atomically; actual return is exposed. */
int pr_slot_a_save_epilogue(const uint8_t *ram,size_t size,const uint8_t *jump,
 size_t jump_size,const uint8_t *tail,size_t tail_size,pr_u1c_3171_cpu *cpu);
#endif
