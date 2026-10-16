#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_POSTCALL_0E13_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_FULLSTATE_POSTCALL_0E13_H

#include <stddef.h>
#include <stdint.h>

#define PR_U9E_ENTRY8_POSTCALL_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u9e_entry8_postcall_cpu;

typedef enum {
    PR_U9E_ENTRY8_POSTCALL_AT_LOOP = 0,
    PR_U9E_ENTRY8_POSTCALL_AT_TAIL,
    PR_U9E_ENTRY8_POSTCALL_ARGUMENT
} pr_u9e_entry8_postcall_status;

/* Resume at original U9E entry8 +0E13 with supplied post-SYSTEM RAM/CPU14.
 * Stop at loop successor +0E05 or shared tail +0E1F. No SYSTEM call,
 * tail, or far return is executed. Loaded CS:+0DE3..+0E2D code must be
 * the pinned original body. */
pr_u9e_entry8_postcall_status pr_u9e_entry8_fullstate_postcall_0e13(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_postcall_cpu *cpu);

#endif
