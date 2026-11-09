#ifndef POOLRAD_REPLAY_U25_SLOT_PREFIX_H
#define POOLRAD_REPLAY_U25_SLOT_PREFIX_H
#include "poolrad/replay/u1c_save_slot_entry.h"
pr_290b_first_status pr_u25_slot_prefix(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
typedef enum { PR_SLOT_PREFIX_EMPTY=0,PR_SLOT_PREFIX_WAITING,PR_SLOT_PREFIX_FINISHED } pr_slot_prefix_phase;
typedef enum { PR_SLOT_PREFIX_YIELD=0,PR_SLOT_PREFIX_REACHED,PR_SLOT_PREFIX_STRING,PR_SLOT_PREFIX_LOADER_STRING,PR_SLOT_PREFIX_LIMIT,PR_SLOT_PREFIX_PROMPT,PR_SLOT_PREFIX_OTHER,PR_SLOT_PREFIX_ARGUMENT,PR_SLOT_PREFIX_STOP } pr_slot_prefix_status;
typedef struct { pr_slot_prefix_phase phase;pr_save_slot_session slot;pr_bus *bus;pr_r0899_cpu *cpu; } pr_slot_prefix_session;
typedef struct { pr_save_slot_status slot_status;pr_save_slot_diagnostic slot;pr_290b_first_status prefix_status; } pr_slot_prefix_diagnostic;
void pr_slot_prefix_session_init(pr_slot_prefix_session *state);
pr_slot_prefix_status pr_slot_prefix_start(pr_slot_prefix_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_slot_prefix_diagnostic *diagnostic);
pr_slot_prefix_status pr_slot_prefix_resume(pr_slot_prefix_session *state,pr_slot_prefix_diagnostic *diagnostic);
#endif
