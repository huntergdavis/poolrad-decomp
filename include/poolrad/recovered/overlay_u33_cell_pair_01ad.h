#ifndef POOLRAD_RECOVERED_OVERLAY_U33_CELL_PAIR_01AD_H
#define POOLRAD_RECOVERED_OVERLAY_U33_CELL_PAIR_01AD_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_01AD_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the +012B model. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_01ad_cpu;

typedef enum {
    PR_U33_01AD_OK = 0,
    PR_U33_01AD_ARGUMENT,   /* null RAM/CPU or RAM below 1 MiB; nothing changed */
    PR_U33_01AD_CODE,       /* a far-call site or stub inside +012B is not the loaded-state original */
    PR_U33_01AD_TRANSFER    /* a callee's RETF frame no longer returned to its call site */
} pr_u33_01ad_status;

/* Complete GAME.OVR unit 0x33:+01AD..+0220 (116 bytes, 54 instructions,
 * no conditional branch), entered after the far CALL: SS:[SP] return IP,
 * [SP+2] return CS, [SP+4] facing, [SP+6] y and [SP+8] x, each read as
 * its low byte; RETF 6. It computes (facing + 4) mod 8 by signed IDIV,
 * calls +012B at +01D3 with x, y, facing and its own BP, then calls +012B
 * at +0209 with x + DS:[2736+facing], y + DS:[273F+facing] and the
 * opposite facing, and returns the OR of the two result bytes in AL with
 * the OR's flags. Both near calls push CS and the return offset relative
 * to the entry cpu->ip (0x01AD in the game) and enter the +012B model at
 * ip - 0x82; that model in turn reaches the unit 0x3BD lookups through
 * the stubs recorded in its note. Every operand is reread from RAM. */
pr_u33_01ad_status pr_u33_cell_pair_01ad(
    uint8_t *ram, size_t ram_size, pr_u33_01ad_cpu *cpu);

#endif
