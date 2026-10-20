#ifndef POOLRAD_RECOVERED_RESIDENT_0464_BLIT_1951_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0464_BLIT_1951_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"

/* Raw-CPU, full-state model of START.EXE resident 0464:[1951,1B52) (513 bytes,
 * 201 instructions, RETF 8): a bitmap blit selected by the video-mode byte
 * DS:4A1E (0: row table DS:4EC0 and SYSTEM Move; 1: EGA sequencer map-mask
 * OUTs 3C4/3C5 with A000 stores through row table DS:5050 and plane masks
 * DS:25F6; 2: row table DS:51E0 and SYSTEM Move; other values return).
 * Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] far bitmap pointer
 * (null returns at once), [SP+8] row argument, [SP+0Ah] column argument.
 * See docs/re/RES_0464_BLIT_1951_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0464_blit_cpu;

typedef enum {
    PR_R0464_BLIT_CPU_OK = 0,
    PR_R0464_BLIT_CPU_ARGUMENT,  /* host arguments or entry state rejected; nothing changed */
    PR_R0464_BLIT_CPU_CALLEE,    /* SYSTEM Move returned a non-RETURNED status; state as it left it */
    PR_R0464_BLIT_CPU_PORT,      /* the bus refused an OUT; stopped before it, IP at the OUT */
    PR_R0464_BLIT_CPU_TRANSFER,  /* Move returned somewhere other than the call's next IP */
    PR_R0464_BLIT_CPU_LIMIT,     /* max_loopbacks backward jumps taken; stopped before the next */
    PR_R0464_BLIT_CPU_CONTRACT   /* a Move source or destination meets A0000..AFFFF with an EGA attached; stopped at Move entry */
} pr_r0464_blit_cpu_status;

typedef struct {
    int move_calls;   /* completed or attempted SYSTEM Move calls */
    int move_status;  /* last Move leaf status, -1 when none */
} pr_r0464_blit_cpu_diagnostic;

pr_r0464_blit_cpu_status pr_r0464_blit_1951_cpu(pr_bus *bus, size_t ram_size, pr_r0464_blit_cpu *cpu,
                                                 size_t max_loopbacks, pr_r0464_blit_cpu_diagnostic *diagnostic);
#endif
