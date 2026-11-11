#ifndef POOLRAD_REPLAY_U1C_UNMATCHED_DISPATCH_RETURN_H
#define POOLRAD_REPLAY_U1C_UNMATCHED_DISPATCH_RETURN_H
#include "poolrad/replay/u1c_compare_later_opcode.h"
pr_290b_first_status pr_u1c_unmatched_dispatch_return(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_290b_return_target *target);
enum { PR_UNMATCHED_RETURN_REACHED=PR_COMPARE_LATER_UNMATCHED+1 };
typedef enum { PR_UNMATCHED_RETURN_EMPTY=0,PR_UNMATCHED_RETURN_WAITING,PR_UNMATCHED_RETURN_FINISHED } pr_unmatched_return_phase;
typedef struct { pr_unmatched_return_phase phase;pr_compare_later_session later;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_return_target target; } pr_unmatched_return_session;
typedef struct { pr_second_dispatch_status later_status;pr_compare_later_diagnostic later;pr_290b_first_status return_status; } pr_unmatched_return_diagnostic;
void pr_unmatched_return_session_init(pr_unmatched_return_session *state);
pr_second_dispatch_status pr_unmatched_return_start(pr_unmatched_return_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_unmatched_return_diagnostic *diagnostic);
pr_second_dispatch_status pr_unmatched_return_resume(pr_unmatched_return_session *state,pr_unmatched_return_diagnostic *diagnostic);
#endif
