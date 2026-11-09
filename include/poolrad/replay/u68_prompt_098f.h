#ifndef POOLRAD_REPLAY_U68_PROMPT_098F_H
#define POOLRAD_REPLAY_U68_PROMPT_098F_H
#include "poolrad/replay/u3b0_prompt_session.h"
#include "poolrad/recovered/overlay_u68_up_preflight_068d_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
typedef enum { PR_U68_PROMPT_EMPTY=0, PR_U68_PROMPT_WAITING, PR_U68_PROMPT_FINISHED } pr_u68_prompt_phase;
typedef enum { PR_U68_PROMPT_YIELD=0, PR_U68_PROMPT_DISPLAY, PR_U68_PROMPT_OUTSIDE,
    PR_U68_PROMPT_ARGUMENT, PR_U68_PROMPT_CODE, PR_U68_PROMPT_PROFILE,
    PR_U68_PROMPT_OVERLAY, PR_U68_PROMPT_CALLEE, PR_U68_PROMPT_TRANSFER, PR_U68_PROMPT_STOP } pr_u68_prompt_status;
typedef struct { pr_u68_prompt_phase phase;pr_u3b0_prompt_session prompt;pr_bus *bus;pr_r0899_cpu *cpu;
    const pr_u3b0_clock_draw_env *env;uint16_t caller_cs,caller_bp,caller_sp; } pr_u68_prompt;
typedef struct { int stage;pr_system_leaf_status strload_status;pr_u3b0_session_status session_status;
    pr_u3b0_session_diagnostic session;pr_u68_068d_status preflight_status; } pr_u68_prompt_diagnostic;
/* Caller supplies the live +098F U68 frame, loaded code/stubs and services.
 * Only the selector-1, extended-Up route reaches the unresolved +0B9B call.
 * See U68_PROMPT_098F_DIAGNOSTIC.md for profiles and continuation limits. */
void pr_u68_prompt_init(pr_u68_prompt *state);
pr_u68_prompt_status pr_u68_prompt_start(pr_u68_prompt *state,pr_bus *bus,size_t ram_size,
    pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,pr_u68_prompt_diagnostic *diagnostic);
pr_u68_prompt_status pr_u68_prompt_resume(pr_u68_prompt *state,pr_u68_prompt_diagnostic *diagnostic);
#endif
