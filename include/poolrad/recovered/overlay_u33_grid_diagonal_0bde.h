#ifndef POOLRAD_RECOVERED_OVERLAY_U33_GRID_DIAGONAL_0BDE_H
#define POOLRAD_RECOVERED_OVERLAY_U33_GRID_DIAGONAL_0BDE_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_0BDE_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the merged unit 0x33 and 0x9E models. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_0bde_cpu;

typedef enum {
    PR_U33_0BDE_OK = 0,
    PR_U33_0BDE_ARGUMENT,   /* null RAM/CPU or RAM not exactly 1 MiB; nothing changed */
    PR_U33_0BDE_CODE,       /* a far-call site or its stub is not the loaded-state original */
    PR_U33_0BDE_CALLEE,     /* the merged entry-8 chain stopped (its status is in *chain_status) */
    PR_U33_0BDE_TRANSFER    /* a RETF frame no longer returned to its call site */
} pr_u33_0bde_status;

/* Complete GAME.OVR unit 0x33:+0BDE..+0D12 (309 bytes, 138 instructions),
 * entered after the far CALL with no arguments; RETF. Two PUSH CS/CALL
 * +08A4 calls (the merged pr_u33_byte_flags_08a4, entered at ip - 33A)
 * raise a threshold from 0 to 23h (class bit 20h) or 4Bh (bit 10h). A
 * (1, 64h) roll above it returns at once. Otherwise x := 22h minus a
 * (5, 4) roll, lowered until x + 2 is a multiple of 7, and for row
 * 0..18h with x at most 31h the far grid DS:6716/6718 cells (x, row) and
 * (x + 1, row) are written 32h and 33h; a (1, 14h) roll of 1, or an odd
 * pair count, calls the +0B97 pair writer (folded here from its original
 * 71 bytes: it writes 34h and 35h at (x, row) and (x + 1, row) and
 * increments this frame's pair count through the pushed BP); x then
 * advances with the row. If no pair was placed, a (1, 9) roll picks a
 * row and two more pairs are written at the saved x plus that row. The
 * far calls go through stub 00B0:0048 to unit 0x9E entry 8 (the merged
 * compose chain, SYSTEM at 0AF8:11C0, seed at DS:4428) with the relocated
 * segment word and the loaded stub read from RAM. cpu->ip is the body's
 * load offset (0x0BDE in the game). Every operand is reread from RAM. */
pr_u33_0bde_status pr_u33_grid_diagonal_0bde(
    uint8_t *ram, size_t ram_size, pr_u33_0bde_cpu *cpu, int *chain_status);

#endif
