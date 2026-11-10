#ifndef POOLRAD_REPLAY_U1C_THIRD_STORE_ENTRY_H
#define POOLRAD_REPLAY_U1C_THIRD_STORE_ENTRY_H
#include "poolrad/replay/u25_third_value_return.h"
#include "poolrad/replay/u1c_save_store_entry.h"
typedef enum { PR_THIRD_STORE_EMPTY=0,PR_THIRD_STORE_WAITING,PR_THIRD_STORE_FINISHED } pr_third_store_phase;
typedef struct { pr_third_store_phase phase;pr_third_value_session value;pr_bus *bus;pr_r0899_cpu *cpu; } pr_third_store_session;
typedef struct { pr_second_dispatch_status value_status;pr_third_value_diagnostic value;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_third_store_diagnostic;
void pr_third_store_session_init(pr_third_store_session *state);
pr_second_dispatch_status pr_third_store_start(pr_third_store_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_third_store_diagnostic *diagnostic);
pr_second_dispatch_status pr_third_store_resume(pr_third_store_session *state,pr_third_store_diagnostic *diagnostic);
#endif
