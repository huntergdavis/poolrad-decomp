#ifndef POOLRAD_REPLAY_U68_290B_TWO_FIELDS_H
#define POOLRAD_REPLAY_U68_290B_TWO_FIELDS_H
#include "poolrad/replay/u68_290b_first_field.h"
/* Runs the source second field from the live +29A6 boundary to +2A02.
 * Uses the first-field status and diagnostic types; see RES_290B_TWO_FIELDS.md. */
pr_290b_first_status pr_290b_second_field(pr_bus *bus,size_t ram_size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_U68_290B_TWO_EMPTY=0,PR_U68_290B_TWO_WAITING,PR_U68_290B_TWO_FINISHED } pr_u68_290b_two_phase;
typedef enum { PR_U68_290B_TWO_YIELD=0,PR_U68_290B_TWO_FIELDS,PR_U68_290B_TWO_ARGUMENT,PR_U68_290B_TWO_STOP } pr_u68_290b_two_status;
typedef struct { pr_u68_290b_two_phase phase;pr_u68_290b first;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u68_290b_two;
typedef struct { pr_u68_290b_status first_status;pr_u68_290b_diagnostic first;
    pr_290b_first_status second_status;pr_290b_first_diagnostic second; } pr_u68_290b_two_diagnostic;
void pr_u68_290b_two_init(pr_u68_290b_two *state);
pr_u68_290b_two_status pr_u68_290b_two_start(pr_u68_290b_two *state,pr_bus *bus,size_t ram_size,
    pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,pr_u68_290b_two_diagnostic *diagnostic);
pr_u68_290b_two_status pr_u68_290b_two_resume(pr_u68_290b_two *state,pr_u68_290b_two_diagnostic *diagnostic);
#endif
