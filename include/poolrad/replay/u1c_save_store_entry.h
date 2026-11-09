#ifndef POOLRAD_REPLAY_U1C_SAVE_STORE_ENTRY_H
#define POOLRAD_REPLAY_U1C_SAVE_STORE_ENTRY_H
#include "poolrad/replay/u25_slot_value_return.h"
pr_290b_first_status pr_u1c_save_store_entry(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_SAVE_STORE_EMPTY=0,PR_SAVE_STORE_WAITING,PR_SAVE_STORE_FINISHED } pr_save_store_phase;
typedef enum { PR_SAVE_STORE_YIELD=0,PR_SAVE_STORE_REACHED,PR_SAVE_STORE_ZERO,PR_SAVE_STORE_WORD,PR_SAVE_STORE_OTHER_TYPE,PR_SAVE_STORE_STRING,PR_SAVE_STORE_LOADER_STRING,PR_SAVE_STORE_LIMIT,PR_SAVE_STORE_PROMPT,PR_SAVE_STORE_OTHER,PR_SAVE_STORE_ARGUMENT,PR_SAVE_STORE_STOP } pr_save_store_status;
typedef struct { pr_save_store_phase phase;pr_value_return_session value;pr_bus *bus;pr_r0899_cpu *cpu; } pr_save_store_session;
typedef struct { pr_value_return_status value_status;pr_value_return_diagnostic value;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_save_store_diagnostic;
void pr_save_store_session_init(pr_save_store_session *state);
pr_save_store_status pr_save_store_start(pr_save_store_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_save_store_diagnostic *diagnostic);
pr_save_store_status pr_save_store_resume(pr_save_store_session *state,pr_save_store_diagnostic *diagnostic);
#endif
