#ifndef POOLRAD_RECOVERED_OVERLAY_U33_STAMP_SQUARE_0377_H
#define POOLRAD_RECOVERED_OVERLAY_U33_STAMP_SQUARE_0377_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_0377_RAM_SIZE (1u << 20)

/* 14-word raw CPU state, laid out like the +0221 model's. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_0377_cpu;

typedef enum {
    PR_U33_0377_OK = 0,
    PR_U33_0377_ARGUMENT,   /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U33_0377_TRANSFER    /* a +0221 RETF 8 did not return to its call frame */
} pr_u33_0377_status;

/* Complete GAME.OVR unit 0x33:+0377..+040E (152 bytes, 82 instructions,
 * one conditional branch), entered after the far CALL with SS:[SP] return
 * IP, [SP+2] return CS and [SP+4] the caller's frame offset L; RETF 2.
 * When SS:[L-7] is 1 it stamps through +0221 glyph 5 at (row 0, columns 3
 * and 4) and 0Ah at (row 1, columns 3 and 4); otherwise glyph 16h at the
 * same four positions. The final flags are the last +0221 call's. The
 * four near calls push CS and the return offset relative to the entry
 * cpu->ip (0x0377 in the game) and enter the merged +0221 model at
 * ip - 156h on the same RAM and CPU words. Every operand is reread from
 * RAM; offsets wrap at 16 bits, physical addresses at 1 MiB; the RETF
 * frame is read physically contiguous. */
pr_u33_0377_status pr_u33_stamp_square_0377(uint8_t *ram, size_t ram_size, pr_u33_0377_cpu *cpu);

#endif
