#ifndef POOLRAD_REPLAY_SLOT_A_FIRST_SAVE_COMPOSE_H
#define POOLRAD_REPLAY_SLOT_A_FIRST_SAVE_COMPOSE_H
#include "poolrad/replay/slot_a_save_slot_value.h"
#include "poolrad/replay/slot_a_first_save_store.h"
/* Caller-authenticated fixed-size references, disjoint host objects, immutable
 * guest code and all constituent model preconditions are required. */
typedef struct { const uint8_t *slot_call,*slot_stub,*slot_body,*word_body,*store_args,*store_call,*store_stub,*store_body,*store_hook,*classifier; } pr_slot_a_first_save_refs;
typedef enum { PR_FIRST_SAVE_ARGUMENT, PR_FIRST_SAVE_SLOT_REFUSED, PR_FIRST_SAVE_SLOT_STOP, PR_FIRST_SAVE_ARGS_REFUSED, PR_FIRST_SAVE_STORE_REFUSED, PR_FIRST_SAVE_STORE_STOP, PR_FIRST_SAVE_AT_02CD } pr_slot_a_first_save_status;
typedef struct { pr_u25_slot_value_status slot; pr_u25_store_word_status store; } pr_slot_a_first_save_report;
/* Starts at 02B7 with live slot-1 tag 00. Each failure preserves reached effects from prior stages;
 * report must not overlap RAM, CPU, references or other host objects. */
pr_slot_a_first_save_status pr_slot_a_first_save_compose(uint8_t *ram,size_t size,
 const pr_slot_a_first_save_refs *refs,pr_u1c_3171_cpu *cpu,pr_slot_a_first_save_report *report);
#endif
