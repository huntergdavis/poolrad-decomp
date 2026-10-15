#ifndef POOLRAD_RECOVERED_OVERLAY_U38_SIDECAR_FLAG_0918_H
#define POOLRAD_RECOVERED_OVERLAY_U38_SIDECAR_FLAG_0918_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38_0918_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_0918_cpu;

/* Called at +092E with the original five argument words on SS:SP.
 * It supplies the effects of resident BA:2199, whose independent full-C
 * translation is pr_rec_res_00ba_2199. Return zero to abort at the call. */
typedef int (*pr_u38_0918_lookup)(void *context, uint8_t *ram,
                                   size_t ram_size, pr_u38_0918_cpu *cpu);

typedef enum {
    PR_U38_0918_OK = 0,
    PR_U38_0918_ARGUMENT,
    PR_U38_0918_CALL_ABORT
} pr_u38_0918_status;

pr_u38_0918_status pr_u38_sidecar_flag_0918(
    uint8_t *ram, size_t ram_size, pr_u38_0918_cpu *cpu,
    pr_u38_0918_lookup lookup, void *context);

#endif
