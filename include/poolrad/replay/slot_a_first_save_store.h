#ifndef POOLRAD_REPLAY_SLOT_A_FIRST_SAVE_STORE_H
#define POOLRAD_REPLAY_SLOT_A_FIRST_SAVE_STORE_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
#include "poolrad/recovered/overlay_u25_store_word_0c77.h"
/* Conditional first SAVE value 11 to address 6E7D. Caller independently
 * authenticates all code/bindings, ensures immutable code and disjoint host
 * objects. Unsupported frames/aliases refuse atomically. Actual store status
 * and reached CPU/RAM are exposed; caller checks the return. */
int pr_slot_a_first_save_store(uint8_t *ram,size_t size,const uint8_t *call,size_t call_size,
 const uint8_t *stub,size_t stub_size,const uint8_t *body,size_t body_size,
 const uint8_t *hook,size_t hook_size,const uint8_t *classify,size_t classify_size,
 pr_u1c_3171_cpu *cpu,pr_u25_store_word_status *status);
#endif
