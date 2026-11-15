#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_6DD5_SLOT_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_6DD5_SLOT_H
#include "poolrad/replay/slot_a_compare_first_slot.h"
/* Caller authenticates fixed references and loaded bindings; code is immutable
 * and host objects disjoint. Only tag-01/address-6DD5 is supported. Refusal is
 * atomic; reached raw helper status and state are exposed. */
int pr_slot_a_compare_6dd5_slot(uint8_t *ram,size_t size,const pr_compare_slot_refs *refs,pr_u1c_3171_cpu *cpu,pr_u25_slot_value_status *status);
#endif
