#ifndef POOLRAD_RECOVERED_SYSTEM_SEEK_1563_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_SEEK_1563_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_SEEK_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of Turbo Pascal SYSTEM Seek(var f: file; n: longint), START.EXE 0AF8:[1563,1593) (48 bytes,
 * 19 instructions, RETF 8) with the shared open-check helper 0AF8:[14B6,14C5) (15 bytes, 4 instructions). Readiness coverage
 * only, no identity claim (docs/re/SYSTEM_SEEK_1563_CPU.md).
 *
 * Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] n (low word), [SP+6] n (high word), [SP+8] the far address of f.
 * PUSH BP; MOV BP,SP; LES DI,f; CALL 14B6: unless the magic f+2 is D7B3h (open), InOutRes DS:[4426] := 103 and return.
 * The byte position is n * recsize (recsize = f+4) truncated to 32 bits, formed by two 16-bit MULs: CX := low word of
 * (n_high * recsize) (its high word is DROPPED), then DX:AX := n_low * recsize and CX := CX + DX (ADD, no carry beyond);
 * DX := AX. INT 21h AX=4200h (LSEEK from the start), BX = the handle f+0, CX:DX the position. Carry: InOutRes := AX.
 * POP BP; RETF 8.
 *
 * Declared environment input: INT 21h AH=42h. The service receives the CPU at the INT (IP 1588h) and returns the complete
 * CPU after DOS returns at 158Ah (same CS, SS, SP, BP); only AH 42h is listed (ENV otherwise); it must not write RAM (DOS's
 * frame below SP is outside the model). The MULs' architecturally undefined SF/ZF/AF/PF are overwritten by the ADD before
 * anything reads them. Exactly 1 MiB A20-off RAM; IP = 1563h; entry SP in [0006h,FFF4h] (pushes reach entry SP - 4,
 * arguments entry SP + 0Bh). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_seek_cpu;

typedef enum {
    PR_SEEK_RETURNED = 0,  /* RETF 8 executed */
    PR_SEEK_ARGUMENT,      /* null RAM/CPU/service, RAM not exactly 1 MiB, IP not 1563h, SP outside [0006h,FFF4h]; nothing changed */
    PR_SEEK_SERVICE,       /* the INT 21h service declined or returned a non-continuation; stopped at the INT (IP 1588h) */
    PR_SEEK_ENV            /* an INT 21h function other than 42h would be issued (structural guard) */
} pr_seek_status;

typedef int (*pr_seek_dos_service)(void *context, const pr_seek_cpu *before, pr_seek_cpu *after);

pr_seek_status pr_system_seek_1563(uint8_t *ram, size_t ram_size, pr_seek_cpu *cpu, pr_seek_dos_service service, void *context);
#endif
