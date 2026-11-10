#ifndef POOLRAD_REPLAY_U1C_FIFTH_HANDLER_RETURN_H
#define POOLRAD_REPLAY_U1C_FIFTH_HANDLER_RETURN_H
#include "poolrad/replay/u25_fifth_store_return.h"
#include "poolrad/replay/u1c_save_handler_return.h"
enum { PR_FIFTH_HANDLER_RETURN_REACHED=PR_FIFTH_STORE_RETURN_REACHED+1 };
typedef enum { PR_FIFTH_HANDLER_EMPTY=0,PR_FIFTH_HANDLER_WAITING,PR_FIFTH_HANDLER_FINISHED } pr_fifth_handler_phase;
typedef struct { pr_fifth_handler_phase phase;pr_fifth_store_return_session store;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_return_target target; } pr_fifth_handler_session;
typedef struct { pr_second_dispatch_status store_status;pr_fifth_store_return_diagnostic store;pr_290b_first_status return_status; } pr_fifth_handler_diagnostic;
void pr_fifth_handler_session_init(pr_fifth_handler_session *state);
pr_second_dispatch_status pr_fifth_handler_start(pr_fifth_handler_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fifth_handler_diagnostic *diagnostic);
pr_second_dispatch_status pr_fifth_handler_resume(pr_fifth_handler_session *state,pr_fifth_handler_diagnostic *diagnostic);
#endif
