#ifndef POOLRAD_RECOVERED_RESIDENT_0802_0020_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_0020_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_R0802_0020_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[0020,0040) (32 bytes, 12 instructions, RETF 6), the t30 cluster's
 * GetMem wrapper, a proposed resident_runtime (never-credit) member (docs/re/RES_0802_0020_CPU.md). Entered by PUSH CS; CALL
 * (0716 at +08BC) or a far call: [SP] IP, [SP+2] CS, [SP+4] the size word, [SP+6] the far address of the pointer variable.
 *
 *   0020 PUSH BP; MOV BP,SP
 *   0023 AX := ([BP+6] + 7) AND FFF8h        (16-bit: a size of FFF9h..FFFFh wraps to 0)
 *   002C PUSH AX; LCALL 0AF8:023F (GetMem)     -> DX:AX
 *   0032 LES DI,[BP+8]; ES:[DI] := AX; ES:[DI+2] := DX
 *   003C POP BP; RETF 6
 *
 * Composition: GetMem 0AF8:023F (system_getmem_023f_cpu, a runtime-covered endpoint), its segment word read live from the
 * code, required to return to 0032h (else CALLEE). GetMem's stops pass through: RUNERROR (allocation failed: AX = 203 at
 * 0AF8:00E2, non-returning), HEAP_ERROR (a non-default HeapError, stopped at its indirect call) and LIMIT (the declared
 * max_steps cap on the free-list walk). No DOS service. Exactly 1 MiB of A20-off RAM with the relocated code at CS (GetMem
 * runs at its original offsets); IP 0020h; entry SP in [0020h,FFF6h]. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_0020_cpu;

typedef enum {
    PR_R0802_0020_RETURNED = 0,  /* RETF 6 executed: the pointer variable holds DX:AX (nil for a rounded size of 0) */
    PR_R0802_0020_ARGUMENT,      /* null pointers, RAM size, IP not 0020h, entry SP out of range; nothing changed */
    PR_R0802_0020_RUNERROR,      /* GetMem failed: AX = 203 at 0AF8:00E2 (RunError's entry); non-returning */
    PR_R0802_0020_HEAP_ERROR,    /* GetMem stopped at a non-default HeapError's indirect call */
    PR_R0802_0020_LIMIT,         /* GetMem's declared max_steps cap on the free-list walk */
    PR_R0802_0020_CALLEE         /* GetMem stopped otherwise or returned elsewhere (see the diagnostic) */
} pr_r0802_0020_status;

typedef struct {
    int callee_status;           /* GetMem's status for a stop other than RETURNED, else -1 */
} pr_r0802_0020_diagnostic;

pr_r0802_0020_status pr_r0802_0020(uint8_t *ram, size_t ram_size, pr_r0802_0020_cpu *cpu, unsigned max_steps,
                                   pr_r0802_0020_diagnostic *diagnostic);
#endif
