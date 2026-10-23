#ifndef POOLRAD_RECOVERED_OVERLAY_U3B6_WINDOW_0000_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B6_WINDOW_0000_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u3ce_tile_00f1_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 3B6h +[0000,0206) (518 bytes, 211 instructions, RETF 4): draw a 5 x 5
 * window of the 44-column tile map DS:3648. For row r ([BP-8], 0..4) and column k ([BP-7], 0..4):
 *   x := low byte of (k + word [BP+8]) ([BP-4]); y := low byte of (r + word [BP+6]) ([BP-5]);
 *   tile := DS:[3648 + y * 2Ch + x] - 1 ([BP-6]; x and y sign-extended, the product and sum wrap at 16 bits);
 *   for each record i = 1..9 ([BP-3]) whose words DS:0C30 + 6i and DS:0C32 + 6i equal x and y (sign-extended to 32 bits
 *   against the zero-extended word): records 3..9 replace the tile with low byte of (DS:0C34 + 6i) - 1, subject to the
 *   window-0 words at far DS:49D2: i = 3 when +318h is not 00FFh; i = 4 unless +3A4h = 0, (+2F8h AND 8) = 0 and
 *   +33Eh is non-zero; i = 5 when (+340h AND 2) = 0; i = 6 always; i = 7 when (+340h AND 1) = 0; i = 8 and 9 when +366h
 *   is above 00FDh (unsigned). Records 1 and 2 never replace it. Later records override earlier ones.
 * Then the tile goes through overlay stub 03E9:002A (unit 3CEh entry 2, +00F1) with the frame (1 in AL, tile, column
 * [BP-2], row [BP-1]); [BP-1] starts at 0 for each row and steps by 3 per column, [BP-2] starts at 0 and steps by 3 per
 * row. AH of each pushed byte word is what the preceding instruction left.
 * The stub is executed as on the CPU: a loaded overlay's entry is JMP FAR (EAh) to the unit's segment:00F1, followed into
 * OVL_03CE_00F1's model; an unloaded entry (INT 3Fh, the overlay manager) stops with OVERLAY. The stub's segment word
 * (CS:01E4) is a loader fixup, read live. Entered after a far call: [SP] IP, [SP+2] CS, then the two argument words;
 * RETF 4. See docs/re/OVL_03B6_0000_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_ovl03b6_0000_regs;

typedef struct {
    pr_ovl03ce_00f1_env tile;   /* the environment of every 00F1 call */
    size_t max_loopbacks;       /* this body's backward jumps before the LIMIT stop */
} pr_ovl03b6_0000_env;

typedef enum {
    PR_OVL03B6_0000_OK = 0,     /* RETF 4 executed */
    PR_OVL03B6_0000_ARGUMENT,   /* null pointers, RAM not exactly 1 MiB, IP not 0000h; nothing changed */
    PR_OVL03B6_0000_CALLEE,     /* 00F1 stopped (CALLEE/TRANSFER/ARGUMENT; diagnostic: site and its status) */
    PR_OVL03B6_0000_TRANSFER,   /* a call returned somewhere other than the instruction after it */
    PR_OVL03B6_0000_LIMIT,      /* max_loopbacks backward jumps taken; stopped before the next */
    PR_OVL03B6_0000_OVERLAY     /* the stub entry is not a loaded JMP FAR to +00F1; stopped at the stub with the frame pushed */
} pr_ovl03b6_0000_status;

typedef struct {
    uint16_t site;              /* the call site of the stopping tile call, 0 when none */
    int status;                 /* 00F1's status there, -1 when none */
    unsigned calls;             /* tile calls made */
    pr_ovl03ce_00f1_diagnostic tile;   /* the last 00F1 call's diagnostic */
} pr_ovl03b6_0000_diagnostic;

pr_ovl03b6_0000_status pr_ovl03b6_0000_cpu(pr_bus *bus, size_t ram_size, pr_ovl03b6_0000_regs *cpu,
                                           const pr_ovl03b6_0000_env *env, pr_ovl03b6_0000_diagnostic *diagnostic);
#endif
