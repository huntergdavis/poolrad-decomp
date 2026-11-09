#ifndef POOLRAD_REPLAY_U25_SECOND_VALUE_RETURN_H
#define POOLRAD_REPLAY_U25_SECOND_VALUE_RETURN_H
#include "poolrad/replay/u25_second_pair_return.h"
#include "poolrad/replay/u25_slot_value_return.h"
typedef enum { PR_SECOND_VALUE_EMPTY=0,PR_SECOND_VALUE_WAITING,PR_SECOND_VALUE_FINISHED } pr_second_value_phase;
typedef struct { pr_second_value_phase phase;pr_second_pair_return_session pair;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_return_target target; } pr_second_value_session;
typedef struct { pr_second_slot_status pair_status;pr_second_pair_return_diagnostic pair;pr_290b_first_status return_status; } pr_second_value_diagnostic;
void pr_second_value_session_init(pr_second_value_session *state);
pr_second_slot_status pr_second_value_start(pr_second_value_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_second_value_diagnostic *diagnostic);
pr_second_slot_status pr_second_value_resume(pr_second_value_session *state,pr_second_value_diagnostic *diagnostic);
#endif
