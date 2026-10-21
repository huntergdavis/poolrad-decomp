#ifndef POOLRAD_RECOVERED_RESIDENT_0802_0193_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0802_0193_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_0802_0105_cpu.h"
#include "poolrad/recovered/resident_0802_0175_cpu.h"
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"

#define PR_R0802_0193_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of START.EXE resident 0802:[0193,01ED) (90 bytes, 40 instructions, RETF 6), the game's status-line prompt and the last leaf
 * of the t30 DISPLAY stack (docs/re/RES_0802_0193_CPU.md). Entered by PUSH CS; near CALL (025D at +02AA, 0716 at +084A) or a far call: [SP] IP, [SP+2] CS,
 * [SP+4] the far address of the Pascal message, [SP+8] the attribute index (a byte in a word slot).
 *
 *   0193 PUSH BP; MOV BP,SP; SUB SP,2Ah
 *   0199 message -> [BP-29h] (StrStore, max 40)                         LCALL 0AF8:0B25
 *   01AC blank row 24:  0175(column 0, row 18h, attribute 0, count 28h)   PUSH CS; CALL 0175
 *   01BC write it:      0105(column 0, row 18h, attribute [BP+0Ah], the copy)   PUSH CS; CALL 0105
 *   01CF wait:          key := 0899:0059 (ReadKey with hotkeys)           LCALL 0899:0059; [BP-2Ah] := AL
 *   01D7 blank row 24 again: 0175(0, 18h, 0, 28h)
 *   01E7 MOV SP,BP; POP BP; RETF 6
 * The prompt path: the message is written on row 24, ReadKey waits (and drains type-ahead), row 24 is blanked twice (before and after).
 *
 * Composition, each through its merged model and interface: StrStore 0B25 (segment word read live from CS:01AA), the block filler 0175 (twice, near-call
 * displacements read live from CS:01BA and CS:01E5), the string writer 0105 (displacement from CS:01CD), and ReadKey 0899:0059 (segment word from CS:01D2),
 * which itself composes the 08AE driver, Crt ReadKey/KeyPressed (INT 16h the declared service), SYSTEM leaves and QUIT. Each callee must return to the
 * instruction after its call (else CALLEE, or DISPLAY for 0105/0175). ReadKey's EXIT (hotkey 03 with ParamStr 'STING'), BREAK and other stops are
 * reported as KEY with ReadKey's own status and diagnostic: ReadKey may not return, and then neither does this prompt.
 * The declared environment is the display handler (INT 10h through Dos.Intr) and ReadKey's environment (driver, INT 16h keyboard, QUIT's INT 10h).
 * Exactly 1 MiB of A20-off RAM, IP = 0193h, entry SP in [0190h,FFF0h] (the 2Ah-byte frame and the composed frames lie below it). No flag is masked. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0802_0193_cpu;

typedef struct {
    pr_r0802_0040_env display;    /* the INT 10h handler for 0040's Intr calls */
    pr_r0899_key_env key;         /* ReadKey's environment: driver, keyboard, QUIT's INT 10h handler, drain limit */
} pr_r0802_0193_env;

typedef enum {
    PR_R0802_0193_RETURNED = 0,   /* RETF 6 executed */
    PR_R0802_0193_ARGUMENT,       /* null pointers, RAM size, IP not 0193h, entry SP out of range; nothing changed */
    PR_R0802_0193_DISPLAY,        /* a 0175 or 0105 call did not return normally (diagnostic: the callee site and status) */
    PR_R0802_0193_KEY,            /* ReadKey did not return (diagnostic: its status: EXIT, BREAK, CRT, DRIVER, LIMIT ...) */
    PR_R0802_0193_CALLEE          /* StrStore stopped, or a callee returned somewhere other than the instruction after its call */
} pr_r0802_0193_status;

typedef struct {
    uint16_t site;                         /* IP of the call this stop concerns (0 when none) */
    int callee_status;                     /* that callee's own status (-1 when none) */
    pr_r0802_0105_diagnostic string;       /* the last 0105 call */
    pr_r0802_0175_diagnostic fill;         /* the last 0175 call */
    int key_status;                        /* pr_r0899_key_status of the ReadKey call (-1 when not called) */
    pr_r0899_key_diagnostic key;           /* its diagnostic */
} pr_r0802_0193_diagnostic;

pr_r0802_0193_status pr_r0802_0193(uint8_t *ram, size_t ram_size, pr_r0802_0193_cpu *cpu, const pr_r0802_0193_env *env,
                                   pr_r0802_0193_diagnostic *diagnostic);
#endif
