#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_SELECT_101D_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_SELECT_101D_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38_101D_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_101d_cpu;

/* Called at BA:2E8B with return IP/CS, a far output-cell pointer, then
 * a far character pointer on SS:SP. Leave SP on that frame for RETF 8.
 * The merged resident selector supplies its result and output writes. */
typedef int (*pr_u38_101d_select)(void *context, uint8_t *ram,
                                   size_t ram_size, pr_u38_101d_cpu *cpu);

typedef enum {
    PR_U38_101D_OK = 0,
    PR_U38_101D_ARGUMENT,
    PR_U38_101D_CALL_ABORT
} pr_u38_101d_status;

pr_u38_101d_status pr_u38_target_select_101d(
    uint8_t *ram, size_t ram_size, pr_u38_101d_cpu *cpu,
    pr_u38_101d_select select_target, void *context);

#endif
