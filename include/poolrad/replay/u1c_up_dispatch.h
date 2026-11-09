#ifndef POOLRAD_REPLAY_U1C_UP_DISPATCH_H
#define POOLRAD_REPLAY_U1C_UP_DISPATCH_H
#include "poolrad/replay/u1c_post_up.h"
pr_290b_first_status pr_u1c_dispatch(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_U1C_DISPATCH_EMPTY=0,PR_U1C_DISPATCH_WAITING,PR_U1C_DISPATCH_FINISHED } pr_u1c_dispatch_phase;
typedef enum { PR_U1C_DISPATCH_YIELD=0,PR_U1C_DISPATCH_REACHED,PR_U1C_DISPATCH_LOOP,PR_U1C_DISPATCH_ARGUMENT,PR_U1C_DISPATCH_STOP } pr_u1c_dispatch_status;
typedef struct { pr_u1c_dispatch_phase phase;pr_u1c_up_session up;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u1c_dispatch_session;
typedef struct { pr_u1c_up_status up_status;pr_u1c_up_diagnostic up;pr_290b_first_status dispatch_status;pr_290b_first_diagnostic dispatch; } pr_u1c_dispatch_diagnostic;
void pr_u1c_dispatch_session_init(pr_u1c_dispatch_session *state);
pr_u1c_dispatch_status pr_u1c_dispatch_start(pr_u1c_dispatch_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,pr_u1c_dispatch_diagnostic *diagnostic);
pr_u1c_dispatch_status pr_u1c_dispatch_resume(pr_u1c_dispatch_session *state,pr_u1c_dispatch_diagnostic *diagnostic);
#endif
