#ifndef POOLRAD_REPLAY_SYSTEM_GETMEM_308_GATE_H
#define POOLRAD_REPLAY_SYSTEM_GETMEM_308_GATE_H
#include "poolrad/replay/system_getmem_23f_abi.h"
typedef enum {
    PR_GETMEM_FLAGS_DOSBOX_X_59915C1 = 1,
    PR_GETMEM_FLAGS_UNICORN_2_1_4 = 2
} pr_getmem_flags_profile;
typedef enum {
    PR_GETMEM_CORE_STEPPED = 0,
    PR_GETMEM_CORE_BLOCKED_TAIL,
    PR_GETMEM_CORE_BLOCKED_RETURN,
    PR_GETMEM_CORE_BLOCKED_HOOK,
    PR_GETMEM_CORE_BLOCKED_MEMORY,
    PR_GETMEM_CORE_BLOCKED_PC,
    PR_GETMEM_CORE_ARGUMENT
} pr_getmem_core_status;
/* One original instruction in loaded SYSTEM 131C:0308 selection and
 * 0491 normalization. Required profile makes undefined SHR AF explicit:
 * count-one SHR sets AF in DOSBox-X 59915c1, clears it in Unicorn 2.1.4.
 * Stops before split/exact-fit tails :0363/:0376, wrapper return :0248,
 * and indirect far CALLs :03B7/:03C3 (before far-frame pushes). Hook report
 * gives the live DS:[4414] loaded target and original return PC.
 * No post-hook result injection, caller stores, or continuous replay bridge.
 * Audited code and live CPU/RAM required; state/report/RAM do not overlap.
 * RAM <= 1 MiB; each access fits its 64 KiB segment. Invalid/memory/gate
 * states preserve CPU/RAM; NULL arguments preserve report. */
pr_getmem_core_status pr_system_getmem_308_step(
    pr_slot_a_directory_state *state, pr_getmem_flags_profile profile,
    pr_system_getmem_report *report);
#endif
