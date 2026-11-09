#ifndef POOLRAD_REPLAY_U1C_ECL_DISPATCH_PREFIX_H
#define POOLRAD_REPLAY_U1C_ECL_DISPATCH_PREFIX_H
#include "poolrad/replay/u1c_runner_prefix.h"
pr_290b_first_status pr_u1c_ecl_dispatch_prefix(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
typedef enum { PR_U1C_ECL_EMPTY=0,PR_U1C_ECL_WAITING,PR_U1C_ECL_FINISHED } pr_u1c_ecl_phase;
typedef enum { PR_U1C_ECL_YIELD=0,PR_U1C_ECL_HANDLER,PR_U1C_ECL_LOOP,PR_U1C_ECL_OTHER,PR_U1C_ECL_ARGUMENT,PR_U1C_ECL_STOP } pr_u1c_ecl_status;
typedef struct { pr_u1c_ecl_phase phase;pr_u1c_runner_session runner;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u1c_ecl_session;
typedef struct { pr_u1c_runner_status runner_status;pr_u1c_runner_diagnostic runner;pr_290b_first_status prefix_status; } pr_u1c_ecl_diagnostic;
void pr_u1c_ecl_session_init(pr_u1c_ecl_session *state);
pr_u1c_ecl_status pr_u1c_ecl_start(pr_u1c_ecl_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,pr_u1c_ecl_diagnostic *diagnostic);
pr_u1c_ecl_status pr_u1c_ecl_resume(pr_u1c_ecl_session *state,pr_u1c_ecl_diagnostic *diagnostic);
#endif
