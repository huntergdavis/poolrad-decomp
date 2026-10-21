#ifndef POOLRAD_RECOVERED_RESIDENT_0802_05CD_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_05CD_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_0802_025d_cpu.h"
#include "poolrad/recovered/system_blockread_14fb_cpu.h"
#include "poolrad/recovered/system_seek_1563_cpu.h"

#define PR_R0802_05CD_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[05CD,06E7) (282 bytes, 118 instructions, RETF 14h), the t30 cluster's
 * archive-open leaf (docs/re/RES_0802_05CD_CPU.md). Entered by PUSH CS; CALL (0716 at +0775) or a far call: [SP] IP, [SP+2] CS,
 * [SP+4] the far address of file record g, [SP+8] the far address of file record f, [SP+0Ch] the far address of the
 * header word, [SP+10h] the far extension string, [SP+14h] the far base-name string.
 *
 *   05CD PUSH BP; MOV BP,SP; SUB SP,456h
 *   05D4 base -> [BP-2Ah], extension -> [BP-53h] (StrStore, max 40 each)
 *   05FA name := base + extension at [BP-155h] (string load 0B0B, concat 0B98; the pointer stays pushed)
 *   0614 message := table[DS:5376] (DS:2614h + 1Eh x the byte) + char(DS:547D) + CS:05CB at [BP-355h] (0B0B, 0C3B, 0B98 x2)
 *   064F opened := CALL 025D(name, message, flag DS:557C pushed as AX, f)              (composed; its prompt -> stop PROMPT)
 *   065C not opened: AL := 0
 *   0660 opened: Assign(g, base + extension); Reset(g, 1); IOCheck; BlockRead(g, header word, 2, [BP-55h]); IOCheck;
 *        header word += 2; Seek(f, header word zero-extended); IOCheck; AL := 1
 *        (f and g are opened on the same base + extension name: f by 025D, g by Assign + Reset)
 *   06DE MOV SP,BP; POP BP; RETF 14h
 *
 * Composition: the leaf 025D (resident_0802_025d_cpu, itself composing 02EB) and the SYSTEM models StrStore 0B25, string load
 * 0B0B, concat 0B98, char-to-string 0C3B, Assign 13E2, Reset 1410 (nested Close composed), IOCheck 04A9, BlockRead 14FB and
 * Seek 1563, each far call's segment word read live from the code, each callee required to return after its call (else
 * CALLEE). The unit mixes I/O modes: 025D below it is {$I-}, while each of this body's own Reset, BlockRead and Seek is
 * followed by IOCheck, which is TERMINAL (RUNERROR) on a non-zero InOutRes. Declared environment: INT 21h AH=3Dh/3Eh (through
 * 025D's environment and Reset's nested Close), AH=3Fh (BlockRead's contract) and AH=42h (Seek's).
 *
 * The prompt 0193 inside 025D is not yet modelled: PROMPT is a NAMED STOP with 025D's PUSH CS; CALL 0193 executed. The model
 * resumes at IP 02ADh (inside 025D) from the post-callee state, runs 025D on, and requires it to return to 065Ch in this body.
 * Exactly 1 MiB of A20-off RAM with the relocated code at CS; IP 05CDh or 02ADh; entry SP in [066Ah,FFE8h] at 05CDh (066Ah
 * keeps 025D's entry SP at or above its own 0200h bound; FFE8h keeps the RETF 14h frame inside the segment). */
typedef pr_r0802_025d_cpu pr_r0802_05cd_cpu;

typedef struct {
    pr_r0802_02eb_env file;                                   /* INT 21h 3Dh/3Eh, as for 025D and 02EB */
    pr_blockread_dos_service read; void *read_context;        /* INT 21h 3Fh */
    pr_seek_dos_service seek;      void *seek_context;        /* INT 21h 42h */
} pr_r0802_05cd_env;

typedef enum {
    PR_R0802_05CD_RETURNED = 0,  /* RETF 14h executed: AL = 1 (opened, header read, archive positioned) or 0 */
    PR_R0802_05CD_ARGUMENT,      /* null pointers, RAM size, IP not an entry, entry SP out of range; nothing changed */
    PR_R0802_05CD_PROMPT,        /* inside 025D: CALL 0193 executed (resume at 02ADh) */
    PR_R0802_05CD_RUNERROR,      /* IOCheck found InOutRes != 0: its JMP to 0AF8:00E2 executed; TERMINAL */
    PR_R0802_05CD_DIVIDE,        /* BlockRead's DIV by recsize 0 (unreachable: Reset sets recsize 1); CPU at the DIV */
    PR_R0802_05CD_SERVICE,       /* a declared DOS service declined or broke its continuation; CPU at that INT */
    PR_R0802_05CD_CALLEE         /* a composed model stopped otherwise or returned elsewhere (see the diagnostic) */
} pr_r0802_05cd_status;

typedef pr_r0802_02eb_diagnostic pr_r0802_05cd_diagnostic;

pr_r0802_05cd_status pr_r0802_05cd(uint8_t *ram, size_t ram_size, pr_r0802_05cd_cpu *cpu, const pr_r0802_05cd_env *env,
                                   pr_r0802_05cd_diagnostic *diagnostic);
#endif
