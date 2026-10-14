#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_PARTY_COUNT_05E0_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_PARTY_COUNT_05E0_H

#include <stddef.h>
#include <stdint.h>

#define PR_U7F_05E0_RAM_SIZE (1u << 20)

/* State after the far CALL to unit 0x7F:+05E0. The far return is in SS RAM. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7f_05e0_cpu;

typedef enum {
    PR_U7F_05E0_OK = 0,
    PR_U7F_05E0_ARGUMENT
} pr_u7f_05e0_status;

/* Executes the complete call-free body through RETF. A cyclic party list
 * loops as in the source. Physical addresses wrap at 1 MiB. */
pr_u7f_05e0_status pr_u7f_party_count_05e0(
    uint8_t *ram, size_t ram_size, pr_u7f_05e0_cpu *cpu);

#endif
