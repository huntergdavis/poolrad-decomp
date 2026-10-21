#ifndef POOLRAD_RECOVERED_RESIDENT_0802_0105_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_0105_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_0802_0040_cpu.h"

#define PR_R0802_0105_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[0105,0175) (112 bytes, 48 instructions, RETF 0Ah), the t30 cluster's status-line
 * string writer (docs/re/RES_0802_0105_CPU.md). Entered by PUSH CS; near CALL (0193 at +01CC) or a far call: [SP] IP, [SP+2] CS,
 * [SP+4] the far address of the Pascal message, [SP+8] the attribute index (a byte in a word slot), [SP+0Ah] the row, [SP+0Ch] the first column.
 *
 *   0105 PUSH BP; MOV BP,SP; SUB SP,104h
 *   010C message -> [BP-100h] (StrStore, max 255)                                 LCALL 0AF8:0B25
 *   0120 n := the copy's length byte; if n = 0 return; i := 1
 *   0142 repeat  0040(column [BP+0Eh], row [BP+0Ch], attribute [BP+0Ah], count 1, char s[i]);  column := column + 1 (a byte, in place);
 *        until i = n  (i is a byte, incremented before the test)                 PUSH CS; CALL 0040, composed
 *   016F MOV SP,BP; POP BP; RETF 0Ah
 * One 0040 call per character, so one character cell per call and the column byte wraps at 256.
 *
 * Composition: StrStore 0B25 (system_strstore_fullstate_0b25) and the character writer 0040 (resident_0802_0040_cpu, which composes Dos.Intr),
 * the far-call segment word read live from CS:011E and the near-call displacement read live from CS:0160. Each callee must return to the
 * instruction after its call (else CALLEE or DISPLAY). The INT 10h handler is the declared environment of 0040 (pr_r0802_0040_env). Exactly
 * 1 MiB of A20-off RAM, IP = 0105h, entry SP in [0140h,FFF0h] (the 104h-byte frame and the composed frames lie below it). No flag is masked. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_0105_cpu;

typedef enum {
    PR_R0802_0105_RETURNED = 0,  /* RETF 0Ah executed */
    PR_R0802_0105_ARGUMENT,      /* null pointers, RAM size, IP not 0105h, entry SP out of range; nothing changed */
    PR_R0802_0105_DISPLAY,       /* a 0040 call did not return normally (diagnostic: its status and the character index) */
    PR_R0802_0105_CALLEE         /* StrStore stopped or a callee returned elsewhere */
} pr_r0802_0105_status;

typedef struct {
    unsigned chars;                     /* 0040 calls completed */
    int callee_status;                  /* the last composed model's own status (-1 when none) */
    pr_r0802_0040_diagnostic display;   /* the last 0040 call's diagnostic */
} pr_r0802_0105_diagnostic;

pr_r0802_0105_status pr_r0802_0105(uint8_t *ram, size_t ram_size, pr_r0802_0105_cpu *cpu, const pr_r0802_0040_env *env,
                                   pr_r0802_0105_diagnostic *diagnostic);
#endif
