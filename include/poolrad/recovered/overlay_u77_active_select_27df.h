#ifndef POOLRAD_RECOVERED_OVERLAY_U77_ACTIVE_SELECT_27DF_H
#define POOLRAD_RECOVERED_OVERLAY_U77_ACTIVE_SELECT_27DF_H

#include <stddef.h>
#include <stdint.h>

#define PR_U77_27DF_RAM_SIZE (1u << 20)

/* State at the entry to unit 0x77:+27DF, after the far CALL. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u77_27df_cpu;

typedef enum {
    PR_U77_27DF_OK = 0,
    PR_U77_27DF_ARGUMENT
} pr_u77_27df_status;

/* Executes the entire 223-byte body through RETF 2. The byte at entry
 * SS:[SP+4] becomes SS:[BP+6] after PUSH BP. Cyclic next-pointer chains
 * may loop as in the DOS
 * source. Effective offsets and SP wrap at 16 bits; multi-byte memory
 * operands and RETF read contiguous physical bytes, wrapping at 1 MiB
 * with A20 disabled. The original code must remain immutable. Returned
 * CS:IP reflects actual RAM, including return frames aliased by writes.
 * AF is unspecified after OR; all other tracked defined flags match. */
pr_u77_27df_status pr_u77_active_select_27df(
    uint8_t *ram, size_t ram_size, pr_u77_27df_cpu *cpu);

#endif
