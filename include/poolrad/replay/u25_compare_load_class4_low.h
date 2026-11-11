#ifndef POOLRAD_REPLAY_U25_COMPARE_LOAD_CLASS4_LOW_H
#define POOLRAD_REPLAY_U25_COMPARE_LOAD_CLASS4_LOW_H
#include "poolrad/replay/u25_compare_load_dispatch.h"
pr_290b_first_status pr_u25_load_class4_low(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
enum { PR_COMPARE_LOAD_LOW_REACHED=PR_COMPARE_LOAD_DISPATCH_OTHER+1,PR_COMPARE_LOAD_HIGH_REACHED };
typedef enum { PR_COMPARE_LOAD_LOW_EMPTY=0,PR_COMPARE_LOAD_LOW_WAITING,PR_COMPARE_LOAD_LOW_FINISHED } pr_compare_load_low_phase;
typedef struct { pr_compare_load_low_phase phase;pr_compare_load_dispatch_session dispatch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_compare_load_low_session;
typedef struct { pr_second_dispatch_status dispatch_status;pr_compare_load_dispatch_diagnostic dispatch;pr_290b_first_status low_status; } pr_compare_load_low_diagnostic;
void pr_compare_load_low_session_init(pr_compare_load_low_session *state);
pr_second_dispatch_status pr_compare_load_low_start(pr_compare_load_low_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_compare_load_low_diagnostic *diagnostic);
pr_second_dispatch_status pr_compare_load_low_resume(pr_compare_load_low_session *state,pr_compare_load_low_diagnostic *diagnostic);
#endif
