#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_SIDECAR_PREFIX_0160_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_SIDECAR_PREFIX_0160_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_0160_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_0160_cpu;

typedef enum {
    PR_U9E_0160_PREFIX_DONE = 0, /* CS:018B, before lcall 00BA:2199 */
    PR_U9E_0160_ARGUMENT
} pr_u9e_0160_status;

/* Execute +0160..+018A on a 1 MiB A20-masked memory image.
 * CPU must enter at CS:0160. On success, all state is left immediately
 * before the resident sidecar lookup instruction at CS:018B. */
pr_u9e_0160_status pr_u9e_sidecar_prefix_0160(
    uint8_t *ram, size_t ram_size, pr_u9e_0160_cpu *cpu);

#endif
