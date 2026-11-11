#ifndef POOLRAD_REPLAY_U25_COMPARE_LOAD_LOW_RETURN_H
#define POOLRAD_REPLAY_U25_COMPARE_LOAD_LOW_RETURN_H
#include "poolrad/replay/u25_compare_load_class4_low.h"
pr_290b_first_status pr_u25_load_word_return(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_290b_return_target *target);
enum { PR_COMPARE_LOAD_RETURN_REACHED=PR_COMPARE_LOAD_HIGH_REACHED+1 };
typedef enum { PR_COMPARE_LOAD_RETURN_EMPTY=0,PR_COMPARE_LOAD_RETURN_WAITING,PR_COMPARE_LOAD_RETURN_FINISHED } pr_compare_load_return_phase;
typedef struct { pr_compare_load_return_phase phase;pr_compare_load_low_session low;pr_bus *bus;pr_r0899_cpu *cpu; } pr_compare_load_return_session;
typedef struct { pr_second_dispatch_status low_status;pr_compare_load_low_diagnostic low;pr_290b_first_status return_status; } pr_compare_load_return_diagnostic;
void pr_compare_load_return_session_init(pr_compare_load_return_session *state);
pr_second_dispatch_status pr_compare_load_return_start(pr_compare_load_return_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_compare_load_return_diagnostic *diagnostic);
pr_second_dispatch_status pr_compare_load_return_resume(pr_compare_load_return_session *state,pr_compare_load_return_diagnostic *diagnostic);
#endif
