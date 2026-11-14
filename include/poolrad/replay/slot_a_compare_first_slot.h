#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_FIRST_SLOT_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_FIRST_SLOT_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
#include "poolrad/recovered/overlay_u25_slot_value_0136.h"
/* Caller-authenticated fixed-size references and immutable loaded bindings. */
typedef struct { const uint8_t *call,*stub,*slot,*word,*load,*classify,*hook,*party; } pr_compare_slot_refs;
/* Fixed tag-01/address-49C9 path at 017E. Unsupported profiles refuse
 * atomically; on reach the raw helper status and actual CPU/RAM are exposed.
 * Host objects are disjoint and guest writes must not overlap loaded code. */
int pr_slot_a_compare_first_slot(uint8_t *ram,size_t size,const pr_compare_slot_refs *refs,
 pr_u1c_3171_cpu *cpu,pr_u25_slot_value_status *status);
#endif
