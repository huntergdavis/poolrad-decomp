#ifndef POOLRAD_RECOVERED_RESIDENT_0464_ALLOC_248E_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0464_ALLOC_248E_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"

/* Raw-CPU, full-state model of START.EXE resident 0464:[248E,2595) (263 bytes,
 * 95 instructions, RETF 0Ch): the bitmap allocator paired with the release at
 * 0464:2595. Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] the far
 * address of a pointer variable P, [SP+8] a flag byte (bit 0 asks for the
 * plane buffer), [SP+0Ah] a byte multiplier, [SP+0Ch] and [SP+0Eh] the two
 * size words stored as the header's +0 and +2. It GetMems a 17h-byte-headed
 * block of the computed size, clears it with FillChar, fills the header, and,
 * when bit 0 is set, GetMems and clears a second buffer stored at header +13h
 * (a quarter of the size in video mode 1). SYSTEM GetMem 0AF8:023F and
 * FillChar 0AF8:16A8 run as their accepted raw models.
 * See docs/re/RES_0464_ALLOC_248E_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r0464_alloc_cpu;

typedef enum {
    PR_R0464_ALLOC_CPU_OK = 0,
    PR_R0464_ALLOC_CPU_ARGUMENT,  /* host arguments or entry state rejected; nothing changed */
    PR_R0464_ALLOC_CPU_CALLEE,    /* GetMem or FillChar did not return normally; diagnostic names which and its status */
    PR_R0464_ALLOC_CPU_PORT,      /* unused by this body (no OUT); kept for the shared status numbering */
    PR_R0464_ALLOC_CPU_TRANSFER,  /* a callee returned somewhere other than the call's next IP */
    PR_R0464_ALLOC_CPU_LIMIT      /* max_loopbacks backward jumps taken (this body has none; always 0 taken) */
} pr_r0464_alloc_cpu_status;

typedef struct {
    int getmem_calls;   /* GetMem calls entered */
    int getmem_status;  /* last pr_system_getmem_status, -1 when none */
    int fill_calls;     /* FillChar calls entered */
    int fill_status;    /* last FillChar leaf status, -1 when none */
} pr_r0464_alloc_cpu_diagnostic;

/* getmem_steps caps GetMem's free-list walk (its LIMIT stop). */
pr_r0464_alloc_cpu_status pr_r0464_alloc_248e_cpu(pr_bus *bus, size_t ram_size, pr_r0464_alloc_cpu *cpu,
                                                   size_t max_loopbacks, unsigned getmem_steps,
                                                   pr_r0464_alloc_cpu_diagnostic *diagnostic);
#endif
