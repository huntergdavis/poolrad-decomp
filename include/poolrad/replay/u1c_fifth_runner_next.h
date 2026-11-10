#ifndef POOLRAD_REPLAY_U1C_FIFTH_RUNNER_NEXT_H
#define POOLRAD_REPLAY_U1C_FIFTH_RUNNER_NEXT_H
#include "poolrad/replay/u1c_fifth_dispatch_return.h"
#include "poolrad/replay/u1c_runner_next.h"
enum { PR_FIFTH_RUNNER_DISPATCH=PR_FIFTH_DISPATCH_RETURN_REACHED+1,PR_FIFTH_RUNNER_RETURNED };
typedef enum { PR_FIFTH_RUNNER_EMPTY=0,PR_FIFTH_RUNNER_WAITING,PR_FIFTH_RUNNER_FINISHED } pr_fifth_runner_phase;
typedef struct { pr_fifth_runner_phase phase;pr_fifth_dispatch_return_session dispatch;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_return_target target; } pr_fifth_runner_session;
typedef struct { pr_second_dispatch_status dispatch_status;pr_fifth_dispatch_return_diagnostic dispatch;pr_290b_first_status next_status; } pr_fifth_runner_diagnostic;
void pr_fifth_runner_session_init(pr_fifth_runner_session *state);
pr_second_dispatch_status pr_fifth_runner_start(pr_fifth_runner_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fifth_runner_diagnostic *diagnostic);
pr_second_dispatch_status pr_fifth_runner_resume(pr_fifth_runner_session *state,pr_fifth_runner_diagnostic *diagnostic);
#endif
