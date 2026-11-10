#ifndef POOLRAD_REPLAY_U1C_SIXTH_COMPARE_ENTRY_H
#define POOLRAD_REPLAY_U1C_SIXTH_COMPARE_ENTRY_H
#include "poolrad/replay/u1c_sixth_dispatch.h"
pr_290b_first_status pr_u1c_compare_loader_entry(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
enum { PR_SIXTH_COMPARE_LOADER=PR_SIXTH_DISPATCH_LATER+1,PR_SIXTH_COMPARE_OTHER_HANDLER };
typedef enum { PR_SIXTH_COMPARE_EMPTY=0,PR_SIXTH_COMPARE_WAITING,PR_SIXTH_COMPARE_FINISHED } pr_sixth_compare_phase;
typedef struct { pr_sixth_compare_phase phase;pr_sixth_dispatch_session dispatch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_sixth_compare_session;
typedef struct { pr_second_dispatch_status dispatch_status;pr_sixth_dispatch_diagnostic dispatch;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_sixth_compare_diagnostic;
void pr_sixth_compare_session_init(pr_sixth_compare_session *state);
pr_second_dispatch_status pr_sixth_compare_start(pr_sixth_compare_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_sixth_compare_diagnostic *diagnostic);
pr_second_dispatch_status pr_sixth_compare_resume(pr_sixth_compare_session *state,pr_sixth_compare_diagnostic *diagnostic);
#endif
