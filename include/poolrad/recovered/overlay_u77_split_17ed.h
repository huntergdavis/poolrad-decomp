#ifndef POOLRAD_RECOVERED_OVERLAY_U77_SPLIT_17ED_H
#define POOLRAD_RECOVERED_OVERLAY_U77_SPLIT_17ED_H

#include <stddef.h>
#include <stdint.h>

#define PR_U77_17ED_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u77_17ed_cpu;

typedef enum {
    PR_U77_17ED_ALLOC = 0,  /* AF8:023F; stack [SP] = size 0x3F */
    PR_U77_17ED_COPY,       /* AF8:1685; size, destination, source */
    PR_U77_17ED_STRING,     /* AF8:B0B; destination, CS:17DC */
    PR_U77_17ED_REPORT      /* BA:1693; no explicit arguments */
} pr_u77_17ed_call;

/* Invoked with original arguments still on the emulated stack. ALLOC returns
 * its far pointer in DX:AX. The adapter owns heap, copy, and message effects.
 * Return 1 to continue; the model performs each callee's stack cleanup. */
typedef int (*pr_u77_17ed_external)(void *context, uint8_t *ram,
                                     size_t ram_size, pr_u77_17ed_cpu *cpu,
                                     pr_u77_17ed_call call);

typedef enum {
    PR_U77_17ED_OK = 0,
    PR_U77_17ED_ARGUMENT,
    PR_U77_17ED_CALL_ABORT
} pr_u77_17ed_status;

/* Unit 0x77:+17ED body through RETF 4, including full RAM and CPU state
 * under caller-provided external-call effects. */
pr_u77_17ed_status pr_u77_split_17ed(
    uint8_t *ram, size_t ram_size, pr_u77_17ed_cpu *cpu,
    pr_u77_17ed_external external, void *context);

#endif
