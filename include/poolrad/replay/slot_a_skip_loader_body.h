#ifndef POOLRAD_REPLAY_SLOT_A_SKIP_LOADER_BODY_H
#define POOLRAD_REPLAY_SLOT_A_SKIP_LOADER_BODY_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"
/* Caller authenticates the 310-byte body and SYSTEM binding, keeps code
 * immutable and host objects disjoint. Refusal is atomic; reached loader
 * status and state are exposed. Only numeric tag 00,02 is supported. */
int pr_slot_a_skip_loader_body(uint8_t *ram,size_t size,const uint8_t *body,size_t body_size,uint16_t system_cs,pr_u1c_3171_cpu *cpu,pr_u25_0000_status *status);
#endif
