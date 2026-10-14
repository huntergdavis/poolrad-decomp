#ifndef POOLRAD_RECOVERED_OVERLAY_U25_PARTY_INDEX_0756_H
#define POOLRAD_RECOVERED_OVERLAY_U25_PARTY_INDEX_0756_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_0756_RAM_SIZE (1u << 20)

/* State at entry to unit 0x25:+0x0756, after its far CALL. The return
 * address and far pointer argument are already in SS RAM. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_0756_cpu;

typedef enum {
    PR_U25_0756_OK = 0,
    PR_U25_0756_ARGUMENT
} pr_u25_0756_status;

/* Executes the complete 102-byte call-free body through RETF 4. The normal
 * game party list is finite. Like the source, a cyclic list does not return.
 * The 1 MiB RAM uses 8086 physical-address wrapping. */
pr_u25_0756_status pr_u25_party_index_0756(
    uint8_t *ram, size_t ram_size, pr_u25_0756_cpu *cpu);

#endif
