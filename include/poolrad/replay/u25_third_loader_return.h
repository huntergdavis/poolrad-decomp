#ifndef POOLRAD_REPLAY_U25_THIRD_LOADER_RETURN_H
#define POOLRAD_REPLAY_U25_THIRD_LOADER_RETURN_H
#include "poolrad/replay/u1c_third_save_entry.h"
#include "poolrad/replay/u25_operand_return.h"
enum { PR_THIRD_LOADER_REACHED=PR_THIRD_SAVE_OTHER_HANDLER+1,PR_THIRD_LOADER_STRING,PR_THIRD_LOADER_LIMIT };
typedef enum { PR_THIRD_LOADER_EMPTY=0,PR_THIRD_LOADER_WAITING,PR_THIRD_LOADER_FINISHED } pr_third_loader_phase;
typedef struct { pr_third_loader_phase phase;pr_third_save_session save;pr_bus *bus;pr_r0899_cpu *cpu;size_t max_iterations;pr_290b_return_target target; } pr_third_loader_session;
typedef struct { pr_second_dispatch_status save_status;pr_third_save_diagnostic save;pr_290b_first_status prefix_status;pr_290b_first_status tag_status;pr_u25_loop_step_status loop_status;pr_u25_loop_step_diagnostic loop;pr_290b_first_status return_status; } pr_third_loader_diagnostic;
void pr_third_loader_session_init(pr_third_loader_session *state);
pr_second_dispatch_status pr_third_loader_start(pr_third_loader_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_third_loader_diagnostic *diagnostic);
pr_second_dispatch_status pr_third_loader_resume(pr_third_loader_session *state,pr_third_loader_diagnostic *diagnostic);
#endif
