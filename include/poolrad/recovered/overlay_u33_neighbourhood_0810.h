#ifndef POOLRAD_RECOVERED_OVERLAY_U33_NEIGHBOURHOOD_0810_H
#define POOLRAD_RECOVERED_OVERLAY_U33_NEIGHBOURHOOD_0810_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_0810_RAM_SIZE (1u << 20)

/* 14-word raw CPU state shared with every composed callee model. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_0810_cpu;

typedef enum {
    PR_U33_0810_OK = 0,
    PR_U33_0810_ARGUMENT,   /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U33_0810_CODE,       /* a callee's far-call site or stub is not the loaded-state original */
    PR_U33_0810_CALLEE,     /* a callee model stopped; which and its status are in *callee */
    PR_U33_0810_TRANSFER    /* a callee's RETF did not return to its call frame */
} pr_u33_0810_status;

/* Which callee stopped, for PR_U33_0810_CALLEE (or its status for the others). */
typedef struct {
    uint16_t site;          /* the offset of the near call, relative to the body (0 if none) */
    int status;             /* the callee model's own status, or the dice chain's for +0000 */
} pr_u33_0810_callee;

/* Complete GAME.OVR unit 0x33:+0810..+08A3 (148 bytes, 71 instructions,
 * two conditional branches), entered after the far CALL with SS:[SP]
 * return IP and [SP+2] return CS and no arguments; RETF. For dy = -2..2
 * ([BP-2]) and dx = -6..6 ([BP-1]) it forms x = sign(dx) + sign(DS:6AAD)
 * ([BP-3]) and y = sign(dy) + sign(DS:6AAE) ([BP-4]), stores the +01AD
 * results for facings 6, 0 and 2 in [BP-8], [BP-7] and [BP-9], and calls
 * +02BA, +0377, +040F and +0615 with its own BP as the static link; it then
 * calls the +0000 grid roll, whose flags become the final flags. All
 * near calls push CS and the return offset; +01AD, +02BA, +0377 and +0000
 * are entered relative to the entry cpu->ip (0x0810 in the game), while
 * the merged +040F and +0615 models are entered at their absolute load
 * offsets 040F and 0615, which they require. Every model runs on the same
 * RAM and CPU words; every operand is reread from RAM; offsets wrap at 16
 * bits, physical addresses at 1 MiB; the RETF frame is read physically
 * contiguous. The dice chain inside +0000 is capped at 256 call slices per
 * roll, which a byte count can never reach. */
pr_u33_0810_status pr_u33_neighbourhood_0810(
    uint8_t *ram, size_t ram_size, pr_u33_0810_cpu *cpu, pr_u33_0810_callee *callee);

#endif
