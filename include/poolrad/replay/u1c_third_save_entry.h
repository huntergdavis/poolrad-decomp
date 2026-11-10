#ifndef POOLRAD_REPLAY_U1C_THIRD_SAVE_ENTRY_H
#define POOLRAD_REPLAY_U1C_THIRD_SAVE_ENTRY_H
#include "poolrad/replay/u1c_third_dispatch.h"
#include "poolrad/replay/u1c_save_loader_entry.h"
enum { PR_THIRD_SAVE_LOADER=PR_THIRD_DISPATCH_LATER+1,PR_THIRD_SAVE_OTHER_HANDLER };
typedef enum { PR_THIRD_SAVE_EMPTY=0,PR_THIRD_SAVE_WAITING,PR_THIRD_SAVE_FINISHED } pr_third_save_phase;
typedef struct { pr_third_save_phase phase;pr_third_dispatch_session dispatch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_third_save_session;
typedef struct { pr_second_dispatch_status dispatch_status;pr_third_dispatch_diagnostic dispatch;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_third_save_diagnostic;
void pr_third_save_session_init(pr_third_save_session *state);
pr_second_dispatch_status pr_third_save_start(pr_third_save_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_third_save_diagnostic *diagnostic);
pr_second_dispatch_status pr_third_save_resume(pr_third_save_session *state,pr_third_save_diagnostic *diagnostic);
#endif
