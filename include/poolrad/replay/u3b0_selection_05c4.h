#ifndef POOLRAD_REPLAY_U3B0_SELECTION_05C4_H
#define POOLRAD_REPLAY_U3B0_SELECTION_05C4_H
#include "poolrad/recovered/overlay_u3b0_entry_01da_cpu.h"
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"

typedef enum {
    PR_U3B0_SELECTION_DRAWN = 0, /* stopped at +06CF */
    PR_U3B0_SELECTION_OTHER,     /* stopped before +060C */
    PR_U3B0_SELECTION_ARGUMENT,
    PR_U3B0_SELECTION_CODE,      /* stopped at reached near CALL */
    PR_U3B0_SELECTION_CALLEE,    /* nested drawer stopped */
    PR_U3B0_SELECTION_TRANSFER   /* drawer did not return to expected frame */
} pr_u3b0_selection_status;
typedef struct {
    uint16_t call_site;
    pr_u3b0_01da_status status;
    pr_u3b0_01da_diagnostic drawer;
} pr_u3b0_selection_diagnostic;
/* Entry +05C4 with live CPU/RAM/EGA bus, source identity and all prior
 * prompt effects supplied by caller. Comma/period invoke the real +01DA
 * drawer; other input stops at +060C. Shared RAM/EGA and completed earlier
 * effects are never rolled back on CODE, CALLEE or TRANSFER. The caller
 * must satisfy the drawer's documented non-aperture, code immutability,
 * storage separation and callback conditions. No prompt/entry2 completion. */
pr_u3b0_selection_status pr_u3b0_selection_05c4(pr_bus *bus,size_t ram_size,
    pr_r0899_cpu *cpu,const pr_u3b0_01da_limits *limits,
    pr_u3b0_selection_diagnostic *diagnostic);
#endif
