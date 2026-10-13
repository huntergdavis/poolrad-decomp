#ifndef POOLRAD_RECOVERED_RESIDENT_PALETTE_2A0D_H
#define POOLRAD_RECOVERED_RESIDENT_PALETTE_2A0D_H

/* The resident EGA palette-register setter 0x0464:0x2A0D (START.EXE unpacked image 0x0704D..0x07089, 61 bytes through `retf 4` = CA 04 00,
 * SHA-256 9a5d907528d68e38983cc795b891dfbc5bb20aa0f8053494f47e553edbfff0a8). Two words are pushed; only their low bytes are read:
 *   [bp+8] (pushed first) = the palette register, [bp+6] = the colour value.
 *   [0x4A03] := 0x10 (AH), [0x4A02] := 0 (AL); value < 8 -> BH := value, else BH := (value + 8) & 0xFF; [0x4A05] := BH; [0x4A04] := the register (BL);
 *   Intr(0x10, DS:0x4A02) through 0xA32:0x000B (BIOS INT 10h, AX = 0x1000: set one palette register, BL = register, BH = value).
 * The BIOS call is the same explicit hook as the merged resident_video_init_4426.h (pr_vi_hooks, pr_vi_ds); no other instruction exists in the routine.
 * Evidence and the static first-view call gate: docs/re/RESIDENT_PALETTE_2A0D_GATE.md.
 *
 * Classification: full translation of the routine with one BIOS hook. No game-function credit is claimed (the routine is library-like support). */

#include "poolrad/recovered/resident_video_init_4426.h"

/* `reg` = [bp+8], `value` = [bp+6]. `r` may be NULL. On a non-OK status the record bytes already written stand. */
pr_vi_status pr_res_set_palette(pr_vi_ds *ds, void *ctx, const pr_vi_hooks *h, uint8_t reg, uint8_t value, pr_vi_report *r);

#endif
