#ifndef POOLRAD_REPLAY_U3B0_PROMPT_SESSION_H
#define POOLRAD_REPLAY_U3B0_PROMPT_SESSION_H
#include "poolrad/replay/u3b0_entry_03bd.h"

typedef enum { PR_U3B0_SESSION_EMPTY=0, PR_U3B0_SESSION_WAITING, PR_U3B0_SESSION_FINISHED } pr_u3b0_session_phase;
typedef enum { PR_U3B0_SESSION_YIELD=0, PR_U3B0_SESSION_RETURN, PR_U3B0_SESSION_STOP, PR_U3B0_SESSION_ARGUMENT } pr_u3b0_session_status;
typedef struct {
    pr_u3b0_session_phase phase;
    pr_bus *bus; pr_bus binding; pr_r0899_cpu *cpu, snapshot;
    const pr_u3b0_clock_draw_env *env;
} pr_u3b0_prompt_session;
typedef struct {
    int from_entry; /* 1: inspect entry; 0: inspect wait */
    pr_u3b0_entry_status entry_status; pr_u3b0_entry_diagnostic entry;
    pr_u3b0_wait_status wait_status; pr_u3b0_wait_diagnostic wait;
} pr_u3b0_session_diagnostic;
/* Initialize once; start executes +03BD. Resume only a yielded session at
 * its own verified no-event +048E boundary. The caller may enqueue keyboard
 * input between calls but must not mutate other guest state, bindings or
 * service configuration. All reached component profiles remain applicable.
 * See U3B0_PROMPT_SESSION.md. No entry2 provided or full prompt claim. */
void pr_u3b0_prompt_session_init(pr_u3b0_prompt_session *session);
pr_u3b0_session_status pr_u3b0_prompt_session_start(pr_u3b0_prompt_session *session,
    pr_bus *bus,size_t ram_size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,
    pr_u3b0_session_diagnostic *diagnostic);
pr_u3b0_session_status pr_u3b0_prompt_session_resume(pr_u3b0_prompt_session *session,
    pr_u3b0_session_diagnostic *diagnostic);
#endif
