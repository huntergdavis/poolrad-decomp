#ifndef POOLRAD_REPLAY_U1C_SAVE_LOADER_ENTRY_H
#define POOLRAD_REPLAY_U1C_SAVE_LOADER_ENTRY_H
#include "poolrad/replay/u1c_ecl_dispatch_prefix.h"
pr_290b_first_status pr_u1c_save_loader_entry(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_U1C_SAVE_EMPTY=0,PR_U1C_SAVE_WAITING,PR_U1C_SAVE_FINISHED } pr_u1c_save_phase;
typedef enum { PR_U1C_SAVE_YIELD=0,PR_U1C_SAVE_LOADER,PR_U1C_SAVE_LOOP,PR_U1C_SAVE_OTHER,PR_U1C_SAVE_ARGUMENT,PR_U1C_SAVE_STOP } pr_u1c_save_status;
typedef struct { pr_u1c_save_phase phase;pr_u1c_ecl_session ecl;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u1c_save_session;
typedef struct { pr_u1c_ecl_status ecl_status;pr_u1c_ecl_diagnostic ecl;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_u1c_save_diagnostic;
void pr_u1c_save_session_init(pr_u1c_save_session *state);
pr_u1c_save_status pr_u1c_save_start(pr_u1c_save_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,pr_u1c_save_diagnostic *diagnostic);
pr_u1c_save_status pr_u1c_save_resume(pr_u1c_save_session *state,pr_u1c_save_diagnostic *diagnostic);
#endif
