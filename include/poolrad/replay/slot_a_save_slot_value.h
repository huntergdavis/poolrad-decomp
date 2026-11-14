#ifndef POOLRAD_REPLAY_SLOT_A_SAVE_SLOT_VALUE_H
#define POOLRAD_REPLAY_SLOT_A_SAVE_SLOT_VALUE_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
#include "poolrad/recovered/overlay_u25_slot_value_0136.h"
/* Conditional numeric tag-02 slot 1 at 02B7 through raw U25 0136. Caller
 * authenticates all code/bindings, ensures immutable code and disjoint host
 * objects. Unsupported tag, stack/table alias and invalid inputs refuse
 * atomically. Once reached, actual helper status and CPU/RAM are exposed. */
int pr_slot_a_save_slot_value(uint8_t *ram,size_t size,const uint8_t *call,size_t call_size,
    const uint8_t *stub,size_t stub_size,const uint8_t *body,size_t body_size,
    const uint8_t *word,size_t word_size,pr_u1c_3171_cpu *cpu,pr_u25_slot_value_status *status);
#endif
