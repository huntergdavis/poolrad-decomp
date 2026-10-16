#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_IN_GRID_031F_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_IN_GRID_031F_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3BD_031F_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3bd_031f_cpu;

typedef enum {
    PR_U3BD_031F_OK = 0,
    PR_U3BD_031F_ARGUMENT
} pr_u3bd_031f_status;

/* Complete U3BD:+031F..+034F from a CALL frame. Both argument low bytes
 * must be signed 0..15 to return AL=1; otherwise AL=0. AH and unrelated
 * registers persist. The last executed CMP defines arithmetic FLAGS.
 * PUSH BP and the byte local remain in RAM; RETF 4 restores BP and consumes
 * the far return address and two argument words. Segment offsets wrap at
 * 16 bits; each word access is physically contiguous with A20 disabled.
 * Code bytes must remain immutable during the call.
 */
pr_u3bd_031f_status pr_u3bd_in_grid_031f(
    uint8_t *ram, size_t ram_size, pr_u3bd_031f_cpu *cpu);

#endif
