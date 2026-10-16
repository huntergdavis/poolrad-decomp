#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_LOOP_0E05_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_LOOP_0E05_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_ENTRY8_LOOP_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_entry8_loop_cpu;

typedef enum {
    PR_U9E_ENTRY8_LOOP_AT_CALL = 0,
    PR_U9E_ENTRY8_LOOP_ARGUMENT
} pr_u9e_entry8_loop_status;

/* Enter at original U9E +0E05 after the post-SYSTEM JNE. Stop before
 * +0E0E LCALL. CPU14 and 1 MiB RAM are live, including SS/DS aliases.
 * Loaded CS:+0DE3..+0E2D code must be the pinned original body. */
pr_u9e_entry8_loop_status pr_u9e_entry8_fullstate_loop_0e05(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_loop_cpu *cpu);

#endif
