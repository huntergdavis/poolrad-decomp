#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY0_1932_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY0_1932_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_1932_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_1932_cpu;

typedef enum {
    PR_U9E_1932_OK = 0,
    PR_U9E_1932_ARGUMENT
} pr_u9e_1932_status;

/* Execute GAME.OVR U9E public entry 0, +1932..+1936, from a far frame. */
pr_u9e_1932_status pr_u9e_entry0_1932(
    uint8_t *ram, size_t ram_size, pr_u9e_1932_cpu *cpu);

#endif
