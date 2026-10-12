#ifndef POOLRAD_RECOVERED_OVERLAY_U5E_PARTY_VALUE_2879_H
#define POOLRAD_RECOVERED_OVERLAY_U5E_PARTY_VALUE_2879_H

#include <stddef.h>
#include <stdint.h>

#define PR_U5E_2879_RAM_SIZE (1u << 20)

/* State at unit 0x5E:+0x2879 after the caller's far CALL. Return IP/CS
 * already occupy SS:SP. All memory, including SS locals, shares A20-off RAM. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u5e_2879_cpu;

typedef enum {
    PR_U5E_2879_OK = 0,
    PR_U5E_2879_ARGUMENT,
    PR_U5E_2879_LIMIT
} pr_u5e_2879_status;

/* Translate all terminating paths of +2879..+2907. The host's positive
 * max_nodes stops a cyclic or overlong traversal at +289E; the DOS routine
 * itself has no such limit. On LIMIT, RAM/CPU show effects already reached. */
pr_u5e_2879_status pr_u5e_party_value_2879(
    uint8_t *ram, size_t ram_size, pr_u5e_2879_cpu *cpu, size_t max_nodes);

#endif
