#ifndef POOLRAD_REPLAY_U1C_FOURTH_DISPATCH_H
#define POOLRAD_REPLAY_U1C_FOURTH_DISPATCH_H
#include "poolrad/replay/u1c_third_runner_next.h"
#include "poolrad/replay/u1c_ecl_dispatch_prefix.h"
enum { PR_FOURTH_DISPATCH_HANDLER=PR_THIRD_RUNNER_RETURNED+1,PR_FOURTH_DISPATCH_LATER };
typedef enum { PR_FOURTH_DISPATCH_EMPTY=0,PR_FOURTH_DISPATCH_WAITING,PR_FOURTH_DISPATCH_FINISHED } pr_fourth_dispatch_phase;
typedef struct { pr_fourth_dispatch_phase phase;pr_third_runner_session runner;pr_bus *bus;pr_r0899_cpu *cpu; } pr_fourth_dispatch_session;
typedef struct { pr_second_dispatch_status runner_status;pr_third_runner_diagnostic runner;pr_290b_first_status dispatch_status; } pr_fourth_dispatch_diagnostic;
void pr_fourth_dispatch_session_init(pr_fourth_dispatch_session *state);
pr_second_dispatch_status pr_fourth_dispatch_start(pr_fourth_dispatch_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fourth_dispatch_diagnostic *diagnostic);
pr_second_dispatch_status pr_fourth_dispatch_resume(pr_fourth_dispatch_session *state,pr_fourth_dispatch_diagnostic *diagnostic);
#endif
