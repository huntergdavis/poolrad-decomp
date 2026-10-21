#ifndef POOLRAD_RECOVERED_RESIDENT_0709_MSGBOX_0BF5_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_MSGBOX_0BF5_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0709_space_05eb_cpu.h"
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"

/* Raw-CPU, full-state model of START.EXE resident 0709:[0BF5,0C3F) (74 bytes, 38 instructions, RETF 6): the game's message-line
 * prompt (docs/re/RES_0709_MSGBOX_0BF5_CPU.md). Entered by a far call: [SP] IP, [SP+2] CS, [SP+4] the far address of the Pascal
 * message (offset, segment), [SP+8] the colour (a byte in a word slot).
 *
 *   0BF5 PUSH BP; MOV BP,SP; SUB SP,2Ah
 *   0BFB message -> [BP-29h] (StrStore, max 28h)                          LCALL 0AF8:0B25
 *   0C0E clear the row:  05EB(count 28h, colour 0, y 18h, x 0)             PUSH CS; CALL near 05EB
 *   0C1E draw it:        0626(message copy, colour [BP+0Ah], y 18h, x 0)    PUSH CS; CALL near 0626
 *   0C31 wait:           key := 0899:0059 (ReadKey with hotkeys)            LCALL 0899:0059; [BP-2Ah] := AL
 *   0C39 MOV SP,BP; POP BP; RETF 6                                         (AL, the key, is the result)
 *
 * Composition, each through its merged model: StrStore 0B25 (segment word read live from CS:0C0C), the space/glyph filler 05EB and the
 * string printer 0626 (near-call displacements read live from CS:0C1C and CS:0C2F; both reach the +01DF glyph writer and the EGA through
 * the bus), and ReadKey 0899:0059 (segment word from CS:0C34) with its closure (INT 16h and the 08AE driver the declared environment).
 * Each callee must return to the instruction after its call (else CALLEE). 05EB/0626 stops are reported as SPACE/PRINTER with the
 * callee's own status and diagnostic; ReadKey's EXIT (hotkey 03 with ParamStr 'STING'), BREAK, LIMIT and the rest as KEY, with ReadKey's
 * status: ReadKey may not return, and then neither does this prompt. Exactly 1 MiB of A20-off RAM, IP = 0BF5h, entry SP in [016Ah,FFF6h]
 * (the 2Ah-byte frame and the composed frames lie below it; the argument words and RETF frame lie above it). Reads of the 05EB/0626
 * stack words stay in ordinary RAM: SS/DS and the string segment exclude A0000..AFFFF. No flag is masked. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0709_0bf5_cpu;

typedef struct {
    pr_r0899_key_env key;               /* ReadKey's environment: driver, keyboard, QUIT's INT 10h handler, drain limit */
    size_t space_loopbacks;             /* 05EB's glyph budget (its max_loopbacks) */
    size_t printer_loopbacks;           /* 0626's outer budget */
    size_t glyph_loopbacks;             /* 0626's fresh glyph budget per call */
} pr_r0709_0bf5_env;

typedef enum {
    PR_R0709_0BF5_RETURNED = 0,   /* RETF 6 executed */
    PR_R0709_0BF5_ARGUMENT,       /* null pointers, RAM size, IP not 0BF5h, entry SP out of range; nothing changed */
    PR_R0709_0BF5_SPACE,          /* the 05EB call did not return normally (diagnostic: its status) */
    PR_R0709_0BF5_PRINTER,        /* the 0626 call did not return normally (diagnostic: its status) */
    PR_R0709_0BF5_KEY,            /* ReadKey did not return (diagnostic: its status: EXIT, BREAK, CRT, DRIVER, LIMIT ...) */
    PR_R0709_0BF5_CALLEE          /* a callee returned somewhere other than the instruction after its call, or a call site's target is wrong */
} pr_r0709_0bf5_status;

typedef struct {
    uint16_t site;                         /* IP of the call this stop concerns (0 when none) */
    int callee_status;                     /* that callee's own status (-1 when none) */
    pr_r0709_space_cpu_diagnostic space;   /* the 05EB call */
    pr_r0709_printer_cpu_diagnostic printer;   /* the 0626 call */
    int key_status;                        /* pr_r0899_key_status of the ReadKey call (-1 when not called) */
    pr_r0899_key_diagnostic key;           /* its diagnostic */
} pr_r0709_0bf5_diagnostic;

pr_r0709_0bf5_status pr_r0709_msgbox_0bf5(pr_bus *bus, size_t ram_size, pr_r0709_0bf5_cpu *cpu, const pr_r0709_0bf5_env *env,
                                          pr_r0709_0bf5_diagnostic *diagnostic);
#endif
