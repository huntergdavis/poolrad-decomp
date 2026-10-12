#ifndef POOLRAD_RECOVERED_OVERLAY_U9B_ENTRY_ZERO_0000_H
#define POOLRAD_RECOVERED_OVERLAY_U9B_ENTRY_ZERO_0000_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9B_0000_RAM_SIZE (1u << 20)

/* State at entry to unit 0x9B:+0x0000, after the far caller's CALL.
 * Return IP/CS and far record argument already occupy SS RAM. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9b_0000_cpu;

typedef enum {
    PR_U9B_0000_OK = 0,
    PR_U9B_0000_ARGUMENT,
    PR_U9B_0000_TRANSFER
} pr_u9b_0000_status;

/* Source-ordered 1 MiB A20-off RAM translation of +0000..+01AE and its
 * three internal calls. TRANSFER exposes the exact state when a corrupted
 * near-call return frame leaves the expected enclosing continuation. */
pr_u9b_0000_status pr_u9b_entry_zero_0000(
    uint8_t *ram, size_t ram_size, pr_u9b_0000_cpu *cpu);

#endif
