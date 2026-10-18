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

/* RETF policy is explicit because the historical Unicorn profile and
 * DOSBox-X normal/386 differ at SS:FFFE/FFFF. Memory word/LES operands
 * retain their existing physically contiguous semantics in both profiles. */
typedef enum {
    PR_U77_27DF_RETF_WRAPPED_SS = 1,
    PR_U77_27DF_RETF_CONTIGUOUS_PHYSICAL = 2
} pr_u77_27df_retf_policy;

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

/* Same invocation with a caller-selected return profile. WRAPPED_SS reads
 * CS at SS:(uint16_t)(SP+2), measured on the complete original body with
 * DOSBox-X normal/386. The API above remains CONTIGUOUS_PHYSICAL for
 * compatibility. Invalid policy returns ARGUMENT without changing state.
 * AF remains unspecified after OR in either profile. CPU storage must not
 * overlap RAM; code is immutable, execution synchronous, and hardware
 * faults/devices/interrupts are outside this flat-RAM contract.
 * A return into this body is reported as the live CS:IP, not resumed. */
pr_u77_27df_status pr_u77_active_select_27df_run(
    uint8_t *ram, size_t ram_size, pr_u77_27df_cpu *cpu,
    pr_u77_27df_retf_policy retf_policy);

#endif
