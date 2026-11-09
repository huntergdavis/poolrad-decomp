#ifndef POOLRAD_REPLAY_U68_290B_MODE_H
#define POOLRAD_REPLAY_U68_290B_MODE_H
#include "poolrad/replay/u68_290b_assembly.h"
pr_290b_first_status pr_290b_mode(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_U68_290B_MODE_EMPTY=0,PR_U68_290B_MODE_WAITING,PR_U68_290B_MODE_FINISHED } pr_u68_290b_mode_phase;
typedef enum { PR_U68_290B_MODE_YIELD=0,PR_U68_290B_MODE_REACHED,PR_U68_290B_MODE_ARGUMENT,PR_U68_290B_MODE_STOP } pr_u68_290b_mode_status;
typedef struct { pr_u68_290b_mode_phase phase;pr_u68_290b_assembly assembly;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u68_290b_mode;
typedef struct { pr_u68_290b_assembly_status assembly_status;pr_u68_290b_assembly_diagnostic assembly;pr_290b_first_status mode_status;pr_290b_first_diagnostic mode; } pr_u68_290b_mode_diagnostic;
void pr_u68_290b_mode_init(pr_u68_290b_mode *state);
pr_u68_290b_mode_status pr_u68_290b_mode_start(pr_u68_290b_mode *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,pr_u68_290b_mode_diagnostic *diagnostic);
pr_u68_290b_mode_status pr_u68_290b_mode_resume(pr_u68_290b_mode *state,pr_u68_290b_mode_diagnostic *diagnostic);
#endif
