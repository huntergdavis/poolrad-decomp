#ifndef POOLRAD_REPLAY_U68_290B_EXIT_H
#define POOLRAD_REPLAY_U68_290B_EXIT_H
#include "poolrad/replay/u68_290b_print.h"
pr_290b_first_status pr_u68_post_display_exit(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_290b_clear_limits *limits,const pr_290b_return_target *target,pr_290b_clear_diagnostic *diagnostic);
typedef enum { PR_U68_290B_EXIT_EMPTY=0,PR_U68_290B_EXIT_WAITING,PR_U68_290B_EXIT_FINISHED } pr_u68_290b_exit_phase;
typedef enum { PR_U68_290B_EXIT_YIELD=0,PR_U68_290B_EXIT_RETURNED,PR_U68_290B_EXIT_LOOP,PR_U68_290B_EXIT_ARGUMENT,PR_U68_290B_EXIT_STOP } pr_u68_290b_exit_status;
typedef struct { pr_u68_290b_exit_phase phase;pr_u68_290b_print print;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_clear_limits limits;pr_290b_return_target target; } pr_u68_290b_exit;
typedef struct { pr_u68_290b_print_status print_status;pr_u68_290b_print_diagnostic print;pr_290b_first_status exit_status;pr_290b_clear_diagnostic exit; } pr_u68_290b_exit_diagnostic;
void pr_u68_290b_exit_init(pr_u68_290b_exit *state);
pr_u68_290b_exit_status pr_u68_290b_exit_start(pr_u68_290b_exit *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,pr_u68_290b_exit_diagnostic *diagnostic);
pr_u68_290b_exit_status pr_u68_290b_exit_resume(pr_u68_290b_exit *state,pr_u68_290b_exit_diagnostic *diagnostic);
#endif
