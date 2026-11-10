#ifndef POOLRAD_REPLAY_U25_THIRD_CLASS4_LOW_H
#define POOLRAD_REPLAY_U25_THIRD_CLASS4_LOW_H
#include "poolrad/replay/u25_third_store_dispatch.h"
#include "poolrad/replay/u25_store_class4_low.h"
enum { PR_THIRD_CLASS4_LOW_REACHED=PR_THIRD_DISPATCH_UNMATCHED+1,PR_THIRD_CLASS4_LOW_HIGH };
typedef enum { PR_THIRD_CLASS4_EMPTY=0,PR_THIRD_CLASS4_WAITING,PR_THIRD_CLASS4_FINISHED } pr_third_class4_phase;
typedef struct { pr_third_class4_phase phase;pr_third_store_dispatch_session dispatch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_third_class4_session;
typedef struct { pr_second_dispatch_status dispatch_status;pr_third_store_dispatch_diagnostic dispatch;pr_290b_first_status low_status; } pr_third_class4_diagnostic;
void pr_third_class4_session_init(pr_third_class4_session *state);
pr_second_dispatch_status pr_third_class4_start(pr_third_class4_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_third_class4_diagnostic *diagnostic);
pr_second_dispatch_status pr_third_class4_resume(pr_third_class4_session *state,pr_third_class4_diagnostic *diagnostic);
#endif
