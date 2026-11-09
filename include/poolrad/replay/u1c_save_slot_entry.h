#ifndef POOLRAD_REPLAY_U1C_SAVE_SLOT_ENTRY_H
#define POOLRAD_REPLAY_U1C_SAVE_SLOT_ENTRY_H
#include "poolrad/replay/u1c_save_tag_branch.h"
pr_290b_first_status pr_u1c_save_slot_entry(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_SAVE_SLOT_EMPTY=0,PR_SAVE_SLOT_WAITING,PR_SAVE_SLOT_FINISHED } pr_save_slot_phase;
typedef enum { PR_SAVE_SLOT_YIELD=0,PR_SAVE_SLOT_REACHED,PR_SAVE_SLOT_STRING,PR_SAVE_SLOT_LOADER_STRING,PR_SAVE_SLOT_LIMIT,PR_SAVE_SLOT_PROMPT,PR_SAVE_SLOT_OTHER,PR_SAVE_SLOT_ARGUMENT,PR_SAVE_SLOT_STOP } pr_save_slot_status;
typedef struct { pr_save_slot_phase phase;pr_save_tag_session branch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_save_slot_session;
typedef struct { pr_save_tag_status branch_status;pr_save_tag_diagnostic branch;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_save_slot_diagnostic;
void pr_save_slot_session_init(pr_save_slot_session *state);
pr_save_slot_status pr_save_slot_start(pr_save_slot_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_save_slot_diagnostic *diagnostic);
pr_save_slot_status pr_save_slot_resume(pr_save_slot_session *state,pr_save_slot_diagnostic *diagnostic);
#endif
