#ifndef POOLRAD_REPLAY_U1C_FOURTH_STORE_ENTRY_H
#define POOLRAD_REPLAY_U1C_FOURTH_STORE_ENTRY_H
#include "poolrad/replay/u25_fourth_value_return.h"
#include "poolrad/replay/u1c_save_store_entry.h"
typedef enum { PR_FOURTH_STORE_EMPTY=0,PR_FOURTH_STORE_WAITING,PR_FOURTH_STORE_FINISHED } pr_fourth_store_phase;
typedef struct { pr_fourth_store_phase phase;pr_fourth_value_session value;pr_bus *bus;pr_r0899_cpu *cpu; } pr_fourth_store_session;
typedef struct { pr_second_dispatch_status value_status;pr_fourth_value_diagnostic value;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_fourth_store_diagnostic;
void pr_fourth_store_session_init(pr_fourth_store_session *state);
pr_second_dispatch_status pr_fourth_store_start(pr_fourth_store_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fourth_store_diagnostic *diagnostic);
pr_second_dispatch_status pr_fourth_store_resume(pr_fourth_store_session *state,pr_fourth_store_diagnostic *diagnostic);
#endif
