#ifndef POOLRAD_REPLAY_SYSTEM_GETMEM_REAL_HOOK_ABI_H
#define POOLRAD_REPLAY_SYSTEM_GETMEM_REAL_HOOK_ABI_H
#include "poolrad/replay/system_getmem_23f_abi.h"
typedef enum {
    PR_GETMEM_REAL_HOOK_STEPPED = 0,
    PR_GETMEM_REAL_HOOK_BLOCKED_STRING_COPY,
    PR_GETMEM_REAL_HOOK_BLOCKED_POSTHOOK,
    PR_GETMEM_REAL_HOOK_BLOCKED_WRAPPER,
    PR_GETMEM_REAL_HOOK_BLOCKED_TARGET,
    PR_GETMEM_REAL_HOOK_BLOCKED_MEMORY,
    PR_GETMEM_REAL_HOOK_BLOCKED_PC,
    PR_GETMEM_REAL_HOOK_ARGUMENT
} pr_getmem_real_hook_status;
/* One original instruction from actual SYSTEM :03B7/:03C3 indirect calls,
 * loaded 083A:0025 thunk and 398A:0042 callback prefix/zero-argument return.
 * Live DS:[4414] must select the audited thunk; unknown targets block before
 * a far frame is pushed. Actual zero-argument AX is read from [BP-2], an
 * uninitialized local; no returned AL is supplied or inferred.
 * Nonzero argument blocks at 398A:005F before SYSTEM AF8:0B0B (ret 0064).
 * Growth return :03BB..03BE consumes the real allocated-pointer frame.
 * :03C7 retry/error selection and :0248 wrapper remain explicit gates.
 * Requires pinned code and live CPU/RAM with state/report/RAM nonoverlapping,
 * RAM <= 1 MiB and spans inside 64 KiB segments. Gates/memory errors
 * preserve CPU/RAM; NULL arguments preserve report. No allocator outcome,
 * post-hook AL injection, snapshot bridge, full-core or count claim. */
pr_getmem_real_hook_status pr_system_getmem_real_hook_step(
    pr_slot_a_directory_state *state, pr_system_getmem_report *report);
#endif
