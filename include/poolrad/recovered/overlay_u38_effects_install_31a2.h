#ifndef POOLRAD_RECOVERED_OVERLAY_U38_EFFECTS_INSTALL_31A2_H
#define POOLRAD_RECOVERED_OVERLAY_U38_EFFECTS_INSTALL_31A2_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38_31A2_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38_31a2_cpu;

typedef enum {
    PR_U38_31A2_OK = 0,
    PR_U38_31A2_ARGUMENT
} pr_u38_31a2_status;

/* Complete +31A2..+38A8 after far CALL; no arguments, plain RETF.
 * Installs 138 ordered far pointers in DS:682C..6A57. Table stores skip
 * DS:6988..698B; stack aliases remain observable. FLAGS, including AF,
 * are preserved exactly.
 * Loaded segment words are inputs at CS:(31A9 + 13*i), i=0..137;
 * no other original instruction bytes are needed by the C model.
 * Code/relocation words must stay immutable. Offsets/SP wrap at 16 bits;
 * multi-byte operands use contiguous physical bytes with A20 off.
 * Live RAM determines restored BP and actual far-return state.
 */
pr_u38_31a2_status pr_u38_effects_install_31a2(
    uint8_t *ram, size_t ram_size, pr_u38_31a2_cpu *cpu);

#endif
