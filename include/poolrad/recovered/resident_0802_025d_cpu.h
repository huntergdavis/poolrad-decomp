#ifndef POOLRAD_RECOVERED_RESIDENT_0802_025D_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_025D_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_0802_02eb_cpu.h"

#define PR_R0802_025D_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[025D,02EB) (142 bytes, 63 instructions, RETF 0Eh), the t30 cluster's
 * open-with-disk-prompt leaf (docs/re/RES_0802_025D_CPU.md). Entered by PUSH CS; CALL (05CD at +0659) or a far call: [SP] IP,
 * [SP+2] CS, [SP+4] the far address of file f, [SP+8] the no-prompt flag (a word slot, read as a byte), [SP+0Ah] the far prompt
 * message, [SP+0Eh] the far file name.
 *
 *   025D PUSH BP; MOV BP,SP; SUB SP,54h
 *   0263 name -> [BP-2Ah], message -> [BP-53h] (StrStore, max 40 each)
 *   0289 exists := CALL 02EB (FileExists, composed); [BP-54h] := AL
 *   0295 not exists and flag = 0: CALL 0193 (message, attribute 0Eh)                     -> stop PROMPT (resume 02ADh)
 *   02AD not exists and flag = 0: JMP 0289 (retry: the "insert disk" loop)
 *   02B9 exists: Assign(f, name); Reset(f, 1)     (no IOCheck: {$I-})
 *   02DC AL := exists; MOV SP,BP; POP BP; RETF 0Eh
 *
 * Composition: FileExists 02EB (resident_0802_02eb_cpu), StrStore 0B25, Assign 13E2, Reset 1410 (with its nested Close
 * composed), each far call's segment word read live from the code, each callee required to return after its call (else CALLEE).
 * The prompt 0193 is not yet modelled: a NAMED STOP with PUSH CS; CALL 0193 executed, resumable at 02ADh with the post-callee
 * state (the two-phase seam). Declared environment: INT 21h AH=3Dh (Reset's contract) and AH=3Eh (Close's), as for 02EB.
 * Exactly 1 MiB of A20-off RAM with the relocated code at CS; IP 025Dh or 02ADh; entry SP in [0200h,FFEEh] at 025Dh (the
 * arguments reach entry SP + 11h). */
typedef pr_r0802_02eb_cpu pr_r0802_025d_cpu;

typedef enum {
    PR_R0802_025D_RETURNED = 0,  /* RETF 0Eh executed: AL = 1 (opened) or 0 */
    PR_R0802_025D_ARGUMENT,      /* null pointers, RAM size, IP not an entry, entry SP out of range; nothing changed */
    PR_R0802_025D_PROMPT,        /* CALL 0193 executed (resume at 02ADh) */
    PR_R0802_025D_SERVICE,       /* a declared DOS service declined or broke its continuation; CPU at that INT */
    PR_R0802_025D_CALLEE         /* a composed model stopped otherwise or returned elsewhere (see the diagnostic) */
} pr_r0802_025d_status;

typedef pr_r0802_02eb_diagnostic pr_r0802_025d_diagnostic;

pr_r0802_025d_status pr_r0802_025d(uint8_t *ram, size_t ram_size, pr_r0802_025d_cpu *cpu, const pr_r0802_02eb_env *env,
                                   pr_r0802_025d_diagnostic *diagnostic);
#endif
