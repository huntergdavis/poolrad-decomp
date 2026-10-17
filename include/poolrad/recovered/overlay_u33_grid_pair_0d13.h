#ifndef POOLRAD_RECOVERED_OVERLAY_U33_GRID_PAIR_0D13_H
#define POOLRAD_RECOVERED_OVERLAY_U33_GRID_PAIR_0D13_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_0D13_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the merged unit 0x33 and 0x9E models. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_0d13_cpu;

typedef enum {
    PR_U33_0D13_OK = 0,
    PR_U33_0D13_ARGUMENT,   /* null RAM/CPU or RAM not exactly 1 MiB; nothing changed */
    PR_U33_0D13_CODE,       /* a far-call site or its stub is not the loaded-state original */
    PR_U33_0D13_CALLEE,     /* the merged entry-8 chain stopped (its status is in *chain_status) */
    PR_U33_0D13_TRANSFER    /* a RETF frame no longer returned to its call site */
} pr_u33_0d13_status;

/* Complete GAME.OVR unit 0x33:+0D13..+0E9D (395 bytes, 163 instructions),
 * entered after the far CALL with no arguments; RETF. Five PUSH CS/CALL
 * +08A4 calls (the merged pr_u33_byte_flags_08a4, entered at ip - 46F)
 * turn class bits 2, 4, 40h, 8 and 80h into a threshold of 1, 3, 4, 7 or
 * 0 (last match wins, 1 otherwise). Then for column 0..31h and row
 * 1..18h of the far grid DS:6716/6718, when both the cell and the cell
 * above it index 16h in the DS table at 2747, a (1, 64h) roll at or
 * below the threshold rolls (1, 0Ah): above 8 a (1, 2) roll plus 4,
 * otherwise (value + 1) / 2; class bit 40h then replaces it with a
 * (1, 3) roll plus 3; the cell is written as value + 23h and, below 5,
 * the cell above as value + 1Fh. The far calls go through stub 00B0:0048
 * to unit 0x9E entry 8 (the merged compose chain, SYSTEM at 0AF8:11C0,
 * seed at DS:4428) with the relocated segment word and the loaded stub
 * read from RAM. cpu->ip is the body's load offset (0x0D13 in the game).
 * Every operand is reread from RAM in source order. */
pr_u33_0d13_status pr_u33_grid_pair_0d13(
    uint8_t *ram, size_t ram_size, pr_u33_0d13_cpu *cpu, int *chain_status);

#endif
