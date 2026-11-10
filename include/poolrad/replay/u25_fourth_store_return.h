#ifndef POOLRAD_REPLAY_U25_FOURTH_STORE_RETURN_H
#define POOLRAD_REPLAY_U25_FOURTH_STORE_RETURN_H
#include "poolrad/replay/u25_fourth_class4_low.h"
#include "poolrad/replay/u25_store_return.h"
enum { PR_FOURTH_STORE_RETURN_REACHED=PR_FOURTH_CLASS4_LOW_HIGH+1 };
typedef enum { PR_FOURTH_STORE_RETURN_EMPTY=0,PR_FOURTH_STORE_RETURN_WAITING,PR_FOURTH_STORE_RETURN_FINISHED } pr_fourth_store_return_phase;
typedef struct { pr_fourth_store_return_phase phase;pr_fourth_class4_session low;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_return_target target; } pr_fourth_store_return_session;
typedef struct { pr_second_dispatch_status low_status;pr_fourth_class4_diagnostic low;pr_290b_first_status return_status; } pr_fourth_store_return_diagnostic;
void pr_fourth_store_return_session_init(pr_fourth_store_return_session *state);
pr_second_dispatch_status pr_fourth_store_return_start(pr_fourth_store_return_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fourth_store_return_diagnostic *diagnostic);
pr_second_dispatch_status pr_fourth_store_return_resume(pr_fourth_store_return_session *state,pr_fourth_store_return_diagnostic *diagnostic);
#endif
