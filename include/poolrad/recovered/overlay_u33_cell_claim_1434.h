#ifndef POOLRAD_RECOVERED_OVERLAY_U33_CELL_CLAIM_1434_H
#define POOLRAD_RECOVERED_OVERLAY_U33_CELL_CLAIM_1434_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_1434_RAM_SIZE (1u << 20)

/* 14-word raw CPU state, laid out like the unit 0x3C6 +0CB9 model's. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_1434_cpu;

typedef enum {
    PR_U33_1434_OK = 0,
    PR_U33_1434_ARGUMENT,   /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U33_1434_CODE,       /* the +150E site or its stub is not the loaded-state original */
    PR_U33_1434_CALLEE,     /* the +0CB9 model stopped; its status is in *callee_status */
    PR_U33_1434_TRANSFER    /* +0CB9's RETF 0Eh did not return to the +1513 frame */
} pr_u33_1434_status;

/* Complete GAME.OVR unit 0x33:+1434..+1576 (323 bytes, 138 instructions),
 * entered after the far CALL with SS:[SP] return IP and [SP+2] return CS
 * and six argument words whose low bytes are read: [BP+6] r, [BP+8] b8,
 * [BP+0A] b10, [BP+0C] y, [BP+0E] x and [BP+10] slot q; RETF 0Ch.
 *
 * x outside 0..0Ah, y outside 0..5 (signed) or a zero availability byte
 * DS:[4440 + 108h*sign(DS:4658) + 42h*r + 0Bh*y + x] returns AL = 0.
 * Otherwise slot q's origin DS:[5F27+4q] := x + 6*b10 + 5*b8 + 16h and
 * DS:[5F28+4q] := y + 5*b8 + 0Ah, and the body far-calls unit 0x3C6
 * public entry 16 (+0CB9) through the stub at its +150E site with the far
 * pointer DS:[65B9+4q], k = 8 and two stack result bytes [BP-2]/[BP-3].
 * A nonzero [BP-2], a zero [BP-3] or DS:[2744 + 4*[BP-3]] = FFh returns
 * AL = 0; otherwise the availability byte is cleared and AL = 1. The
 * final flags are the deciding CMP's, or ADD DI,AX's on success.
 *
 * The far call is the loaded-state transfer: the site must hold 9A 0070
 * and the relocated segment word, the stub there EA 0CB9 and the unit's
 * segment; the merged-in public +0CB9 model then runs on the same RAM and
 * CPU words with its RETF policy CONTIGUOUS_PHYSICAL, the frame reading
 * this model also uses (CS two bytes above the IP word, physically). Its
 * declared LES seam and code-alias stops surface as CALLEE. Offsets wrap
 * at 16 bits, physical addresses at 1 MiB; every operand is reread from
 * RAM in source order. Dead intermediate flag writes are folded. */
pr_u33_1434_status pr_u33_cell_claim_1434(
    uint8_t *ram, size_t ram_size, pr_u33_1434_cpu *cpu, int *callee_status);

#endif
