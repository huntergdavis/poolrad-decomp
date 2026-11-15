#ifndef POOLRAD_REPLAY_SLOT_A_STORE_CLASSIFIER_CALL_H
#define POOLRAD_REPLAY_SLOT_A_STORE_CLASSIFIER_CALL_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
/* Explicit synthetic source; caller authenticates 13 code bytes, keeps code
 * immutable and host objects disjoint. Store effect remains unexecuted. */
int pr_slot_a_store_classifier_call(uint8_t *ram,size_t size,const uint8_t *code,uint8_t synthetic,pr_u1c_3171_cpu *cpu);
#endif
