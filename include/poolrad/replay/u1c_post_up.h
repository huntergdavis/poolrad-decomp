#ifndef POOLRAD_REPLAY_U1C_POST_UP_H
#define POOLRAD_REPLAY_U1C_POST_UP_H
#include "poolrad/replay/u68_290b_exit.h"
pr_290b_first_status pr_u1c_post_up(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
typedef enum { PR_U1C_UP_EMPTY=0,PR_U1C_UP_WAITING,PR_U1C_UP_FINISHED } pr_u1c_up_phase;
typedef enum { PR_U1C_UP_YIELD=0,PR_U1C_UP_REACHED,PR_U1C_UP_LOOP,PR_U1C_UP_ARGUMENT,PR_U1C_UP_STOP } pr_u1c_up_status;
typedef struct { pr_u1c_up_phase phase;pr_u68_290b_exit exit;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u1c_up_session;
typedef struct { pr_u68_290b_exit_status exit_status;pr_u68_290b_exit_diagnostic exit;pr_290b_first_status caller_status; } pr_u1c_up_diagnostic;
void pr_u1c_up_session_init(pr_u1c_up_session *state);
pr_u1c_up_status pr_u1c_up_start(pr_u1c_up_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,pr_u1c_up_diagnostic *diagnostic);
pr_u1c_up_status pr_u1c_up_resume(pr_u1c_up_session *state,pr_u1c_up_diagnostic *diagnostic);
#endif
