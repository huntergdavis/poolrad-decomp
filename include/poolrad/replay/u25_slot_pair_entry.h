#ifndef POOLRAD_REPLAY_U25_SLOT_PAIR_ENTRY_H
#define POOLRAD_REPLAY_U25_SLOT_PAIR_ENTRY_H
#include "poolrad/replay/u25_slot_dispatch.h"
pr_290b_first_status pr_u25_slot_pair_entry(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_SLOT_PAIR_EMPTY=0,PR_SLOT_PAIR_WAITING,PR_SLOT_PAIR_FINISHED } pr_slot_pair_phase;
typedef enum { PR_SLOT_PAIR_YIELD=0,PR_SLOT_PAIR_REACHED,PR_SLOT_PAIR_ZERO,PR_SLOT_PAIR_WORD,PR_SLOT_PAIR_OTHER_TYPE,PR_SLOT_PAIR_STRING,PR_SLOT_PAIR_LOADER_STRING,PR_SLOT_PAIR_LIMIT,PR_SLOT_PAIR_PROMPT,PR_SLOT_PAIR_OTHER,PR_SLOT_PAIR_ARGUMENT,PR_SLOT_PAIR_STOP } pr_slot_pair_status;
typedef struct { pr_slot_pair_phase phase;pr_slot_dispatch_session dispatch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_slot_pair_session;
typedef struct { pr_slot_dispatch_status dispatch_status;pr_slot_dispatch_diagnostic dispatch;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_slot_pair_diagnostic;
void pr_slot_pair_session_init(pr_slot_pair_session *state);
pr_slot_pair_status pr_slot_pair_start(pr_slot_pair_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_slot_pair_diagnostic *diagnostic);
pr_slot_pair_status pr_slot_pair_resume(pr_slot_pair_session *state,pr_slot_pair_diagnostic *diagnostic);
#endif
