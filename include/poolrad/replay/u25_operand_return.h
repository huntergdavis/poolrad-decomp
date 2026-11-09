#ifndef POOLRAD_REPLAY_U25_OPERAND_RETURN_H
#define POOLRAD_REPLAY_U25_OPERAND_RETURN_H
#include "poolrad/replay/u25_operand_loop.h"
pr_290b_first_status pr_u25_operand_return(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_290b_return_target *target);
typedef enum { PR_U25_RETURN_EMPTY=0,PR_U25_RETURN_WAITING,PR_U25_RETURN_FINISHED } pr_u25_return_phase;
typedef enum { PR_U25_RETURN_YIELD=0,PR_U25_RETURN_REACHED,PR_U25_RETURN_STRING,PR_U25_RETURN_LIMIT,PR_U25_RETURN_PROMPT,PR_U25_RETURN_OTHER,PR_U25_RETURN_ARGUMENT,PR_U25_RETURN_STOP } pr_u25_return_status;
typedef struct { pr_u25_return_phase phase;pr_u25_loop_session loop;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_return_target target; } pr_u25_return_session;
typedef struct { pr_u25_loop_status loop_status;pr_u25_loop_diagnostic loop;pr_290b_first_status return_status; } pr_u25_return_diagnostic;
void pr_u25_return_session_init(pr_u25_return_session *state);
pr_u25_return_status pr_u25_return_start(pr_u25_return_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_u25_return_diagnostic *diagnostic);
pr_u25_return_status pr_u25_return_resume(pr_u25_return_session *state,pr_u25_return_diagnostic *diagnostic);
#endif
