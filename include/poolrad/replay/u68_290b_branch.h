#ifndef POOLRAD_REPLAY_U68_290B_BRANCH_H
#define POOLRAD_REPLAY_U68_290B_BRANCH_H
#include "poolrad/replay/u68_290b_two_fields.h"
/* Live +2A02 record branch through +2A51; see branch documentation. */
pr_290b_first_status pr_290b_record_branch(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_U68_290B_BRANCH_EMPTY=0,PR_U68_290B_BRANCH_WAITING,PR_U68_290B_BRANCH_FINISHED } pr_u68_290b_branch_phase;
typedef enum { PR_U68_290B_BRANCH_YIELD=0,PR_U68_290B_BRANCH_REACHED,PR_U68_290B_BRANCH_ARGUMENT,PR_U68_290B_BRANCH_STOP } pr_u68_290b_branch_status;
typedef struct { pr_u68_290b_branch_phase phase;pr_u68_290b_two two;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u68_290b_branch;
typedef struct { pr_u68_290b_two_status two_status;pr_u68_290b_two_diagnostic two;pr_290b_first_status branch_status;pr_290b_first_diagnostic branch; } pr_u68_290b_branch_diagnostic;
void pr_u68_290b_branch_init(pr_u68_290b_branch *state);
pr_u68_290b_branch_status pr_u68_290b_branch_start(pr_u68_290b_branch *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,pr_u68_290b_branch_diagnostic *diagnostic);
pr_u68_290b_branch_status pr_u68_290b_branch_resume(pr_u68_290b_branch *state,pr_u68_290b_branch_diagnostic *diagnostic);
#endif
