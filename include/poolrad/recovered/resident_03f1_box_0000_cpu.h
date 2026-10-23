#ifndef POOLRAD_RECOVERED_RESIDENT_03F1_BOX_0000_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_03F1_BOX_0000_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u40c_tile_015d_cpu.h"

/* Raw-CPU, full-state model of START.EXE resident 03F1:[0000,0137) (311 bytes, 145 instructions, RETF 0Ah): draw a
 * tile box. Arguments: [BP+6] style byte, [BP+8] bottom row, [BP+0Ah] right column, [BP+0Ch] top row, [BP+0Eh] left
 * column (the four coordinate bytes are first adjusted in place: left and top DEC, right and bottom INC). The style's
 * three tile numbers are style * 3 + 114h + {0 corner, 1 vertical edge, 2 horizontal edge} ([BP-3]). Four corners are
 * placed, then the horizontal edges for each interior column ([BP-1] from left + 1 to right - 1, signed) and the vertical
 * edges for each interior row, every tile through overlay stub 0428:002A (unit 40Ch entry 2, +015D) with the frame
 * (column, row, number, 0, 0); AH of each pushed byte word is what the preceding instruction left.
 * The stub is executed as on the CPU: a loaded overlay's entry is JMP FAR (EAh) to the unit's segment:015D, followed into
 * OVL_040C_015D's model; an unloaded entry (INT 3Fh, the overlay manager) stops with OVERLAY. The stub's segment word is
 * a loader relocation, read live at each call site. Entered after a far call: [SP] IP, [SP+2] CS, then the five argument
 * words; RETF 0Ah. See docs/re/RES_03F1_BOX_0000_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r03f1_box_regs;

typedef struct {
    pr_ovl040c_015d_env tile;   /* the environment of every 015D call */
    size_t max_loopbacks;       /* this body's backward jumps before the LIMIT stop */
} pr_r03f1_box_env;

typedef enum {
    PR_R03F1_BOX_OK = 0,        /* RETF 0Ah executed */
    PR_R03F1_BOX_ARGUMENT,      /* null pointers, RAM not exactly 1 MiB, IP not 0000h; nothing changed */
    PR_R03F1_BOX_CALLEE,        /* 015D stopped (CALLEE/TRANSFER/ARGUMENT; diagnostic: site and its status) */
    PR_R03F1_BOX_TRANSFER,      /* a call returned somewhere other than the instruction after it */
    PR_R03F1_BOX_LIMIT,         /* max_loopbacks backward jumps taken; stopped before the next */
    PR_R03F1_BOX_OVERLAY,       /* the stub entry is not a loaded JMP FAR to +015D; stopped at the stub with the frame pushed */
    PR_R03F1_BOX_CONTRACT,      /* 015D's CONTRACT stop (EGA attached), state as it left it */
    PR_R03F1_BOX_EXIT           /* 015D's EXIT (its error branch reached Halt, or ReadKey exited) */
} pr_r03f1_box_status;

typedef struct {
    uint16_t site;              /* the call site of the stopping tile call, 0 when none */
    int status;                 /* 015D's status there, -1 when none */
    unsigned calls;             /* tile calls made */
    pr_ovl040c_015d_diagnostic tile;   /* the last 015D call's diagnostic */
} pr_r03f1_box_diagnostic;

pr_r03f1_box_status pr_r03f1_box_0000_cpu(pr_bus *bus, size_t ram_size, pr_r03f1_box_regs *cpu, const pr_r03f1_box_env *env,
                                          pr_r03f1_box_diagnostic *diagnostic);
#endif
