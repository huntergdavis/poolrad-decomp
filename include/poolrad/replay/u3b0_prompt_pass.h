#ifndef POOLRAD_REPLAY_U3B0_PROMPT_PASS_H
#define POOLRAD_REPLAY_U3B0_PROMPT_PASS_H
#include "poolrad/replay/u3b0_poll_0566.h"
#include "poolrad/replay/u3b0_character_059c.h"
#include "poolrad/replay/u3b0_selection_05c4.h"
#include "poolrad/replay/u3b0_mapped_060c.h"
#include "poolrad/replay/u3b0_text_0634.h"
#include "poolrad/replay/u3b0_return_06cf.h"

typedef enum { PR_U3B0_PASS_RETURN=0, PR_U3B0_PASS_LOOP, PR_U3B0_PASS_ARGUMENT,
    PR_U3B0_PASS_PROFILE, PR_U3B0_PASS_STOP } pr_u3b0_pass_status;
/* On PROFILE, diagnostic.status is one of these reasons. */
typedef enum { PR_U3B0_PASS_PROFILE_SEGMENT=1, PR_U3B0_PASS_PROFILE_FLAG_SEGMENT,
    PR_U3B0_PASS_PROFILE_FRAME, PR_U3B0_PASS_PROFILE_RETURN_APERTURE,
    PR_U3B0_PASS_PROFILE_FLAG_POINTER_ALIAS } pr_u3b0_pass_profile_reason;
typedef enum { PR_U3B0_PASS_STAGE_ENTRY=0, PR_U3B0_PASS_STAGE_POLL,
    PR_U3B0_PASS_STAGE_CHARACTER, PR_U3B0_PASS_STAGE_SELECTION,
    PR_U3B0_PASS_STAGE_MAPPED, PR_U3B0_PASS_STAGE_TEXT,
    PR_U3B0_PASS_STAGE_RETURN_GUARD, PR_U3B0_PASS_STAGE_RETURN } pr_u3b0_pass_stage;
typedef struct {
    pr_u3b0_poll_env poll;pr_u3b0_mapped_env mapped;
    pr_u3b0_01da_limits selection;pr_u3b0_text_limits text;
    pr_u3b0_return_target target;
} pr_u3b0_pass_env;
typedef struct {
    pr_u3b0_pass_stage stage;int status;
    pr_u3b0_poll_diagnostic poll;pr_u3b0_selection_diagnostic selection;
    pr_u3b0_mapped_diagnostic mapped;pr_u3b0_text_diagnostic text;
} pr_u3b0_pass_diagnostic;
/* One source-bounded pass from a caller-supplied live +0566 frame to the
 * +048E loop boundary or an actual restricted epilogue RETF. Caller owns
 * CPU, RAM/EGA bus, all services, stable code, all earlier prompt effects,
 * and the expected return. It must satisfy every component's profile and
 * ensure the flag-pointer storage remains unchanged until the poll's write;
 * that provenance/stability is not established here. Flat-RAM components
 * require all their reached reads/writes, including nested services, outside
 * EGA aperture. No arbitrary-PC resume, entry2 provided or full prompt claim.
 * All completed effects remain on STOP/PROFILE; inspect diagnostic.stage. */
pr_u3b0_pass_status pr_u3b0_prompt_pass(pr_bus *bus,size_t ram_size,
    pr_r0899_cpu *cpu,const pr_u3b0_pass_env *env,pr_u3b0_pass_diagnostic *diagnostic);
#endif
