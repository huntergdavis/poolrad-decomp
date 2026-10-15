#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_VECTOR_MERGE_032F_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_VECTOR_MERGE_032F_H

#include <stddef.h>
#include <stdint.h>

#define PR_U6F_032F_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6f_032f_cpu;

typedef enum {
    PR_U6F_032F_OK = 0,
    PR_U6F_032F_ARGUMENT
} pr_u6f_032f_status;

/* Complete unit 6F:+032F..+0475 through RETF 8. Entry SS:SP contains
 * return IP/CS, destination far pointer, then source far pointer.
 * RETF pops IP and CS at separate 16-bit stack offsets; individual words
 * and memory far-pointer operands remain physically contiguous.
 * The mode at DS:4A1E controls inline OR / pointed-vector AND operations.
 * Reads and writes retain source order, live pointers, and 20-bit wrapping.
 * A zero source count underflows to 65536 iterations, matching the original.
 * Aliased loop-control storage can retain source nontermination. Code must
 * remain immutable; RAM is a flat A20-disabled model, without video devices.
 * Ownership and reached game identity remain separate review gates. */
pr_u6f_032f_status pr_u6f_vector_merge_032f(
    uint8_t *ram, size_t ram_size, pr_u6f_032f_cpu *cpu);

#endif
