#ifndef POOLRAD_REPLAY_U25_FIFTH_PAIR_ENTRY_H
#define POOLRAD_REPLAY_U25_FIFTH_PAIR_ENTRY_H
#include "poolrad/replay/u25_fifth_slot_dispatch.h"
#include "poolrad/replay/u25_slot_pair_entry.h"
typedef enum { PR_FIFTH_PAIR_EMPTY=0,PR_FIFTH_PAIR_WAITING,PR_FIFTH_PAIR_FINISHED } pr_fifth_pair_phase;
typedef struct { pr_fifth_pair_phase phase;pr_fifth_slot_session slot;pr_bus *bus;pr_r0899_cpu *cpu; } pr_fifth_pair_session;
typedef struct { pr_second_dispatch_status slot_status;pr_fifth_slot_diagnostic slot;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_fifth_pair_diagnostic;
void pr_fifth_pair_session_init(pr_fifth_pair_session *state);
pr_second_dispatch_status pr_fifth_pair_start(pr_fifth_pair_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fifth_pair_diagnostic *diagnostic);
pr_second_dispatch_status pr_fifth_pair_resume(pr_fifth_pair_session *state,pr_fifth_pair_diagnostic *diagnostic);
#endif
