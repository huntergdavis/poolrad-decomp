#ifndef POOLRAD_RECOVERED_RESIDENT_0464_VIDEO_MODE_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0464_VIDEO_MODE_CPU_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/recovered/dos_intr_000b_cpu.h"

#define PR_R0464_VIDEO_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0464_video_cpu;

typedef struct {
    pr_dos_intr_service video;   /* the INT 10h handler Intr transfers to (dos_intr_000b_cpu.h) */
    void *video_context;
} pr_r0464_video_env;

typedef enum {
    PR_R0464_VIDEO_OK = 0,
    PR_R0464_VIDEO_ARGUMENT,   /* null pointers, RAM not exactly 1 MiB, IP not the entry, SP outside the guard; nothing changed */
    PR_R0464_VIDEO_INTR,       /* Dos.Intr did not return (*intr_status holds its status; SERVICE = the INT 10h handler) */
    PR_R0464_VIDEO_TRANSFER    /* a callee returned somewhere other than the instruction after its call */
} pr_r0464_video_status;

/* Raw-CPU, full-state model of START.EXE resident 0464:[00CF,00FF) (48 bytes, 19 instructions), the video-mode set on
 * the RES 00BA:16EC -> RES_03F1_0000 -> OVL_040C_015D path (docs/re/RES_0464_VIDEO_MODE_00CF_CPU.md). Entered by a far
 * call or PUSH CS; near CALL with the mode word at [SP+4]; RETF 2.
 *   00CF PUSH BP; MOV BP,SP; SUB SP,2                 (SUB's flags, overwritten by Intr)
 *   00D5 [4A1C] := [4A1D]                             the previous mode is saved
 *   00DB [4A03] := 0; [4A02] := mode ([BP+6])          Regs.AX at DS:4A02 = AH 0 (set mode), AL the mode
 *   00E6 PUSH 10h; PUSH DS; PUSH 4A02h; LCALL 0A32:000B   composed: Dos.Intr(10h, Regs) (#1259)
 *   00F3 [4A1D] := mode ([BP+6], reread)               the current mode is recorded
 *   00F9 MOV SP,BP; POP BP; RETF 2
 * On return the flags are those Intr returns with, AX/DI as Intr leaves them (DI = 4A02h is pushed, not kept), and DS is
 * restored by Intr. The far-call segment word is read live from CS:00F1 (a loader relocation). Entry SP must lie in
 * [0022h,FFFAh]: the body pushes 14 bytes before Intr, whose own guard needs its entry SP in [0014h,FFF6h], and RETF 2
 * then reads [SP,SP+5] without wrapping. */
pr_r0464_video_status pr_r0464_set_mode_00cf_cpu(uint8_t *ram, size_t ram_size, pr_r0464_video_cpu *cpu,
                                                 const pr_r0464_video_env *env, int *intr_status);

/* Raw-CPU, full-state model of START.EXE resident 0464:[00FF,010C) (13 bytes, 8 instructions), the video-mode restore:
 * PUSH BP; MOV BP,SP; MOV AL,[4A1C]; PUSH AX; PUSH CS; CALL 00CF (composed above, which sets the saved mode and saves
 * the mode it replaces); POP BP; RETF. Entry SP must lie in [002Ah,FFFCh] (8 bytes pushed before 00CF's guard). */
pr_r0464_video_status pr_r0464_restore_mode_00ff_cpu(uint8_t *ram, size_t ram_size, pr_r0464_video_cpu *cpu,
                                                     const pr_r0464_video_env *env, int *intr_status);

#endif
