#ifndef POOLRAD_RECOVERED_RESIDENT_0802_0000_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_0000_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_R0802_0000_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[0000,0020) (32 bytes, 12 instructions, RETF 6), the t30 cluster's
 * FreeMem wrapper, a proposed resident_runtime (never-credit) member (docs/re/RES_0802_0000_CPU.md). Entered by PUSH CS; CALL
 * (0716 at +0941) or a far call: [SP] IP, [SP+2] CS, [SP+4] the size word, [SP+6] the far address of the pointer variable.
 *
 *   0000 PUSH BP; MOV BP,SP
 *   0003 LES DI,[BP+8]; PUSH ES:[DI+2]; PUSH ES:[DI]      (the pointer variable's VALUE; the variable is not cleared)
 *   000D AX := ([BP+6] + 7) AND FFF8h                     (16-bit: a size of FFF9h..FFFFh wraps to 0)
 *   0016 PUSH AX; LCALL 0AF8:0254 (FreeMem)
 *   001C POP BP; RETF 6
 *
 * Composition: FreeMem 0AF8:0254 (system_freemem_0254_cpu, a runtime-covered endpoint), its segment word read live from the
 * code, required to return to 001Ch (else CALLEE). FreeMem's stops pass through: RUNERROR (an invalid or duplicate pointer:
 * AX = 204 at 0AF8:00E2, non-returning) and LIMIT (the declared max_steps cap on its free-list walk). A rounded size of 0 is
 * FreeMem's no-op, with no pointer check. No DOS service. Exactly 1 MiB of A20-off RAM with the relocated code at CS; IP
 * 0000h; entry SP in [002Ch,FFF6h] (FreeMem's own entry SP, 12 bytes lower, must lie in its [0020h,FFF6h]). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_0000_cpu;

typedef enum {
    PR_R0802_0000_RETURNED = 0,  /* RETF 6 executed */
    PR_R0802_0000_ARGUMENT,      /* null pointers, RAM size, IP not 0000h, entry SP out of range; nothing changed */
    PR_R0802_0000_RUNERROR,      /* FreeMem refused the pointer: AX = 204 at 0AF8:00E2 (RunError's entry); non-returning */
    PR_R0802_0000_LIMIT,         /* FreeMem's declared max_steps cap on the free-list walk */
    PR_R0802_0000_CALLEE         /* FreeMem stopped otherwise or returned elsewhere (see the diagnostic) */
} pr_r0802_0000_status;

typedef struct {
    int callee_status;           /* FreeMem's status for a stop other than RETURNED, else -1 */
} pr_r0802_0000_diagnostic;

pr_r0802_0000_status pr_r0802_0000(uint8_t *ram, size_t ram_size, pr_r0802_0000_cpu *cpu, unsigned max_steps,
                                   pr_r0802_0000_diagnostic *diagnostic);
#endif
