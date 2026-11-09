#ifndef POOLRAD_REPLAY_U1C_SCRIPT_ENTRY_H
#define POOLRAD_REPLAY_U1C_SCRIPT_ENTRY_H
#include "poolrad/replay/u1c_up_dispatch.h"
pr_290b_first_status pr_u1c_script_entry(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
typedef enum { PR_U1C_SCRIPT_EMPTY=0,PR_U1C_SCRIPT_WAITING,PR_U1C_SCRIPT_FINISHED } pr_u1c_script_phase;
typedef enum { PR_U1C_SCRIPT_YIELD=0,PR_U1C_SCRIPT_REACHED,PR_U1C_SCRIPT_LOOP,PR_U1C_SCRIPT_OTHER,PR_U1C_SCRIPT_ARGUMENT,PR_U1C_SCRIPT_STOP } pr_u1c_script_status;
typedef struct { pr_u1c_script_phase phase;pr_u1c_dispatch_session dispatch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u1c_script_session;
typedef struct { pr_u1c_dispatch_status dispatch_status;pr_u1c_dispatch_diagnostic dispatch;pr_290b_first_status entry_status; } pr_u1c_script_diagnostic;
void pr_u1c_script_session_init(pr_u1c_script_session *state);
pr_u1c_script_status pr_u1c_script_start(pr_u1c_script_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,pr_u1c_script_diagnostic *diagnostic);
pr_u1c_script_status pr_u1c_script_resume(pr_u1c_script_session *state,pr_u1c_script_diagnostic *diagnostic);
#endif
