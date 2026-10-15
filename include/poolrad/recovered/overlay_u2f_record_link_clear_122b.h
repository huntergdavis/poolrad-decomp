#ifndef POOLRAD_RECOVERED_OVERLAY_U2F_RECORD_LINK_CLEAR_122B_H
#define POOLRAD_RECOVERED_OVERLAY_U2F_RECORD_LINK_CLEAR_122B_H

#include <stddef.h>
#include <stdint.h>

#define PR_U2F_122B_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u2f_122b_cpu;

typedef enum {
    PR_U2F_122B_OK = 0,
    PR_U2F_122B_ARGUMENT
} pr_u2f_122b_status;

/* Complete +122B..+1279 after CALL, far record argument at SS:SP+4.
 * Sets record+10F to 1; follows record+108 and its +0A far pointer.
 * Clears that pointer if nonnull and both records' +10E bytes match.
 * Ordered reloads, frame aliases and defined CPU/FLAGS state are retained.
 * AF is undefined on OR/XOR exits; C chooses zero there. RETF 4.
 * Effective offsets/SP wrap at 16 bits; multi-byte operands are contiguous
 * physical bytes with A20 off. Code must remain immutable.
 */
pr_u2f_122b_status pr_u2f_record_link_clear_122b(
    uint8_t *ram, size_t ram_size, pr_u2f_122b_cpu *cpu);

#endif
