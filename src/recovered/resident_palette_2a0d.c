/* Resident palette-register setter 0x464:0x2A0D; see docs/re/RESIDENT_PALETTE_2A0D_GATE.md. */
#include "poolrad/recovered/resident_palette_2a0d.h"

#include <string.h>

pr_vi_status pr_res_set_palette(pr_vi_ds *ds, void *ctx, const pr_vi_hooks *h, uint8_t reg, uint8_t value, pr_vi_report *r)
{
    int rc;
    if (r) memset(r, 0, sizeof *r);
    if (!ds) return PR_VI_ARGUMENT;
    ds->regs[1] = 0x10u;                                                   /* +0x2A10: [0x4A03] := 0x10 (AH) */
    ds->regs[0] = 0u;                                                      /* +0x2A15: [0x4A02] := 0 (AL) */
    ds->regs[3] = value < 8u ? value : (uint8_t)(value + 8u);              /* +0x2A1A..+0x2A30: [0x4A05] (BH) */
    ds->regs[2] = reg;                                                     /* +0x2A33: [0x4A04] (BL) := [bp+8] */
    if (!h || !h->intr_a32_000b) return PR_VI_HOOK;
    if (r) { r->intr_ah[0] = ds->regs[1]; r->intr_al[0] = ds->regs[0]; r->intr_calls = 1u; }
    rc = h->intr_a32_000b(ctx, ds, 0x10u, ds->regs);                       /* +0x2A39..+0x2A41: push 0x10, push ds:0x4A02, lcall 0xA32:0xB */
    if (rc != 0) { if (r) r->callback_status = rc; return PR_VI_CALLBACK; }
    return PR_VI_OK;
}
