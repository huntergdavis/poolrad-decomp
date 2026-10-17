#ifndef POOLRAD_RECOVERED_OVERLAY_U33_GRID_ROLL_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U33_GRID_ROLL_0000_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_0000_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the merged unit 0x9E entry-8 chain. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_0000_cpu;

typedef enum {
    PR_U33_0000_OK = 0,
    PR_U33_0000_ARGUMENT,   /* null RAM/CPU or RAM not exactly 1 MiB; nothing changed */
    PR_U33_0000_CODE,       /* a far-call site or its stub is not the loaded-state original */
    PR_U33_0000_CALLEE,     /* the merged entry-8 chain stopped (its status is in *chain_status) */
    PR_U33_0000_TRANSFER    /* a RETF frame no longer returned to its call site */
} pr_u33_0000_status;

/* Complete GAME.OVR unit 0x33:+0000..+012A (299 bytes, 123 instructions),
 * entered after the far CALL with no arguments; RETF. For every column
 * 0..31h and row 0..18h it reads byte +7 of cell 50*row + column in the
 * far grid DS:6716/6718 and, when the DS table byte 2747 + 4*that byte is
 * 16h, rolls once through unit 0x9E public entry 8 (+0DE3, stub
 * 00B0:0048, count 1, range 64h): 62h writes 1Ah and 63h writes 1Bh to
 * the cell; 64h, unless DS:84DC is 0Ah, rolls again and on exactly 1 rolls
 * (1, 0Ah), writing 1Ch for 1..6 and 1Dh for 7..0Ah. The three far calls
 * are modelled as the loaded-state transfer: the relocated segment word
 * is read from each call site in RAM (cs:(ip+4E), cs:(ip+BA),
 * cs:(ip+C9)), the five-byte far jump at that paragraph's offset 48
 * supplies the callee CS:IP, the merged pr_u9e_entry8_fullstate_compose_0de3
 * runs on the same RAM and CPU words (SYSTEM at 0AF8:11C0, seed at
 * DS:4428), and each RETF frame is checked. cpu->ip is the body's load
 * offset (0 in the game). Every operand is reread from RAM. */
pr_u33_0000_status pr_u33_grid_roll_0000(
    uint8_t *ram, size_t ram_size, pr_u33_0000_cpu *cpu, int *chain_status);

#endif
