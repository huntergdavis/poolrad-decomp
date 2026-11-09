#ifndef POOLRAD_REPLAY_U25_OPERAND_LOOP_H
#define POOLRAD_REPLAY_U25_OPERAND_LOOP_H
#include "poolrad/replay/u25_operand_tag.h"
typedef enum { PR_U25_LOOP_STEP_DONE=0,PR_U25_LOOP_STEP_STRING,PR_U25_LOOP_STEP_LIMIT,PR_U25_LOOP_STEP_ARGUMENT,PR_U25_LOOP_STEP_STOP } pr_u25_loop_step_status;
typedef struct { size_t iterations;pr_290b_first_status tag_status; } pr_u25_loop_step_diagnostic;
pr_u25_loop_step_status pr_u25_numeric_loop(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,size_t max_iterations,pr_u25_loop_step_diagnostic *diagnostic);
typedef enum { PR_U25_LOOP_EMPTY=0,PR_U25_LOOP_WAITING,PR_U25_LOOP_FINISHED } pr_u25_loop_phase;
typedef enum { PR_U25_LOOP_YIELD=0,PR_U25_LOOP_REACHED,PR_U25_LOOP_STRING,PR_U25_LOOP_LIMIT,PR_U25_LOOP_PROMPT,PR_U25_LOOP_OTHER,PR_U25_LOOP_ARGUMENT,PR_U25_LOOP_STOP } pr_u25_loop_status;
typedef struct { pr_u25_loop_phase phase;pr_u25_tag_session tag;pr_bus *bus;pr_r0899_cpu *cpu;size_t max_iterations; } pr_u25_loop_session;
typedef struct { pr_u25_tag_status tag_status;pr_u25_tag_diagnostic tag;pr_u25_loop_step_status loop_status;pr_u25_loop_step_diagnostic loop; } pr_u25_loop_diagnostic;
void pr_u25_loop_session_init(pr_u25_loop_session *state);
pr_u25_loop_status pr_u25_loop_start(pr_u25_loop_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_u25_loop_diagnostic *diagnostic);
pr_u25_loop_status pr_u25_loop_resume(pr_u25_loop_session *state,pr_u25_loop_diagnostic *diagnostic);
#endif
