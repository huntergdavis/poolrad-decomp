#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_SIDECAR_REMOVE_1509_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_SIDECAR_REMOVE_1509_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_0709_msgbox_0bf5_cpu.h"

typedef pr_r0709_0bf5_cpu pr_res_1509_cpu;
typedef enum {
    PR_RES_1509_RETURNED = 0,
    PR_RES_1509_ARGUMENT,   /* null pointers, RAM size, IP not 1509h, entry SP outside [0200h,FFF2h]; nothing changed */
    PR_RES_1509_CODE,       /* a far call site is not the original opcode or offset; IP at the call, before its return frame */
    PR_RES_1509_LIMIT,      /* the host budget ran out at the next unexecuted instruction of this body (a cyclic list never ends, as in DOS), or FreeMem's cap */
    PR_RES_1509_TRANSFER,   /* a return went elsewhere than expected (this body's RETF 8, or a callee) */
    PR_RES_1509_CALLEE,     /* StrLoad or the prompt stopped otherwise; diagnostic.callee and callee_status say which and how */
    PR_RES_1509_KEY,        /* the prompt's ReadKey did not return (EXIT, BREAK, ...): diagnostic.d0bf5.key_status */
    PR_RES_1509_RUNERROR    /* SYSTEM FreeMem raised RunError 204 (an invalid or duplicate block): CPU at SYSTEM:00E2, AX = 204, as in DOS */
} pr_res_1509_status;

typedef struct { pr_r0709_0bf5_env prompt; unsigned freemem_steps; } pr_res_1509_env;
typedef struct {
    int callee;               /* -1 none yet; 1 StrLoad, 2 the prompt 0709:0BF5, 3 FreeMem */
    int callee_status;
    uint16_t site;
    pr_r0709_0bf5_diagnostic d0bf5;
} pr_res_1509_diagnostic;

/* START.EXE resident segment 00BA [1509,15F7): remove a 3Fh-byte record from the character's +0C8h list, 238 bytes / 83 instructions. Enter after
 * the far call at IP 1509: SS:[SP] return IP, [SP+2] return CS, [SP+4]/[SP+6] the record far pointer (offset, segment), [SP+8]/[SP+10] the
 * character far pointer; RETF 8. A null record (OR of its words) loads the Pascal message at CS:14CE into [BP-18h] (StrLoad) and shows it through the
 * resident prompt 0709:0BF5 with colour 0Eh, then returns. Otherwise: if the character's head +0C8h/+0CAh is the record, the head := the record's link
 * +2Ah/+2Ch; else the walk from the head (through +2Ah) looks for the node whose link is the record. Not found (the walk reaches null): the message at
 * CS:14E2 into [BP-2Bh], the prompt, return. Found: the predecessor's link := the record's link. Then the record's link := 0:0 and SYSTEM FreeMem
 * frees the record (3Fh bytes). A cyclic list never ends, as in DOS. Every access through a far pointer the body did not create (the record, the
 * character, the list nodes) goes THROUGH THE BUS; the stack, the code and DS globals are flat; StrLoad and FreeMem keep their flat contracts and the
 * prompt its own (bus, EGA, ReadKey environment). Declared exclusion: FreeMem reaches the freed record FLAT, so a record in the A0000..AFFFF
 * aperture is freed through RAM, not the planes. The flags follow the pinned DOSBox-X stored rules. Each instruction of this body consumes one unit
 * of *budget; StrLoad, the prompt and FreeMem do not (the prompt and FreeMem have their own caps in *env). Exactly 1 MiB of A20-off RAM; entry SP
 * in [0200h,FFF2h]. CPU, budget, env, bus and diagnostic storage are distinct. Code is immutable. */
pr_res_1509_status pr_res_00ba_sidecar_remove_1509_cpu(pr_bus *bus, size_t ram_size, pr_res_1509_cpu *cpu, size_t *budget, const pr_res_1509_env *env,
                                                     pr_res_1509_diagnostic *diagnostic);
#endif
