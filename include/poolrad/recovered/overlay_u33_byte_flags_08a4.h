#ifndef POOLRAD_RECOVERED_OVERLAY_U33_BYTE_FLAGS_08A4_H
#define POOLRAD_RECOVERED_OVERLAY_U33_BYTE_FLAGS_08A4_H

#include <stddef.h>
#include <stdint.h>

#define PR_U33_08A4_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_08a4_cpu;

typedef enum {
    PR_U33_08A4_OK = 0,
    PR_U33_08A4_ARGUMENT
} pr_u33_08a4_status;

/* State after far CALL to unit 33:+08A4; no arguments. RETF pops the four
 * return-frame bytes with no additional argument cleanup.
 * Reads DS:465A and optionally window 0 (DS:49D2) +0366. Unmatched input
 * retains the actual pre-existing SS:[entry SP-4] byte, then applies the
 * same high-bit/window rule. AL returns the result; AH is preserved.
 * Code must remain immutable. Offsets/SP wrap at 16 bits; multi-byte
 * accesses are contiguous physical bytes with A20 off (1 MiB wrap).
 * Returns actual CS:IP/BP from RAM, including aliased frame writes.
 * AF is unspecified after logic instructions, defined after CMP.
 */
pr_u33_08a4_status pr_u33_byte_flags_08a4(
    uint8_t *ram, size_t ram_size, pr_u33_08a4_cpu *cpu);

#endif
