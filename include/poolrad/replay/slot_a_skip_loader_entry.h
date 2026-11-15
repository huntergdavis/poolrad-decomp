#ifndef POOLRAD_REPLAY_SLOT_A_SKIP_LOADER_ENTRY_H
#define POOLRAD_REPLAY_SLOT_A_SKIP_LOADER_ENTRY_H
#include "poolrad/recovered/overlay_u1c_run_3171_cpu.h"
typedef enum { PR_SKIP_LOADER_REACHED,PR_SKIP_LOADER_ARGUMENT,PR_SKIP_LOADER_SETUP_CODE,PR_SKIP_LOADER_CALL_CODE } pr_skip_loader_entry_status;
/* Caller authenticates seven-byte code, immutable guest code and disjoint
 * host objects. CALL_CODE retains argument and CS pushes. */
pr_skip_loader_entry_status pr_slot_a_skip_loader_entry(uint8_t *ram,size_t size,const uint8_t *code,size_t code_size,pr_u1c_3171_cpu *cpu);
#endif
