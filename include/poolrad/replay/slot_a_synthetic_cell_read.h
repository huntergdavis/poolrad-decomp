#ifndef POOLRAD_REPLAY_SLOT_A_SYNTHETIC_CELL_READ_H
#define POOLRAD_REPLAY_SLOT_A_SYNTHETIC_CELL_READ_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Explicit synthetic source byte; caller authenticates ten code bytes, keeps
 * code immutable, and supplies disjoint host objects. */
int pr_slot_a_synthetic_cell_read(uint8_t *ram,size_t size,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *cpu);
#endif
