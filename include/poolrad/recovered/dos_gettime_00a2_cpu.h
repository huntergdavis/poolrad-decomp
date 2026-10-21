#ifndef POOLRAD_RECOVERED_DOS_GETTIME_00A2_CPU_H
#define POOLRAD_RECOVERED_DOS_GETTIME_00A2_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_DOS_GETTIME_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of the Turbo Pascal Dos unit's GetTime at START.EXE
 * 0A32:[00A2,00C7) (37 bytes, 19 instructions, RETF 10h), readiness coverage
 * only (no identity claim). Entered after a far call: [SP] IP, [SP+2] CS, then
 * the far addresses of Sec100 [SP+4], Second [SP+8], Minute [SP+0Ch] and
 * Hour [SP+10h]. PUSH BP; MOV BP,SP; MOV AH,2Ch; INT 21h; XOR AH,AH; then
 * AL := DL, DH, CL, CH in turn, each stored by STOSW through LES DI of the
 * matching argument; POP BP; RETF 10h.
 *
 * INT 21h is a DECLARED environment input: the host service receives the CPU
 * at the INT (IP 00A7, AH 2Ch) and returns the complete CPU after the DOS call
 * returns at 00A9. The model refuses (SERVICE, nothing after the INT applied) a
 * result whose CS, IP, SS, SP or BP is not that continuation. The service must
 * not write RAM: DOS's own interrupt frame and stack use below SP are outside
 * this model (stated limit). Every other operand is reread from RAM in source
 * order; the frame never crosses SS:FFFF (ARGUMENT guard), so LES pairs never wrap; the two
 * bytes of any word are linear-contiguous with 1 MiB wrap; STOSW honours DF. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_dos_gettime_cpu;

typedef enum {
    PR_DOS_GETTIME_RETURNED = 0,  /* RETF 10h executed */
    PR_DOS_GETTIME_ARGUMENT,      /* null RAM/CPU/service, RAM not exactly 1 MiB, IP not 00A2h, or SP outside
                                     [0002h,FFECh] (the pushed BP and the 14h-byte frame must not cross SS:FFFF); nothing changed */
    PR_DOS_GETTIME_SERVICE        /* the service refused or returned a non-continuation; stopped at the INT (IP 00A7) */
} pr_dos_gettime_status;

/* Returns 0 with *after filled, or nonzero to refuse. */
typedef int (*pr_dos_gettime_service)(void *context, const pr_dos_gettime_cpu *before, pr_dos_gettime_cpu *after);

pr_dos_gettime_status pr_dos_gettime_00a2(uint8_t *ram, size_t ram_size, pr_dos_gettime_cpu *cpu,
                                          pr_dos_gettime_service service, void *context);
#endif
