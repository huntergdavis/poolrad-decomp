#ifndef POOLRAD_REPLAY_U68_290B_FIRST_FIELD_H
#define POOLRAD_REPLAY_U68_290B_FIRST_FIELD_H
#include "poolrad/replay/u68_prompt_098f.h"
#include "poolrad/recovered/resident_00ba_word_string_1073_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
typedef enum { PR_290B_FIRST_DONE=0, PR_290B_FIRST_ARGUMENT, PR_290B_FIRST_OUTSIDE,
    PR_290B_FIRST_CODE, PR_290B_FIRST_PROFILE, PR_290B_FIRST_CALLEE, PR_290B_FIRST_TRANSFER } pr_290b_first_status;
typedef struct { int stage,status;pr_res_1073_callee formatter_callee;int formatter_status; } pr_290b_first_diagnostic;
/* Starts after the real far call at 290B. Stops before +29A6, after the first
 * field and its optional padding; see RES_290B_FIRST_FIELD.md. */
pr_290b_first_status pr_290b_first_field(pr_bus *bus,size_t ram_size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_U68_290B_EMPTY=0,PR_U68_290B_WAITING,PR_U68_290B_FINISHED } pr_u68_290b_phase;
typedef enum { PR_U68_290B_YIELD=0,PR_U68_290B_FIRST_FIELD,PR_U68_290B_ARGUMENT,
    PR_U68_290B_STOP,PR_U68_290B_CODE,PR_U68_290B_PROFILE } pr_u68_290b_status;
typedef struct { pr_u68_290b_phase phase;pr_u68_prompt u68;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u68_290b;
typedef struct { pr_u68_prompt_status u68_status;pr_u68_prompt_diagnostic u68;
    pr_290b_first_status first_status;pr_290b_first_diagnostic first; } pr_u68_290b_diagnostic;
/* Composes U68 +098F through real prompt/preflight and first 290B field.
 * Caller owns the live frame, code, data and services; stops are terminal. */
void pr_u68_290b_init(pr_u68_290b *state);
pr_u68_290b_status pr_u68_290b_start(pr_u68_290b *state,pr_bus *bus,size_t ram_size,
    pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,pr_u68_290b_diagnostic *diagnostic);
pr_u68_290b_status pr_u68_290b_resume(pr_u68_290b *state,pr_u68_290b_diagnostic *diagnostic);
#endif
