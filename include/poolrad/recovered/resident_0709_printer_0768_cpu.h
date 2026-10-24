#ifndef POOLRAD_RECOVERED_RESIDENT_0709_PRINTER_0768_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_PRINTER_0768_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"

/* Raw-CPU, full-state model of START.EXE resident 0709:[0768,09E3) (635 bytes, 246 instructions, one RETF 10h), the game's
 * text-window printer (docs/re/RES_0709_PRINTER_0768_CPU.md). Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] a far
 * pointer, [SP+8] a flag byte, [SP+0Ah] a byte 06A7 reads, [SP+0Ch] y2, [SP+0Eh] x2, [SP+10h] y1, [SP+12h] x1 (bytes; the
 * names follow the comparisons below). DS:5E24 / DS:5E25 are the column / row bytes the body compares with x / y.
 *
 *   0768 PUSH BP; MOV BP,SP; SUB SP,126h; 0B25(far pointer -> SS:[BP-100h], max FFh)
 *   0783 x1 > 27h or y1 > 18h or x2 > 27h or y2 > 27h (unsigned bytes): JMP 09DD
 *   079E [5E24] outside [x1,x2] or [5E25] outside [y1,y2]: [5E24] := x1, [5E25] := y1
 *   07CA flag byte nonzero: CALL 0DD3(x1, y1, x2, y2); [5E24] := x1, [5E25] := y1
 *   07F0 i := 1 ([BP-103h]); n := [BP-100h] ([BP-101h]); n = 0: JMP 09DD; [BP-102h] := x2 - x1 + 1 (low byte)
 *   081A j := i ([BP-104h]); 0822 while j < n and 0DC0(text[j], set expanded from DS:0D16 by 0D12): j += 1
 *   085A while j < n and not in the set and text[j] <> 20h: j += 1
 *   08A1 text[j] <> 20h: 08B0 while j + 1 < n (16-bit signed) and text[j+1] in the set: j += 1
 *   08F2 j - i + [5E24] > x2 (16-bit signed): wrap (0955: [5E24] := x1, [5E25] += 1, CALL 0714); then if [5E25] > y2 and
 *        i < n: [5E24] := x1, [5E25] := y1, 0B0B(CS:074E -> SS:[BP-11Fh]), CALL 0BF5(0Dh, SS:BP-11Fh), 0899:0104, CALL 0DD3(window),
 *        CALL 06A7; else (no wrap) CALL 06A7. 0919..0952 is dead (0919 recomputes 08F2's value; 093B JNE is always taken).
 *   09BE i <= n: JMP 081A (09C8); 09CB [5E24] > x2: [5E24] := x1, [5E25] += 1
 *   09DD MOV SP,BP; POP BP; RETF 10h
 *
 * Composition: StrStore 0AF8:0B25, set expansion 0D12, set membership 0DC0 and string load 0B0B (SYSTEM leaf models),
 * resident 0899:0104, and the near callees 0DD3 (on the bus: EGA and ports), 06A7 (glyph 01DF, UpCase, CRT Delay; on the bus)
 * and 0714. 06A7 and 0714 are nested procedures: they receive this frame's BP and read its locals. Each far call's segment
 * word is read live; each callee must return to the instruction after its call (else CALLEE). The port environment is the
 * bus's (pr_port_env + pr_ega), used only inside 0DD3 and 06A7: this body issues no IN/OUT itself.
 *
 * 0BF5 is not yet modelled: STOP_BF5 is a NAMED STOP with PUSH CS; CALL 0BF5 executed (its two arguments on the stack); the
 * caller runs 0BF5 and resumes this model at IP 0999h with the post-callee state.
 *
 * Loop budgets: every loop-back JMP of this body counts against a declared budget, fresh on each entry (0768h or 0999h):
 * word_loopbacks for 09C8 (JMP 081A), scan_loopbacks for 0858, 089F and 08F0 together. When a JMP would run past its budget
 * the model stops LIMIT with IP at that JMP and nothing of it executed; diagnostic.site is the JMP's offset. The scans are
 * bounded by n <= 255 per pass; the word loop is not bounded for every input (docs: the x1 > x2 case).
 * Exactly 1 MiB of RAM on the bus with the relocated code at CS; IP 0768h or 0999h; entry SP in [0300h,FFECh] at 0768h. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0709_printer_0768_regs;

typedef struct {
    size_t clear_loopbacks, clear_rect_loopbacks;                 /* 0DD3's budgets */
    size_t text_loopbacks, text_glyph_loopbacks;                  /* 06A7's budgets */
    uint64_t text_delay_loopbacks;                                /* 06A7's CRT Delay budget */
    unsigned skip_loopbacks;                                      /* 0714's budget */
    size_t word_loopbacks;                                        /* this body: JMP 081A at 09C8 */
    size_t scan_loopbacks;                                        /* this body: JMPs at 0858, 089F, 08F0 together */
} pr_r0709_printer_budget;

typedef enum {
    PR_R0709_PRINTER_RETURNED = 0,  /* RETF 10h executed */
    PR_R0709_PRINTER_ARGUMENT,      /* null pointers, RAM size, IP not an entry, entry SP out of range; nothing changed */
    PR_R0709_PRINTER_STOP_BF5,      /* PUSH CS; CALL 0BF5 executed (resume at 0999h) */
    PR_R0709_PRINTER_LIMIT,         /* a declared budget ran out: this body's loop-backs or a callee's (see the diagnostic) */
    PR_R0709_PRINTER_CALLEE         /* a callee stopped otherwise (port refused, CODE, TRANSFER) or returned elsewhere */
} pr_r0709_printer_status;

typedef struct {
    uint16_t site;       /* the call site of a CALLEE/LIMIT/STOP_BF5 stop or the JMP of a loop-back LIMIT, else 0 */
    int callee_status;   /* that callee's status, else -1 (also -1 for a loop-back LIMIT) */
    int nested[4];       /* the callee's own diagnostic fields when it has them (06A7: callee, status, glyph callee, glyph
                            status; 0DD3: callee, status, -1, -1), else -1 */
} pr_r0709_printer_diagnostic;

pr_r0709_printer_status pr_r0709_printer_0768_cpu(pr_bus *bus, size_t ram_size, pr_r0709_printer_0768_regs *cpu,
                                                  const pr_r0709_printer_budget *budget, pr_r0709_printer_diagnostic *diagnostic);
#endif
