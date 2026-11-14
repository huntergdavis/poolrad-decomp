#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_SECOND_SLOT_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_SECOND_SLOT_H
#include "poolrad/replay/slot_a_compare_first_slot.h"
/* Caller-authenticated 11-byte 0186 span and immutable loaded bindings.
 * Only the tag-00 immediate second operand is supported. Host objects are
 * disjoint and guest writes must not overlap loaded code. Refusal is atomic. */
int pr_slot_a_compare_second_slot(uint8_t *ram,size_t size,const pr_compare_slot_refs *refs,
 pr_u1c_3171_cpu *cpu,pr_u25_slot_value_status *status);
#endif
