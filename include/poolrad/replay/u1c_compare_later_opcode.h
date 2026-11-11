#ifndef POOLRAD_REPLAY_U1C_COMPARE_LATER_OPCODE_H
#define POOLRAD_REPLAY_U1C_COMPARE_LATER_OPCODE_H
#include "poolrad/replay/u1c_compare_next_dispatch.h"
pr_290b_first_status pr_u1c_later_opcode_checks(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
enum { PR_COMPARE_LATER_HANDLER=PR_COMPARE_NEXT_DISPATCH_LATER+1,PR_COMPARE_LATER_UNMATCHED };
typedef enum { PR_COMPARE_LATER_EMPTY=0,PR_COMPARE_LATER_WAITING,PR_COMPARE_LATER_FINISHED } pr_compare_later_phase;
typedef struct { pr_compare_later_phase phase;pr_compare_next_dispatch_session dispatch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_compare_later_session;
typedef struct { pr_second_dispatch_status dispatch_status;pr_compare_next_dispatch_diagnostic dispatch;pr_290b_first_status checks_status; } pr_compare_later_diagnostic;
void pr_compare_later_session_init(pr_compare_later_session *state);
pr_second_dispatch_status pr_compare_later_start(pr_compare_later_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_compare_later_diagnostic *diagnostic);
pr_second_dispatch_status pr_compare_later_resume(pr_compare_later_session *state,pr_compare_later_diagnostic *diagnostic);
#endif
