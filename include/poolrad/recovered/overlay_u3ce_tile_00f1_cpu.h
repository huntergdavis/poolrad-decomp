#ifndef POOLRAD_RECOVERED_OVERLAY_U3CE_TILE_00F1_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3CE_TILE_00F1_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0464_compose_08d4_cpu.h"

/* Raw-CPU, full-state model of GAME.OVR unit 3CEh +[00F1,014E) (93 bytes, 38 instructions, RETF 8): place one tile in
 * the off-screen object DS:4A20 through 0464:08D4 (the tile compositor). Tile byte [BP+8] above 7Fh selects the tile set
 * at far DS:5E56 with the tile [BP+8] AND 7Fh; 00h..7Fh selects the tile set at far DS:5E52 with the tile unchanged. The
 * 08D4 frame is (object DS:4A20/4A22, tile set, flag 0 in AL, tile, sign-extended byte [BP+0Ah], sign-extended byte
 * [BP+0Ch]); AH of each pushed byte word is what the preceding instruction left. The word [BP+6] is not read.
 * Entered after a far call: [SP] IP, [SP+2] CS, then the four argument words; RETF 8. CS is the unit's load segment;
 * the two far-call segment words (CS:0120, CS:0148) are loader fixups, read live. 08D4 runs through its merged model on
 * the same RAM. Operands are reread in source order; offsets wrap at 16 bits and physical addresses at 1 MiB.
 * See docs/re/OVL_03CE_00F1_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_ovl03ce_00f1_regs;

typedef struct {
    size_t max_loopbacks;       /* passed to 08D4 */
} pr_ovl03ce_00f1_env;

typedef enum {
    PR_OVL03CE_00F1_OK = 0,     /* RETF 8 executed */
    PR_OVL03CE_00F1_ARGUMENT,   /* null pointers, RAM not exactly 1 MiB, IP not 00F1h; nothing changed */
    PR_OVL03CE_00F1_CALLEE,     /* 08D4 stopped (diagnostic: site and its status); state as it left it */
    PR_OVL03CE_00F1_TRANSFER    /* 08D4 returned somewhere other than the instruction after its call */
} pr_ovl03ce_00f1_status;

typedef struct {
    uint16_t site;              /* the far-call site of the stopping 08D4 call, 0 when none */
    int status;                 /* 08D4's status there, -1 when none */
    unsigned calls;             /* 08D4 calls entered */
    pr_r0464_compose_cpu_diagnostic compose;
} pr_ovl03ce_00f1_diagnostic;

pr_ovl03ce_00f1_status pr_ovl03ce_00f1_cpu(pr_bus *bus, size_t ram_size, pr_ovl03ce_00f1_regs *cpu,
                                           const pr_ovl03ce_00f1_env *env, pr_ovl03ce_00f1_diagnostic *diagnostic);
#endif
