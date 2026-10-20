#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ENTRY20_1CC3_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ENTRY20_1CC3_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_real_cpu.h"
#define PR_U25_1CC3_CPU_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u25_1cc3_cpu_state;
typedef enum {
    PR_U25_1CC3_CPU_OK = 0,
    PR_U25_1CC3_CPU_ARGUMENT,  /* bad host object or profile violation; nothing changed */
    PR_U25_1CC3_CPU_CODE,      /* a reached LCALL is not 9A <entry> <seg> (stopped at the site, nothing pushed) */
    PR_U25_1CC3_CPU_CALLEE,    /* a SYSTEM Real routine stopped: its RunError exit (AX=00CD/00CF, CS:IP = SYSTEM:00E2)
                                  or a refused argument; diagnostic names the call; effects kept */
    PR_U25_1CC3_CPU_TRANSFER   /* a SYSTEM routine's RETF popped a CS:IP other than the call's next instruction */
} pr_u25_1cc3_cpu_status;
typedef struct {
    int callee_status;         /* last pr_system_real_status, or -1 before any call */
    unsigned calls;            /* SYSTEM routines entered */
    uint16_t call_site;        /* last reached LCALL site; zero before any */
} pr_u25_1cc3_cpu_diagnostic;
/* Complete GAME.OVR U25 [1CC3,1DF0), public entry 20 (far stub 2B:0084), 301 bytes / 81 instructions, RETF 0Ah.
 * Entered after a far CALL: [SP] return IP, [SP+2] return CS, [SP+4]/[SP+6] the far record pointer R (BP+6),
 * [SP+8..0Dh] a 6-byte Real factor F (BP+0Ah). For the seven record words at R+88h, +8Ah, ..., +94h in order:
 * AX := word; DX := 0 (XOR); LCALL SYSTEM 119A (LongInt to Real); CX/SI/DI := F's three words (re-read from
 * the stack); LCALL SYSTEM 1186 (Real multiply); LCALL SYSTEM 119E (Real to LongInt, truncating); LES DI,[BP+6]
 * again; the word := AX. The three SYSTEM routines are the raw models pr_system_real_*, entered through the real
 * far-call frame (the caller's CS and the next IP pushed, the callee's CS read from the LCALL's live segment word),
 * with no callback and no shortcut: their pushes stay in RAM below the stack as the original leaves them.
 * Raw CPU14 over exactly 1 MiB A20-off RAM; entry IP=1CC3; entry SP in [0046,FFF2] (the saved BP, the far call
 * frame and the callee's pushes stay above 0040 and the RETF 0Ah frame does not wrap). Code immutable. Pointer and
 * factor are reread from RAM at every use, so the record may alias the frame. XOR DX,DX leaves CF/OF/SF/AF clear
 * and ZF/PF set (DOSBox-X); the rest of the flags are the callees'. A callee that stops keeps its exact state; no
 * rollback. A null diagnostic is allowed. */
pr_u25_1cc3_cpu_status pr_u25_entry20_1cc3_cpu(uint8_t *ram, size_t ram_size, pr_u25_1cc3_cpu_state *cpu,
                                               pr_u25_1cc3_cpu_diagnostic *diagnostic);
#endif
