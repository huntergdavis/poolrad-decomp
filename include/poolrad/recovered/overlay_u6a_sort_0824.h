#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_SORT_0824_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_SORT_0824_H

#include <stddef.h>
#include <stdint.h>

#define PR_U6A_0824_RAM_SIZE (1u << 20)

/* CPU state at the entry to unit 0x6A:+0824, after the caller's far CALL. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6a_0824_cpu;

typedef enum {
    PR_U6A_0824_OK = 0,
    PR_U6A_0824_ARGUMENT
} pr_u6a_0824_status;

/* Executes the complete 155-byte body through RETF. The routine sorts 21
 * bytes at the far pointer DS:[5D92] + 17h by their low seven bits, ascending.
 * A comparison swaps only for a strictly greater left key; whole bytes move.
 * Physical memory wraps at 1 MiB. */
pr_u6a_0824_status pr_u6a_sort_0824(
    uint8_t *ram, size_t ram_size, pr_u6a_0824_cpu *cpu);

#endif
