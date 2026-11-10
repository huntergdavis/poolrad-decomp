#ifndef POOLRAD_REPLAY_U25_FOURTH_PAIR_ENTRY_H
#define POOLRAD_REPLAY_U25_FOURTH_PAIR_ENTRY_H
#include "poolrad/replay/u25_fourth_slot_dispatch.h"
#include "poolrad/replay/u25_slot_pair_entry.h"
typedef enum { PR_FOURTH_PAIR_EMPTY=0,PR_FOURTH_PAIR_WAITING,PR_FOURTH_PAIR_FINISHED } pr_fourth_pair_phase;
typedef struct { pr_fourth_pair_phase phase;pr_fourth_slot_session slot;pr_bus *bus;pr_r0899_cpu *cpu; } pr_fourth_pair_session;
typedef struct { pr_second_dispatch_status slot_status;pr_fourth_slot_diagnostic slot;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_fourth_pair_diagnostic;
void pr_fourth_pair_session_init(pr_fourth_pair_session *state);
pr_second_dispatch_status pr_fourth_pair_start(pr_fourth_pair_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fourth_pair_diagnostic *diagnostic);
pr_second_dispatch_status pr_fourth_pair_resume(pr_fourth_pair_session *state,pr_fourth_pair_diagnostic *diagnostic);
#endif
