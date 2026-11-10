#ifndef POOLRAD_REPLAY_U25_THIRD_PAIR_RETURN_H
#define POOLRAD_REPLAY_U25_THIRD_PAIR_RETURN_H
#include "poolrad/replay/u25_third_pair_entry.h"
#include "poolrad/replay/u25_word_return.h"
typedef enum { PR_THIRD_PAIR_RETURN_EMPTY=0,PR_THIRD_PAIR_RETURN_WAITING,PR_THIRD_PAIR_RETURN_FINISHED } pr_third_pair_return_phase;
typedef struct { pr_third_pair_return_phase phase;pr_third_pair_session pair;pr_bus *bus;pr_r0899_cpu *cpu; } pr_third_pair_return_session;
typedef struct { pr_second_dispatch_status pair_status;pr_third_pair_diagnostic pair;pr_290b_first_status return_status; } pr_third_pair_return_diagnostic;
void pr_third_pair_return_session_init(pr_third_pair_return_session *state);
pr_second_dispatch_status pr_third_pair_return_start(pr_third_pair_return_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_third_pair_return_diagnostic *diagnostic);
pr_second_dispatch_status pr_third_pair_return_resume(pr_third_pair_return_session *state,pr_third_pair_return_diagnostic *diagnostic);
#endif
