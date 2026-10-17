#ifndef POOLRAD_RECOVERED_OVERLAY_U33_GRID_BANDS_0E9E_H
#define POOLRAD_RECOVERED_OVERLAY_U33_GRID_BANDS_0E9E_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_0E9E_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with the merged unit 0x33 and 0x9E models. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_0e9e_cpu;

typedef enum {
    PR_U33_0E9E_OK = 0,
    PR_U33_0E9E_ARGUMENT,   /* null RAM/CPU or RAM not exactly 1 MiB; nothing changed */
    PR_U33_0E9E_CODE,       /* a far-call site or its stub is not the loaded-state original */
    PR_U33_0E9E_CALLEE,     /* the merged entry-8 chain stopped (its status is in *chain_status) */
    PR_U33_0E9E_TRANSFER    /* a RETF frame no longer returned to its call site */
} pr_u33_0e9e_status;

/* Complete GAME.OVR unit 0x33:+0E9E..+11B3 (790 bytes, 323 instructions),
 * entered after the far CALL with no arguments; RETF. Four PUSH CS/CALL
 * +08A4 calls (the merged pr_u33_byte_flags_08a4, entered at ip - 5FA)
 * adjust five band widths from their defaults 0, 6, 0Fh, 28h and 0 by
 * class bits 2, 4, 40h and 80h. Then for column 0..31h and row 0..18h of
 * the far grid DS:6716/6718, an eligible cell (DS table 2747 + 4 * cell
 * byte equals 16h) takes a (1, 0FFh) roll and the band it falls in picks
 * the write: the first band rolls (1, 4), on 4 with an eligible cell
 * above writes 41h above and 40h here, and below 4 writes the roll +
 * 3Ch; the next bands write a (1, 3) roll + 36h, a (1, 4) roll + 29h, a
 * (1, 0Ah) roll folded by three + 38h, and a (1, 4) roll + 2Dh after an
 * optional (1, 3) reroll when the 4 is not covered by class bit 40h; a
 * roll beyond every band writes nothing. The far calls go through stub
 * 00B0:0048 to unit 0x9E entry 8 (the merged compose chain, SYSTEM at
 * 0AF8:11C0, seed at DS:4428) with the relocated segment word and the
 * loaded stub read from RAM. cpu->ip is the body's load offset (0x0E9E
 * in the game). Every operand is reread from RAM in source order. */
pr_u33_0e9e_status pr_u33_grid_bands_0e9e(
    uint8_t *ram, size_t ram_size, pr_u33_0e9e_cpu *cpu, int *chain_status);

#endif
