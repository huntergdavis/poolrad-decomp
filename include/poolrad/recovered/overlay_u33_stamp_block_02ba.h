#ifndef POOLRAD_RECOVERED_OVERLAY_U33_STAMP_BLOCK_02BA_H
#define POOLRAD_RECOVERED_OVERLAY_U33_STAMP_BLOCK_02BA_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_02BA_RAM_SIZE (1u << 20)

/* 14-word raw CPU state, laid out like the +0221 model's. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_02ba_cpu;

typedef enum {
    PR_U33_02BA_OK = 0,
    PR_U33_02BA_ARGUMENT,   /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U33_02BA_TRANSFER    /* a +0221 RETF 8 did not return to its call frame */
} pr_u33_02ba_status;

/* Complete GAME.OVR unit 0x33:+02BA..+0376 (189 bytes, 86 instructions,
 * five conditional branches), entered after the far CALL with SS:[SP]
 * return IP, [SP+2] return CS and [SP+4] the caller's frame offset L (the
 * static link); RETF 2. Through +0221 (glyph + 1 written at the caller's
 * frame-relative position, clipped to 50 by 25) it stamps glyph 16h at
 * rows 2..4 by columns 0..5. When SS:[L-8] is 1 it then stamps, for rows
 * 2..4, glyph 4 at column row-1, 3 at column row and 0Dh at column row+1;
 * when it is 3, glyph 8 at (row 2, column 1) and 0 at (row 4, column 5).
 * The final flags are the last CMP's, or +0221's after the selector-3
 * calls. The eleven to twenty-one near calls push CS and the return offset
 * relative to the entry cpu->ip (0x02BA in the game) and enter the merged
 * +0221 model at ip - 99h on the same RAM and CPU words. Every operand is
 * reread from RAM; offsets wrap at 16 bits, physical addresses at 1 MiB;
 * the RETF frame is read physically contiguous. Dead intermediate flag
 * writes are folded: +0221 overwrites every arithmetic flag before it
 * returns. */
pr_u33_02ba_status pr_u33_stamp_block_02ba(uint8_t *ram, size_t ram_size, pr_u33_02ba_cpu *cpu);

#endif
