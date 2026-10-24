#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_WINDOW_16EC_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_WINDOW_16EC_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0709_printer_0768_cpu.h"
#include "poolrad/recovered/resident_0709_msgbox_0bf5_cpu.h"
#include "poolrad/recovered/resident_03f1_box_0000_cpu.h"

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_res_16ec_cpu;

typedef struct {
    size_t clear_loopbacks, clear_rect_loopbacks;                  /* 03F1:0137, each call */
    size_t display_printer_loopbacks, display_glyph_loopbacks;      /* the near 1816, each call */
    pr_r0709_printer_budget printer;                                /* 0709:0768, fresh on each entry (0768h or the 0999h resume) */
    pr_r0709_0bf5_env prompt;                                       /* 0709:0BF5, run when the printer stops at its CALL 0BF5 */
    unsigned prompt_rounds;                                         /* 0BF5 rounds allowed per printer call before LIMIT */
    pr_r03f1_box_env box;                                           /* 03F1:0000 and, through overlay stub 0428:002A, OVL_040C_015D */
    uint64_t delay_loopbacks;                                       /* 0709:0DBE (CRT Delay) */
    size_t clear17e3_loopbacks, clear17e3_rect_loopbacks;           /* the near 17E3 */
} pr_res_16ec_env;

typedef enum {
    PR_RES_16EC_RETURNED = 0,   /* RETF 0Ch executed back to the entry frame's return address */
    PR_RES_16EC_ARGUMENT,       /* null pointers, RAM not exactly 1 MiB, IP not 16ECh, entry SP outside [0800h,FFF0h]; nothing changed */
    PR_RES_16EC_LIMIT,          /* this body's *budget ran out at the next unexecuted instruction, or a callee's cap (diagnostic.callee_status) */
    PR_RES_16EC_CODE,           /* a call site is not the original opcode and offset (or near displacement); CPU at the call, before its frame */
    PR_RES_16EC_TRANSFER,       /* a callee returned elsewhere than after its call, or this RETF 0Ch popped another return address */
    PR_RES_16EC_CALLEE,         /* a callee stopped otherwise (its ARGUMENT, CODE, CALLEE, CONTRACT ...): diagnostic.callee and callee_status */
    PR_RES_16EC_OVERLAY,        /* 03F1:0000's overlay stub 0428:002A is not a loaded JMP FAR: the box stopped at the stub (its state kept) */
    PR_RES_16EC_EXIT,           /* 03F1:0000's EXIT (OVL_040C_015D's error branch reached Halt, or its ReadKey exited) */
    PR_RES_16EC_KEY             /* 0709:0BF5's ReadKey did not return (EXIT, BREAK, ...): diagnostic.prompt */
} pr_res_16ec_status;

typedef struct {
    uint16_t site;              /* the call site of the stopping call, 0 when none */
    int callee;                 /* -1 none; 1 StrStore 0B25, 2 clear 0137, 3 display 1816, 4 printer 0768, 5 prompt 0BF5, 6 box 0000,
                                   7 delay 0DBE, 8 clear 17E3 */
    int callee_status;          /* that callee's own status, -1 when none */
    unsigned prompt_rounds;     /* 0BF5 rounds taken in the last printer call */
    pr_r0709_printer_diagnostic printer;
    pr_r0709_0bf5_diagnostic prompt;
    pr_r03f1_box_diagnostic box;
} pr_res_16ec_diagnostic;

/* START.EXE resident 00BA [16EC,17E3): the window message, 247 bytes / 116 instructions, RETF 0Ch. Enter after the far call at IP 16EC:
 * SS:[SP] return IP, [SP+2] return CS, then [BP+6] the wait byte, [BP+8] the row byte, [BP+0Ah]/[BP+0Ch] the message far pointer (a Pascal
 * string) and [BP+0Eh]/[BP+10h] two words passed on to 1816. PUSH BP; MOV BP,SP; SUB SP,102h. SYSTEM StrStore 0AF8:0B25 copies the message
 * (at most FFh bytes) to the local [BP-100h]. If the mode byte DS:[49F3] is 5: 03F1:0137 clears (15h, 26h, row, 17h); the near 1816 shows
 * ([BP+0Eh], [BP+10h], 17h, row, 0); 0709:0768 prints the local string in the window (1, 0Ah, 15h, 26h, row+1, 17h). Otherwise the colour byte
 * [BP-101h] := 12h when DS:[6DDA] <> 0, else 11h after 03F1:0000 draws the box (0, 16h, 26h, 11h, 1); then the clear, 1816 and 0768 with that
 * colour (rows colour+1 and colour+2). Then, when [BP+6] <> 0, 0709:0DBE (CRT Delay of DS:[49E2]*225 ms) and the near 17E3 clear. MOV SP,BP;
 * POP BP; RETF 0Ch. Every callee runs as its raw model: StrStore flat, 0137, 1816, 0768, 0BF5, 0000 and 17E3 on the bus (EGA through
 * pr_ega), 0DBE flat; 0768's named STOP_BF5 is followed by 0709:0BF5 and the resume at 0999h, as on the CPU; 03F1:0000 follows the overlay
 * stub into OVL_040C_015D. Each LCALL's opcode and offset and each near CALL's displacement are checked at the site (CODE); far segment
 * words are read from RAM. This body's own accesses are flat (the stack frame and DS:49F3/6DDA). The flags follow the pinned DOSBox-X stored
 * rules (CMP byte: the six arithmetic flags; XOR AH,AH: logic; INC AX: CF kept); the callees keep theirs. Each instruction of THIS body
 * consumes one unit of *budget (the callees are bounded by env). Exactly 1 MiB of A20-off RAM; entry SP in [0800h,FFF0h], so the 102h-byte
 * frame and the callees' frames (0768 needs SP >= 0300h at its entry) stay inside the stack segment. Code is immutable. */
pr_res_16ec_status pr_res_00ba_window_16ec_cpu(pr_bus *bus, size_t ram_size, pr_res_16ec_cpu *cpu, size_t *budget, const pr_res_16ec_env *env,
                                               pr_res_16ec_diagnostic *diagnostic);
#endif
