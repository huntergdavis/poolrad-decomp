#ifndef POOLRAD_REPLAY_SLOT_A_LOADWORD_CLASSIFIER_H
#define POOLRAD_REPLAY_SLOT_A_LOADWORD_CLASSIFIER_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
#include "poolrad/recovered/overlay_u25_address_class_06fb_cpu.h"
/* Caller authenticates 91 code bytes, keeps code immutable and host objects disjoint. */
int pr_slot_a_loadword_classifier(uint8_t *ram,size_t size,const uint8_t *code,pr_u1c_3171_cpu *cpu,pr_u25_06fb_status *status);
#endif
