#ifndef POOLRAD_REPLAY_U25_COMPARE_NUMERIC_FLAGS_H
#define POOLRAD_REPLAY_U25_COMPARE_NUMERIC_FLAGS_H
#include "poolrad/replay/u25_compare_fill_return.h"
pr_290b_first_status pr_u25_compare_numeric_flags(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
enum { PR_COMPARE_FLAGS_REACHED=PR_COMPARE_FILL_RETURN_REACHED+1 };
typedef enum { PR_COMPARE_FLAGS_EMPTY=0,PR_COMPARE_FLAGS_WAITING,PR_COMPARE_FLAGS_FINISHED } pr_compare_flags_phase;
typedef struct { pr_compare_flags_phase phase;pr_compare_fill_return_session fill;pr_bus *bus;pr_r0899_cpu *cpu; } pr_compare_flags_session;
typedef struct { pr_second_dispatch_status fill_status;pr_compare_fill_return_diagnostic fill;pr_290b_first_status flags_status; } pr_compare_flags_diagnostic;
void pr_compare_flags_session_init(pr_compare_flags_session *state);
pr_second_dispatch_status pr_compare_flags_start(pr_compare_flags_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_compare_flags_diagnostic *diagnostic);
pr_second_dispatch_status pr_compare_flags_resume(pr_compare_flags_session *state,pr_compare_flags_diagnostic *diagnostic);
#endif
