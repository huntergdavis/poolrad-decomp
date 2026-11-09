#ifndef POOLRAD_REPLAY_U68_290B_ASSEMBLY_H
#define POOLRAD_REPLAY_U68_290B_ASSEMBLY_H
#include "poolrad/replay/u68_290b_branch.h"
pr_290b_first_status pr_290b_assembly(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_U68_290B_ASSEMBLY_EMPTY=0,PR_U68_290B_ASSEMBLY_WAITING,PR_U68_290B_ASSEMBLY_FINISHED } pr_u68_290b_assembly_phase;
typedef enum { PR_U68_290B_ASSEMBLY_YIELD=0,PR_U68_290B_ASSEMBLY_REACHED,PR_U68_290B_ASSEMBLY_ARGUMENT,PR_U68_290B_ASSEMBLY_STOP } pr_u68_290b_assembly_status;
typedef struct { pr_u68_290b_assembly_phase phase;pr_u68_290b_branch branch;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u68_290b_assembly;
typedef struct { pr_u68_290b_branch_status branch_status;pr_u68_290b_branch_diagnostic branch;pr_290b_first_status assembly_status;pr_290b_first_diagnostic assembly; } pr_u68_290b_assembly_diagnostic;
void pr_u68_290b_assembly_init(pr_u68_290b_assembly *state);
pr_u68_290b_assembly_status pr_u68_290b_assembly_start(pr_u68_290b_assembly *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,pr_u68_290b_assembly_diagnostic *diagnostic);
pr_u68_290b_assembly_status pr_u68_290b_assembly_resume(pr_u68_290b_assembly *state,pr_u68_290b_assembly_diagnostic *diagnostic);
#endif
