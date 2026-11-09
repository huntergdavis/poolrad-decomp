#ifndef POOLRAD_REPLAY_U1C_RUNNER_PREFIX_H
#define POOLRAD_REPLAY_U1C_RUNNER_PREFIX_H
#include "poolrad/replay/u1c_script_entry.h"
pr_290b_first_status pr_u1c_runner_prefix(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_290b_return_target *target);
typedef enum { PR_U1C_RUNNER_EMPTY=0,PR_U1C_RUNNER_WAITING,PR_U1C_RUNNER_FINISHED } pr_u1c_runner_phase;
typedef enum { PR_U1C_RUNNER_YIELD=0,PR_U1C_RUNNER_DISPATCH,PR_U1C_RUNNER_RETURNED,PR_U1C_RUNNER_LOOP,PR_U1C_RUNNER_OTHER,PR_U1C_RUNNER_ARGUMENT,PR_U1C_RUNNER_STOP } pr_u1c_runner_status;
typedef struct { pr_u1c_runner_phase phase;pr_u1c_script_session script;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_return_target target; } pr_u1c_runner_session;
typedef struct { pr_u1c_script_status script_status;pr_u1c_script_diagnostic script;pr_290b_first_status runner_status; } pr_u1c_runner_diagnostic;
void pr_u1c_runner_session_init(pr_u1c_runner_session *state);
pr_u1c_runner_status pr_u1c_runner_start(pr_u1c_runner_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,pr_u1c_runner_diagnostic *diagnostic);
pr_u1c_runner_status pr_u1c_runner_resume(pr_u1c_runner_session *state,pr_u1c_runner_diagnostic *diagnostic);
#endif
