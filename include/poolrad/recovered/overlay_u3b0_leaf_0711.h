#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_LEAF_0711_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_LEAF_0711_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3B0_0711_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3b0_0711_cpu;

typedef enum {
    PR_U3B0_0711_OK = 0,
    PR_U3B0_0711_ARGUMENT
} pr_u3b0_0711_status;

/* Execute GAME.OVR U3B0 +0711..+07A3 without hooks or outbound calls.
 * Entry IP=0711. SS:SP holds return IP/CS, an unused word, the far head
 * pointer (offset/segment), and a signed 16-bit traversal bound. RETF 8
 * consumes the frame; DX:AX returns the last flagged node, or the original
 * head if its +29 byte is nonzero, else zero. Even a null head is read at
 * 0000:0029 on the fallback path. Node links are separate word loads at
 * +2A/+2C; stack LES operands consume four physically contiguous bytes.
 *
 * Profile: full 1 MiB A20-wrapped RAM, contiguous scalar words after 16-bit
 * effective addressing, and separate wrapped RETF pops measured with
 * DOSBox-X normal386. LES seams at FFFD..FFFF have Unicorn evidence only;
 * hardware exceptions, interrupts, and executing-code aliases are outside
 * this profile. Undefined logical AF is cleared, matching the measured
 * oracles. CPU storage must not overlap RAM.
 *
 * All data/stack aliases are read live. The original initializes its local
 * count to zero and compares it with the signed bound each iteration; a
 * positive bound limits traversal even for cyclic lists. All CPU words and
 * RAM effects survive. Invalid arguments leave both objects unchanged.
 */
pr_u3b0_0711_status pr_u3b0_leaf_0711(
    uint8_t *ram, size_t ram_size, pr_u3b0_0711_cpu *cpu);

#endif
