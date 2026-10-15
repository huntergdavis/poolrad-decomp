#ifndef POOLRAD_RECOVERED_RESIDENT_464_2632_RUN014_H
#define POOLRAD_RECOVERED_RESIDENT_464_2632_RUN014_H

#include <stddef.h>
#include <stdint.h>

enum { PR_R464_2632_RAM_BYTES = 0x100000u };

/* Real-mode CPU register values at the loaded 0C88:2632 entry. */
typedef struct {
    uint16_t cs, ip, ax, bx, cx, dx, si, di;
    uint16_t bp, sp, ds, es, ss, flags;
} pr_r464_2632_cpu;

typedef enum {
    PR_R464_2632_OK = 0,
    PR_R464_2632_INVALID_HOST = 1,
    PR_R464_2632_UNSUPPORTED_PATH = 2
} pr_r464_2632_status;

/* The observer runs at native instruction boundaries, before that IP's
 * instruction. It must treat RAM and CPU as read-only. */
typedef void (*pr_r464_2632_observer)(uint16_t ip, const uint8_t *ram,
                                     const pr_r464_2632_cpu *cpu, void *context);

/* Bounded composition of the measured run014 positive-X, nonnegative-row,
 * video-state-1, one-row path through its RETF 12 to overlay +01C3.
 * Unsupported preflight inputs have no effects. This is not a full routine. */
pr_r464_2632_status pr_r464_2632_run014(
    uint8_t *ram, size_t ram_size, pr_r464_2632_cpu *cpu,
    pr_r464_2632_observer observer, void *context);

#endif
