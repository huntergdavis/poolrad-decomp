#ifndef POOLRAD_RECOVERED_RESIDENT_0802_0175_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_0175_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_0802_0040_cpu.h"

#define PR_R0802_0175_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[0175,0193) (30 bytes, 16 instructions, RETF 8), the t30 cluster's block filler
 * (docs/re/RES_0802_0175_CPU.md). SIDE (ii), NON-CREDIT: both call sites (0193 at +01B9 and +01E4) push the constants 0, 18h, 0, 28h (column 0, row 24,
 * attribute 0, 40 blocks), so no game state enters this body whatever calls it; it has a model because 0193 composes it, and it has NO identity and NO credit.
 * Entered by PUSH CS; near CALL or a far call: [SP] IP, [SP+2] CS, then four byte-valued arguments, each in a word slot:
 * [SP+4] count (BP+6), [SP+6] attribute (BP+8), [SP+8] row (BP+0Ah), [SP+0Ah] column (BP+0Ch).
 *
 *   0175 PUSH BP; MOV BP,SP
 *   0178 0040(column [BP+0Ch], row [BP+0Ah], attribute [BP+8], count [BP+6], char DBh)        PUSH CS; CALL 0040, composed
 *   018F POP BP; RETF 8
 * One character writer call: the full block DBh, count times, from the column on one row.
 *
 * Composition: the character writer 0040 (resident_0802_0040_cpu, which composes Dos.Intr), the near-call displacement read live from CS:018D, the
 * callee required to return to 018Fh (else CALLEE, or DISPLAY when 0040 stopped otherwise). The INT 10h handler is the declared environment of 0040.
 * Exactly 1 MiB of A20-off RAM, IP = 0175h, entry SP in [0040h,FFF4h]. Each pushed argument carries a stale AH (the five PUSH AX are not preceded by a
 * clear), which the model reproduces. No flag is masked. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_0175_cpu;

typedef enum {
    PR_R0802_0175_RETURNED = 0,  /* RETF 8 executed */
    PR_R0802_0175_ARGUMENT,      /* null pointers, RAM size, IP not 0175h, entry SP out of range; nothing changed */
    PR_R0802_0175_DISPLAY,       /* the 0040 call did not return normally (diagnostic: its status) */
    PR_R0802_0175_CALLEE         /* 0040 returned somewhere other than 018Fh, or the call target is not 0040 */
} pr_r0802_0175_status;

typedef struct {
    int callee_status;                  /* 0040's own status (-1 when it was not called) */
    pr_r0802_0040_diagnostic display;   /* its diagnostic */
} pr_r0802_0175_diagnostic;

pr_r0802_0175_status pr_r0802_0175(uint8_t *ram, size_t ram_size, pr_r0802_0175_cpu *cpu, const pr_r0802_0040_env *env,
                                   pr_r0802_0175_diagnostic *diagnostic);
#endif
