#ifndef POOLRAD_RECOVERED_OVERLAY_U33_PAIR_WRITER_0B97_H
#define POOLRAD_RECOVERED_OVERLAY_U33_PAIR_WRITER_0B97_H
#include <stddef.h>
#include <stdint.h>
#define PR_U33_0B97_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u33_0b97_cpu;
typedef enum { PR_U33_0B97_OK = 0, PR_U33_0B97_ARGUMENT } pr_u33_0b97_status;
/* Complete +0B97..+0BDD, entered after CALL. Word args at SS:SP+4
 * (caller frame offset), +6 (signed column byte), +8 (signed row byte).
 * RETF 6. Writes 34h/35h through live DS:6716/6718 at row*50+column+7,
 * then increments SS:[frame-2]. Preserves ordered aliases and all returned
 * CPU words under the immutable-code, A20-off 1 MiB RAM contract. */
pr_u33_0b97_status pr_u33_pair_writer_0b97(
    uint8_t *ram, size_t ram_size, pr_u33_0b97_cpu *cpu);
#endif
