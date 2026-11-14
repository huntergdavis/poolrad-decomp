#ifndef POOLRAD_REPLAY_SLOT_A_IF_CALL_H
#define POOLRAD_REPLAY_SLOT_A_IF_CALL_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
typedef enum { PR_IF_CALL_REACHED,PR_IF_CALL_ARGUMENT,PR_IF_CALL_ENTRY_CODE,PR_IF_CALL_NEAR_CODE } pr_if_call_status;
/* Caller authenticates four-byte code reference, keeps guest code immutable,
 * and supplies disjoint host objects. NEAR_CODE retains the pushed CS. */
pr_if_call_status pr_slot_a_if_call(uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
