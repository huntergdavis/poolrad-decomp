#ifndef POOLRAD_REPLAY_SLOT_A_COMPARE_LOADER_H
#define POOLRAD_REPLAY_SLOT_A_COMPARE_LOADER_H
#include "poolrad/replay/slot_a_save_loader_compose.h"
/* Caller authenticates fixed-size call/stub/body references and SYSTEM binding,
 * maintains the underlying loader's code/data nonalias contract, and supplies
 * disjoint host objects. A post-call refusal retains its reached call frame. */
typedef enum { PR_COMPARE_LOAD_ARGUMENT, PR_COMPARE_LOAD_CALL_REFUSED, PR_COMPARE_LOAD_STUB_REFUSED, PR_COMPARE_LOAD_REACHED } pr_slot_a_compare_load_status;
typedef struct { pr_u25_0000_status loader; } pr_slot_a_compare_load_report;
pr_slot_a_compare_load_status pr_slot_a_compare_loader(uint8_t *ram,size_t size,
 const uint8_t *call,const uint8_t *stub,const uint8_t *body,uint16_t system_cs,
 size_t max_operands,size_t max_strings,pr_u1c_3171_cpu *cpu,pr_slot_a_compare_load_report *report);
#endif
