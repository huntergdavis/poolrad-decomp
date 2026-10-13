/* Resident startup video-mode selection; see docs/re/RESIDENT_VIDEO_INIT_4426.md. */
#include "poolrad/recovered/resident_video_init_4426.h"

#include <string.h>

#define AX_L 0u    /* regs[0] = AL, regs[1] = AH (DS:0x4A02, 0x4A03); BL/BH follow at 2 and 3 */
#define AX_H 1u

static pr_vi_status intr10(pr_vi_ds *ds, void *ctx, const pr_vi_hooks *h, pr_vi_report *r)
{
    int rc;
    if (!h || !h->intr_a32_000b) return PR_VI_HOOK;
    if (r && r->intr_calls < 4u) { r->intr_ah[r->intr_calls] = ds->regs[AX_H]; r->intr_al[r->intr_calls] = ds->regs[AX_L]; }
    if (r) r->intr_calls++;
    rc = h->intr_a32_000b(ctx, ds, 0x10u, ds->regs);                              /* push 0x10 ; push ds:0x4A02 ; lcall 0xA32:0xB */
    if (rc != 0) { if (r) r->callback_status = rc; return PR_VI_CALLBACK; }
    return PR_VI_OK;
}

pr_vi_status pr_vi_set_video_mode(pr_vi_ds *ds, void *ctx, const pr_vi_hooks *h, uint8_t mode, pr_vi_report *r)
{
    pr_vi_status st;
    if (!ds) return PR_VI_ARGUMENT;
    ds->b4a1c = ds->b4a1d;                                                        /* +0x00D5 */
    ds->regs[AX_H] = 0u;                                                          /* +0x00DB */
    ds->regs[AX_L] = mode;                                                        /* +0x00E0 */
    st = intr10(ds, ctx, h, r);                                                   /* +0x00E6..+0x00EE */
    if (st != PR_VI_OK) return st;
    ds->b4a1d = mode;                                                             /* +0x00F3 */
    return PR_VI_OK;
}

pr_vi_status pr_vi_set_active_page(pr_vi_ds *ds, void *ctx, const pr_vi_hooks *h, uint8_t page, pr_vi_report *r)
{
    pr_vi_status st;
    if (!ds) return PR_VI_ARGUMENT;
    ds->regs[AX_H] = 5u;                                                          /* +0x0074 */
    ds->regs[AX_L] = page;                                                        /* +0x0079 */
    st = intr10(ds, ctx, h, r);                                                   /* +0x007F..+0x0087 */
    if (st != PR_VI_OK) return st;
    ds->w4a16 = (uint16_t)((uint16_t)((uint16_t)page << 9) + PR_VI_SEG_BASE);     /* +0x008C..+0x0099: shl 9 and add 0xA000, both 16 bit */
    return PR_VI_OK;
}

void pr_vi_set_draw_page(pr_vi_ds *ds, uint8_t page)
{
    if (!ds) return;
    ds->w4a18 = (uint16_t)((uint16_t)((uint16_t)page << 9) + PR_VI_SEG_BASE);     /* +0x00A3..+0x00B0 */
}

pr_vi_status pr_vi_select_mode(pr_vi_ds *ds, void *ctx, const pr_vi_hooks *h, pr_vi_report *r)
{
    pr_vi_status st;
    if (r) memset(r, 0, sizeof *r);
    if (!ds) return PR_VI_ARGUMENT;
    ds->regs[AX_H] = 0x0Fu;                                                       /* +0x4426: get the current video mode */
    st = intr10(ds, ctx, h, r);                                                   /* +0x442B..+0x4433 */
    if (st != PR_VI_OK) return st;
    ds->b4a1c = ds->regs[AX_L];                                                   /* +0x4438 */
    ds->b4a1d = ds->b4a1c;                                                        /* +0x443E */
    if (ds->b484c == PR_VI_ADAPTER_E) {                                           /* +0x4444..+0x4449 */
        ds->b4a1e = 1u;                                                           /* +0x444B */
        st = pr_vi_set_video_mode(ds, ctx, h, PR_VI_MODE_EGA, r);                 /* +0x4450..+0x4453 */
        if (st != PR_VI_OK) return st;
        ds->b4a1b = 0u; ds->b4a1a = 0u;                                           /* +0x4458, +0x445D */
        st = pr_vi_set_active_page(ds, ctx, h, ds->b4a1b, r);                     /* +0x4462..+0x4466: 0x709:0x71 */
        if (st != PR_VI_OK) return st;
        pr_vi_set_draw_page(ds, ds->b4a1a);                                       /* +0x446B..+0x446F: 0x709:0xA0 */
        return PR_VI_OK;
    }
    if (ds->b484c == PR_VI_ADAPTER_T) {                                           /* +0x4476..+0x4478 */
        uint16_t hi = (uint16_t)(ds->w440a - 0x400u);                             /* +0x447F..+0x4485 */
        ds->b4a1e = 2u;                                                           /* +0x447A */
        ds->w4848 = ds->w4408; ds->w484a = hi;                                    /* +0x4487..+0x448D */
        ds->w4408 = ds->w4848; ds->w440a = ds->w484a;                             /* +0x4491..+0x449B */
        return pr_vi_set_video_mode(ds, ctx, h, PR_VI_MODE_TANDY, r);             /* +0x449F..+0x44A2 */
    }
    ds->b4a1e = 0u;                                                               /* +0x44A9 */
    return pr_vi_set_video_mode(ds, ctx, h, PR_VI_MODE_CGA, r);                   /* +0x44AE..+0x44B1 */
}
