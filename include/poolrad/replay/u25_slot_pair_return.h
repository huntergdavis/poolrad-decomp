#ifndef POOLRAD_REPLAY_U25_SLOT_PAIR_RETURN_H
#define POOLRAD_REPLAY_U25_SLOT_PAIR_RETURN_H
#include "poolrad/replay/u25_slot_pair_entry.h"
typedef enum { PR_PAIR_RETURN_EMPTY=0,PR_PAIR_RETURN_WAITING,PR_PAIR_RETURN_FINISHED } pr_pair_return_phase;
typedef enum { PR_PAIR_RETURN_YIELD=0,PR_PAIR_RETURN_REACHED,PR_PAIR_RETURN_ZERO,PR_PAIR_RETURN_WORD,PR_PAIR_RETURN_OTHER_TYPE,PR_PAIR_RETURN_STRING,PR_PAIR_RETURN_LOADER_STRING,PR_PAIR_RETURN_LIMIT,PR_PAIR_RETURN_PROMPT,PR_PAIR_RETURN_OTHER,PR_PAIR_RETURN_ARGUMENT,PR_PAIR_RETURN_STOP } pr_pair_return_status;
typedef struct { pr_pair_return_phase phase;pr_slot_pair_session pair;pr_bus *bus;pr_r0899_cpu *cpu; } pr_pair_return_session;
typedef struct { pr_slot_pair_status pair_status;pr_slot_pair_diagnostic pair;pr_290b_first_status return_status; } pr_pair_return_diagnostic;
void pr_pair_return_session_init(pr_pair_return_session *state);
pr_pair_return_status pr_pair_return_start(pr_pair_return_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_pair_return_diagnostic *diagnostic);
pr_pair_return_status pr_pair_return_resume(pr_pair_return_session *state,pr_pair_return_diagnostic *diagnostic);
#endif
