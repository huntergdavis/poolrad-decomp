#ifndef POOLRAD_RECOVERED_RESIDENT_VIDEO_INIT_4426_H
#define POOLRAD_RECOVERED_RESIDENT_VIDEO_INIT_4426_H

/* The startup video-mode selection of the DOS v1.3 resident program (START.EXE unpacked image 0x04426..0x044B5, 144 bytes) with the three
 * small resident routines it calls, plus the display-state bytes they write. This is what puts the game into the 320x200 16-colour EGA mode (BIOS
 * mode 0x0D) and fixes the two video segments, i.e. the geometry and memory plane the first gameplay view is drawn into. Evidence:
 * docs/re/RESIDENT_VIDEO_INIT_4426.md (static decode plus the hash-pinned native first-view RAM images).
 *
 *   SetVideoMode(mode)     0x0464:0x00CF (+0x00CF..+0x00FC, 46 bytes, `retf 2`): prev := cur; regs.AH := 0; regs.AL := mode; INT 10h through the
 *                          Pascal Intr unit routine 0x0A32:0x000B; cur := mode                                           (full translation, one hook)
 *   SetActivePage(page)    0x0709:0x0071 (+0x0071..+0x009D, 45 bytes, `retf 2`): regs.AH := 5; regs.AL := page; INT 10h; [0x4A16] := 0xA000 + (page << 9)
 *   SetDrawPage(page)      0x0709:0x00A0 (+0x00A0..+0x00B4, 21 bytes, `retf 2`): [0x4A18] := 0xA000 + (page << 9)       (no hook)
 *   selection              regs.AH := 0x0F; INT 10h; prev := cur := regs.AL; then on the adapter byte [0x484C]:
 *                            0x45: [0x4A1E] := 1; SetVideoMode(0x0D); [0x4A1B] := [0x4A1A] := 0; SetActivePage([0x4A1B]); SetDrawPage([0x4A1A])
 *                            0x54: [0x4A1E] := 2; [0x440A] -= 0x400 (via the scratch words 0x4848/0x484A); SetVideoMode(9)
 *                            else: [0x4A1E] := 0; SetVideoMode(4)
 *                          and falls into the unrelated code at +0x44B6, which is not modelled.
 * The register record is the Turbo Pascal Registers record (AX BX CX DX BP SI DI DS ES Flags) at DS:0x4A02; INT 10h itself (0xA32:0x000B ->
 * the BIOS) is the one explicit hook. Words are little endian, every word add wraps at 16 bits.
 *
 * Classification: bounded model of a slice with full translations of the three callees; no part of the BIOS is invented. Not a strict translation of
 * the enclosing startup routine. No game-function credit is claimed. */

#include <stdint.h>

#define PR_VI_REGS_BYTES 20u                 /* the Registers record DS:0x4A02..0x4A15 */
#define PR_VI_MODE_EGA 0x0Du
#define PR_VI_MODE_TANDY 0x09u
#define PR_VI_MODE_CGA 0x04u
#define PR_VI_ADAPTER_E 0x45u
#define PR_VI_ADAPTER_T 0x54u
#define PR_VI_SEG_BASE 0xA000u

typedef struct pr_vi_ds pr_vi_ds;

typedef struct {
    /* 0xA32:0x000B Intr(intno, regs): loads the registers from the record, calls the interrupt, writes them back. The hook may change the record. */
    int (*intr_a32_000b)(void *ctx, pr_vi_ds *ds, uint8_t intno, uint8_t regs[PR_VI_REGS_BYTES]);
} pr_vi_hooks;

struct pr_vi_ds {
    uint8_t regs[PR_VI_REGS_BYTES];          /* DS:0x4A02 .. 0x4A15 */
    uint16_t w4a16;                          /* active (displayed) page segment */
    uint16_t w4a18;                          /* draw page segment */
    uint8_t b4a1a, b4a1b;                    /* draw page, active page */
    uint8_t b4a1c, b4a1d;                    /* previous and current BIOS video mode */
    uint8_t b4a1e;                           /* adapter class: 1 = EGA, 2 = Tandy, 0 = other */
    uint8_t b484c;                           /* adapter letter (read only here) */
    uint16_t w4408, w440a;                   /* the Tandy path rewrites w440a */
    uint16_t w4848, w484a;                   /* scratch words of the Tandy path */
};

typedef enum {
    PR_VI_OK = 0,
    PR_VI_ARGUMENT,       /* null argument */
    PR_VI_HOOK,           /* the Intr hook is NULL when reached */
    PR_VI_CALLBACK        /* the hook returned nonzero (report->callback_status) */
} pr_vi_status;

typedef struct {
    unsigned intr_calls;
    uint8_t intr_ah[4];                      /* AH of each INT 10h call, in order */
    uint8_t intr_al[4];
    int callback_status;
} pr_vi_report;

/* The three callees as separate entry points (each pushes one word; only the low byte is read). */
pr_vi_status pr_vi_set_video_mode(pr_vi_ds *ds, void *ctx, const pr_vi_hooks *h, uint8_t mode, pr_vi_report *r);
pr_vi_status pr_vi_set_active_page(pr_vi_ds *ds, void *ctx, const pr_vi_hooks *h, uint8_t page, pr_vi_report *r);
void pr_vi_set_draw_page(pr_vi_ds *ds, uint8_t page);

/* The selection slice +0x4426..+0x44B5. `r` may be NULL. On a non-OK status the effects made before the stop stand. */
pr_vi_status pr_vi_select_mode(pr_vi_ds *ds, void *ctx, const pr_vi_hooks *h, pr_vi_report *r);

#endif
