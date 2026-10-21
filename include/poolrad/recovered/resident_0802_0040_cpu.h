#ifndef POOLRAD_RECOVERED_RESIDENT_0802_0040_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_0040_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/dos_intr_000b_cpu.h"

#define PR_R0802_0040_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[0040,0105) (197 bytes, 77 instructions, RETF 0Ah), the t30 cluster's
 * character writer (docs/re/RES_0802_0040_CPU.md). Entered by PUSH CS; near CALL (0105 at +015F, 0175 at +018C) or a far call: [SP] IP,
 * [SP+2] CS, then five byte-valued arguments, each in a word slot: [SP+4] char (BP+6 after PUSH BP), [SP+6] count, [SP+8] attribute
 * index, [SP+0Ah] row (DH), [SP+0Ch] column (DL).
 *
 *   0040 PUSH BP; MOV BP,SP
 *   0043 Regs.AH := 2 (set cursor); Regs.BH := [4A1B] (page); Regs.DH := row; Regs.DL := column   (Regs is DS:4A02)
 *   005A PUSH AX (AL = 10h, AH as it stands); PUSH DS; PUSH 4A02h; LCALL 0A32:000B                 Intr(10h, Regs): composed
 *   0067 Regs.AH := 9; Regs.AL := char; Regs.BH := [4A1B]
 *   0078 [4A1E] = 0: Regs.BL := [25D5 + attr]   else Regs.BL := attr          (the EGA flag selects the attribute translation)
 *   0095 Regs.CX := count (zero-extended); PUSH 0010h; PUSH DS; PUSH 4A02h; LCALL 0A32:000B           Intr(10h, Regs): composed
 *   00AA [4A1E] = 1 ONLY (not 2, not any other non-zero): the same pair again on page [4A1A] with the attribute untranslated
 *   0101 POP BP; RETF 0Ah
 * The DS:4A1E values mean: 0 = translate the attribute through DS:25D5, one write on page [4A1B]; 1 = no translation, two writes (pages
 * [4A1B] then [4A1A]); any other value (2 included) = no translation, ONE write: it is NOT the same as 1.
 *
 * Composition: Dos.Intr 0A32:000B (dos_intr_000b_cpu) four times, each far-call segment word read live from CS:0065, CS:00A8, CS:00D3 and
 * CS:00FF (loader relocations), each required to return to the instruction after its call (else TRANSFER). The INT 10h handler that Intr
 * transfers to is the declared environment (dos_intr_000b_cpu.h): it may write RAM, and every operand is reread afterwards in source order.
 * Exactly 1 MiB of A20-off RAM; IP = 0040h; entry SP in [0022h,FFF0h] (the frame, the three pushes and the far call lie below it; the
 * arguments reach entry SP + 0Dh; RETF 0Ah ends at entry SP + 0Eh). No flag is masked. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_0040_cpu;

typedef struct {
    pr_dos_intr_service video;   /* the INT 10h handler Intr transfers to */
    void *video_context;
} pr_r0802_0040_env;

typedef enum {
    PR_R0802_0040_RETURNED = 0,  /* RETF 0Ah executed */
    PR_R0802_0040_ARGUMENT,      /* null pointers, RAM size, IP not 0040h, entry SP out of range; nothing changed */
    PR_R0802_0040_INTR,          /* Dos.Intr did not return (diagnostic: its status; SERVICE = the INT 10h handler) */
    PR_R0802_0040_TRANSFER       /* Intr returned somewhere other than the instruction after its call */
} pr_r0802_0040_status;

typedef struct {
    int intr_status;     /* pr_dos_intr_status of the last Intr call, -1 when none was made */
    unsigned calls;      /* Intr calls completed (0..4) */
} pr_r0802_0040_diagnostic;

pr_r0802_0040_status pr_r0802_0040(uint8_t *ram, size_t ram_size, pr_r0802_0040_cpu *cpu, const pr_r0802_0040_env *env,
                                   pr_r0802_0040_diagnostic *diagnostic);
#endif
