#ifndef POOLRAD_REPLAY_U25_FOURTH_SLOT_DISPATCH_H
#define POOLRAD_REPLAY_U25_FOURTH_SLOT_DISPATCH_H
#include "poolrad/replay/u1c_fourth_save_branch.h"
#include "poolrad/replay/u1c_save_slot_entry.h"
#include "poolrad/replay/u25_slot_dispatch.h"
enum { PR_FOURTH_SLOT_ZERO=PR_FOURTH_BRANCH_SAVE_STRING+1,PR_FOURTH_SLOT_WORD,PR_FOURTH_SLOT_PAIR,PR_FOURTH_SLOT_OTHER_TYPE };
typedef enum { PR_FOURTH_SLOT_EMPTY=0,PR_FOURTH_SLOT_WAITING,PR_FOURTH_SLOT_FINISHED } pr_fourth_slot_phase;
typedef struct { pr_fourth_slot_phase phase;pr_fourth_branch_session branch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_fourth_slot_session;
typedef struct { pr_second_dispatch_status branch_status;pr_fourth_branch_diagnostic branch;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry;pr_290b_first_status prefix_status;pr_290b_first_status dispatch_status; } pr_fourth_slot_diagnostic;
void pr_fourth_slot_session_init(pr_fourth_slot_session *state);
pr_second_dispatch_status pr_fourth_slot_start(pr_fourth_slot_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fourth_slot_diagnostic *diagnostic);
pr_second_dispatch_status pr_fourth_slot_resume(pr_fourth_slot_session *state,pr_fourth_slot_diagnostic *diagnostic);
#endif
