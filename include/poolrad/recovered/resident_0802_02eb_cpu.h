#ifndef POOLRAD_RECOVERED_RESIDENT_0802_02EB_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_02EB_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_close_1491_cpu.h"
#include "poolrad/recovered/system_reset_1410_cpu.h"

#define PR_R0802_02EB_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[02EB,0356) (107 bytes, 43 instructions, RETF 4), the t30 cluster's
 * FileExists leaf (docs/re/RES_0802_02EB_CPU.md). Entered by PUSH CS; CALL (025D at +028E) or a far call: [SP] IP, [SP+2] CS,
 * [SP+4] the far address of the Pascal file name.
 *
 *   02EB PUSH BP; MOV BP,SP; SUB SP,0AAh
 *   02F2 name -> [BP-2Ah] (StrStore, max 40)
 *   0305 Assign(f = [BP-0AAh], name)              LCALL 0AF8:13E2
 *   0315 Reset(f, 128)                            LCALL 0AF8:1410 (and its nested Close 1491 when f is open)
 *   0324 Close(f)                                 LCALL 0AF8:1491
 *   032F DS:[49CE] := IOResult                    LCALL 0AF8:04A2 (reads and clears InOutRes; no IOCheck: {$I-})
 *   0337 AL := 1 when [49CE] = 0 and the copied name is not empty, else 0; [BP-1] := AL
 *   0350 MOV SP,BP; POP BP; RETF 4
 *
 * Composition: StrStore 0B25, Assign 13E2, Reset 1410, Close 1491 and IOResult 04A2, each from its model on main, each far
 * call's segment word read live from the code, each required to return to the instruction after its call (else CALLEE).
 * Reset's nested-Close stop is composed (Close, then Reset resumed at 1445h); it is unreachable here because Assign leaves the
 * magic D7B0h. Declared environment: Reset's INT 21h AH=3Dh and Close's AH=3Eh services, each under its own model's
 * contract. Exactly 1 MiB of A20-off RAM with the relocated code at CS; IP = 02EBh; entry SP in [0100h,FFF8h] (the frame is
 * AAh bytes, the composed frames lie below it; the argument reaches entry SP + 7). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_02eb_cpu;

typedef struct {
    pr_reset_dos_service open;   void *open_context;    /* INT 21h AH=3Dh, Reset's contract */
    pr_system_close_service close; void *close_context; /* INT 21h AH=3Eh, Close's contract */
} pr_r0802_02eb_env;

typedef enum {
    PR_R0802_02EB_RETURNED = 0,  /* RETF 4 executed: AL = 1 (exists) or 0 */
    PR_R0802_02EB_ARGUMENT,      /* null pointers, RAM size, IP not 02EBh, entry SP out of range; nothing changed */
    PR_R0802_02EB_SERVICE,       /* a declared DOS service declined or broke its continuation; CPU at that INT */
    PR_R0802_02EB_CALLEE         /* a composed model stopped otherwise or returned elsewhere (see the diagnostic) */
} pr_r0802_02eb_status;

typedef struct {
    uint16_t site;      /* IP of the far call this stop concerns (0 when none) */
    int callee_status;  /* the composed model's own status (-1 when none) */
} pr_r0802_02eb_diagnostic;

pr_r0802_02eb_status pr_r0802_02eb(uint8_t *ram, size_t ram_size, pr_r0802_02eb_cpu *cpu, const pr_r0802_02eb_env *env,
                                   pr_r0802_02eb_diagnostic *diagnostic);
#endif
