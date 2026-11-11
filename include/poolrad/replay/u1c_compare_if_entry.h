#ifndef POOLRAD_REPLAY_U1C_COMPARE_IF_ENTRY_H
#define POOLRAD_REPLAY_U1C_COMPARE_IF_ENTRY_H
#include "poolrad/replay/u1c_unmatched_dispatch_return.h"
pr_290b_first_status pr_u1c_compare_if_entry(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
enum { PR_COMPARE_IF_ENTRY_REACHED=PR_UNMATCHED_RETURN_REACHED+1 };
typedef enum { PR_COMPARE_IF_EMPTY=0,PR_COMPARE_IF_WAITING,PR_COMPARE_IF_FINISHED } pr_compare_if_phase;
typedef struct { pr_compare_if_phase phase;pr_compare_later_session later;pr_bus *bus;pr_r0899_cpu *cpu; } pr_compare_if_session;
typedef struct { pr_second_dispatch_status later_status;pr_compare_later_diagnostic later;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_compare_if_diagnostic;
void pr_compare_if_session_init(pr_compare_if_session *state);
pr_second_dispatch_status pr_compare_if_start(pr_compare_if_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_compare_if_diagnostic *diagnostic);
pr_second_dispatch_status pr_compare_if_resume(pr_compare_if_session *state,pr_compare_if_diagnostic *diagnostic);
#endif
