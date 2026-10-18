#ifndef POOLRAD_RECOVERED_OVERLAY_U38_LEAF_2002_H
#define POOLRAD_RECOVERED_OVERLAY_U38_LEAF_2002_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38_2002_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_2002_cpu;

typedef enum {
    PR_U38_2002_OK = 0,
    PR_U38_2002_ARGUMENT
} pr_u38_2002_status;

/* Execute GAME.OVR U38 +2002..+20DE, with no callbacks or outbound calls.
 * Entry: IP=2002; SS:SP contains return IP, CS, then the inherited-frame
 * offset. That frame has a far cell pointer at +8 and score scratch at
 * -31..-2A. The original leaves the selected-index local uninitialized;
 * if no cell qualifies, AL returns its incoming byte at SS:(entry SP-3).
 *
 * Memory profile: 20-bit A20-wrapped physical RAM; scalar words and LES
 * operands are physically contiguous after 16-bit effective addressing.
 * RETF uses two separately wrapped 16-bit stack pops, as measured in the
 * DOSBox-X normal-386 oracle. CPU exceptions/interrupts and writes to the
 * executing code are outside this profile. LES operands at FFFD..FFFF
 * are checked against Unicorn; native behavior at that seam is unmeasured.
 *
 * Data/stack aliases are read live. The usual eight iterations are not a
 * bound when aliases overwrite the loop index: such original executions
 * can fail to return, and this model imposes no artificial loop limit.
 * CPU storage must not overlap RAM. Invalid arguments are rejected without
 * changing either object. On return, all CPU words and RAM writes survive.
 */
pr_u38_2002_status pr_u38_leaf_2002(
    uint8_t *ram, size_t ram_size, pr_u38_2002_cpu *cpu);

#endif
