#ifndef POOLRAD_RECOVERED_RESIDENT_0802_06E7_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_06E7_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_close_1491_cpu.h"

#define PR_R0802_06E7_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[06E7,070C) (37 bytes, 14 instructions, RETF 8), the t30 cluster's
 * "close both archive files" leaf (docs/re/RES_0802_06E7_CPU.md). Entered by PUSH CS; CALL (from 0716 at +08AD/+091D) or
 * any far call: [SP] IP, [SP+2] CS, [SP+4] the far address of file A, [SP+8] the far address of file B.
 *
 *   06E7 PUSH BP; MOV BP,SP
 *   06EA LES DI,[BP+0Ah]; PUSH ES; PUSH DI; LCALL 0AF8:1491 (Close B); LCALL 0AF8:04A9 (IOCheck)
 *   06F9 LES DI,[BP+6];   PUSH ES; PUSH DI; LCALL 0AF8:1491 (Close A); LCALL 0AF8:04A9 (IOCheck)
 *   0708 POP BP; RETF 8
 *
 * Composition: Close (system_close_1491_cpu, #1309) and IOCheck (system_iocheck_04a9_cpu, #1306), each far call's segment
 * word read live from the code (relocations); each must return to the instruction after its call, or the stop is CALLEE.
 * Declared environment: Close's INT 21h AH=3Eh service, under Close's own contract. Terminal stop RUNERROR: IOCheck's jump to
 * 0AF8:00E2 when InOutRes is not zero (a close error, or 103 for a file that is not open). Exactly 1 MiB of A20-off RAM with the
 * relocated code at CS (the far-call operands are read from it); IP = 06E7h; entry SP in [000Ch,FFF4h] (pushes reach entry
 * SP - 0Ch inside Close, the arguments entry SP + 0Bh). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_06e7_cpu;

typedef enum {
    PR_R0802_06E7_RETURNED = 0,  /* RETF 8 executed */
    PR_R0802_06E7_ARGUMENT,      /* null pointers, RAM size, IP not 06E7h, entry SP out of range; nothing changed */
    PR_R0802_06E7_RUNERROR,      /* TERMINAL: IOCheck at 0AF8:00E2 (RunError), AX the error */
    PR_R0802_06E7_SERVICE,       /* Close's INT 21h service declined or broke its continuation; CPU at the INT */
    PR_R0802_06E7_CALLEE         /* a composed model stopped otherwise or returned elsewhere (see the diagnostic) */
} pr_r0802_06e7_status;

typedef struct {
    uint16_t site;      /* IP of the far call this stop concerns (0 when none) */
    int callee_status;  /* the composed model's own status (-1 when none) */
} pr_r0802_06e7_diagnostic;

pr_r0802_06e7_status pr_r0802_06e7(uint8_t *ram, size_t ram_size, pr_r0802_06e7_cpu *cpu, pr_system_close_service service,
                                   void *context, pr_r0802_06e7_diagnostic *diagnostic);
#endif
