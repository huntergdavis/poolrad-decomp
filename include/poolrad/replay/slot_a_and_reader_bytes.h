#ifndef POOLRAD_REPLAY_SLOT_A_AND_READER_BYTES_H
#define POOLRAD_REPLAY_SLOT_A_AND_READER_BYTES_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Caller authenticates 26 code bytes, keeps code immutable and host objects disjoint. */
int pr_slot_a_and_reader_bytes(uint8_t *ram,size_t size,const uint8_t *code,pr_u1c_3171_cpu *cpu);
#endif
