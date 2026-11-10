#ifndef POOLRAD_REPLAY_U25_FIFTH_SLOT_DISPATCH_H
#define POOLRAD_REPLAY_U25_FIFTH_SLOT_DISPATCH_H
#include "poolrad/replay/u1c_fifth_save_branch.h"
#include "poolrad/replay/u1c_save_slot_entry.h"
#include "poolrad/replay/u25_slot_dispatch.h"
enum { PR_FIFTH_SLOT_ZERO=PR_FIFTH_BRANCH_SAVE_STRING+1,PR_FIFTH_SLOT_WORD,PR_FIFTH_SLOT_PAIR,PR_FIFTH_SLOT_OTHER_TYPE };
typedef enum { PR_FIFTH_SLOT_EMPTY=0,PR_FIFTH_SLOT_WAITING,PR_FIFTH_SLOT_FINISHED } pr_fifth_slot_phase;
typedef struct { pr_fifth_slot_phase phase;pr_fifth_branch_session branch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_fifth_slot_session;
typedef struct { pr_second_dispatch_status branch_status;pr_fifth_branch_diagnostic branch;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry;pr_290b_first_status prefix_status;pr_290b_first_status dispatch_status; } pr_fifth_slot_diagnostic;
void pr_fifth_slot_session_init(pr_fifth_slot_session *state);
pr_second_dispatch_status pr_fifth_slot_start(pr_fifth_slot_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fifth_slot_diagnostic *diagnostic);
pr_second_dispatch_status pr_fifth_slot_resume(pr_fifth_slot_session *state,pr_fifth_slot_diagnostic *diagnostic);
#endif
