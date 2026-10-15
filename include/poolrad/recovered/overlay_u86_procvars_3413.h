#ifndef POOLRAD_RECOVERED_OVERLAY_U86_PROCVARS_3413_H
#define POOLRAD_RECOVERED_OVERLAY_U86_PROCVARS_3413_H

#include <stddef.h>
#include <stdint.h>

#define PR_U86_3413_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u86_3413_cpu;

typedef enum {
    PR_U86_3413_OK = 0,
    PR_U86_3413_ARGUMENT
} pr_u86_3413_status;

/* Complete unit 86:+3413..+37AF, entered after far CALL; no arguments.
 * The 69 loaded MOV DX relocation words are inputs at CS:341A,
 * CS:(343E + 13*i), i=0..66, and CS:37A5. The C needs these synthetic
 * words, not the remaining instruction bytes. Actual DOS loader fixups
 * relocate each original 0096 segment by the executable load paragraph.
 * All source code/relocation words must remain immutable during the call.
 * Offsets/SP wrap at 16 bits; multi-byte accesses are physically contiguous
 * with A20 off (1 MiB wrap). Live RAM determines restored BP and CS:IP.
 * Undefined XOR AF is normalized to zero; other defined flags match.
 */
pr_u86_3413_status pr_u86_procvars_3413(
    uint8_t *ram, size_t ram_size, pr_u86_3413_cpu *cpu);

#endif
