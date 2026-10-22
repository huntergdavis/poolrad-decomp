#ifndef POOLRAD_RECOVERED_RESIDENT_0464_COMPOSE_08D4_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0464_COMPOSE_08D4_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"

/* Raw-CPU, full-state model of START.EXE resident 0464:[08D4,0D71) (1,181 bytes,
 * 429 instructions, RETF 10h): the tile compositor. It places tile [BP+10h] of the
 * source tile set [BP+0Ah] at (x [BP+14h], y [BP+12h]) in the off-screen
 * destination object [BP+6], by the video-state byte DS:4A1E (0 and 2: row copies
 * through SYSTEM Move 0AF8:1685, the auxiliary-present loop when flag [BP+0Eh] bit 0
 * is set and the tile's +13h/+15h pointer is non-null; other values take the
 * plane-byte path), and records the dirty rows DS:4A28[] and the row extents
 * DS:4AD0[], 4C20[] and 4D70[], with byte counters at DS:70C7.
 * Entered after a far call: [SP] IP, [SP+2] CS, then the six arguments; RETF 10h.
 * See docs/re/RES_0464_COMPOSE_08D4_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0464_compose_cpu;

typedef enum {
    PR_R0464_COMPOSE_CPU_OK = 0,
    PR_R0464_COMPOSE_CPU_ARGUMENT,  /* host arguments or entry state rejected; nothing changed */
    PR_R0464_COMPOSE_CPU_CALLEE,    /* SYSTEM Move returned a non-RETURNED status; state as it left it */
    PR_R0464_COMPOSE_CPU_PORT,      /* never produced: the body has no OUT (kept for the shared emitter) */
    PR_R0464_COMPOSE_CPU_TRANSFER,  /* Move returned somewhere other than the call's next IP */
    PR_R0464_COMPOSE_CPU_LIMIT,     /* max_loopbacks backward jumps taken; stopped before the next */
    PR_R0464_COMPOSE_CPU_CONTRACT   /* a Move source or destination meets A0000..AFFFF with an EGA attached; stopped at Move entry */
} pr_r0464_compose_cpu_status;

typedef struct {
    int move_calls;   /* completed or attempted SYSTEM Move calls */
    int move_status;  /* last Move leaf status, -1 when none */
} pr_r0464_compose_cpu_diagnostic;

pr_r0464_compose_cpu_status pr_r0464_compose_08d4_cpu(pr_bus *bus, size_t ram_size, pr_r0464_compose_cpu *cpu,
                                                       size_t max_loopbacks, pr_r0464_compose_cpu_diagnostic *diagnostic);
#endif
