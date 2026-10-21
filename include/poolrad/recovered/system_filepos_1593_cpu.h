#ifndef POOLRAD_RECOVERED_SYSTEM_FILEPOS_1593_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_FILEPOS_1593_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_FILEPOS_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of Turbo Pascal SYSTEM FilePos(var f: file): longint, START.EXE 0AF8:[1593,15AA) (23 bytes,
 * 9 instructions, RETF 4) with its position helper 0AF8:[15DA,161F) (69 bytes, 32 instructions, near RET; shared with
 * FileSize 15AA and Eof 15C5, which are separate endpoints). The record division composes the covered LongDiv model
 * (system_longdiv_0a76_cpu). Runtime coverage only, no identity claim (docs/re/SYSTEM_FILEPOS_1593_CPU.md).
 *
 * Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] the far address of f. CALL 15DA: MOV BX,SP; LES DI,SS:[BX+6].
 * Not open (magic f+2 != D7B3h): InOutRes DS:[4426] := 103, DX:AX := 0, STC, RET. Open: THREE LSEEKs, none checked for
 * carry: INT 21h AX=4201h CX:DX=0 (current), PUSH DX/AX; AX=4202h CX:DX=0 (end); POP BX/CX (the current position),
 * PUSH DX/AX (the end); AX=4200h CX:DX = the current position (restore); POP CX/BX (CX := end low, BX := end high);
 * RET with the restore's flags and DX:AX. Back in FilePos: JB 15A7 (carry from the restore, or STC); CX := recsize (f+4);
 * if CX != 1: BX := 0, PUSH CS, CALL 0A76 (LongDiv: DX:AX := DX:AX DIV BX:CX, signed); RETF 4.
 *
 * Declared environment input: INT 21h AH=42h at 15F2h, 1600h and 160Eh, in that order. Each service call receives the CPU at
 * the INT and returns the complete CPU after DOS returns at the INT + 2 (same CS, SS, SP, BP); only AH 42h is listed (ENV
 * otherwise); an LSEEK writes no RAM (DOS's frame below SP is outside the model).
 * DIVIDE: recsize 0 reaches LongDiv with divisor 0: its runtime-error path (AX := 00C8h, JMP 00E2, RunError 200) is an
 * EXIT-class stop, the CPU as LongDiv's model leaves it (CS:00E2).
 * TRANSFER: a RET or RETF that rereads its slot returns somewhere other than the call's next instruction (the helper's
 * InOutRes store lands on its own return slot when SS = DS and entry SP = 4428h); CS:IP is where it returned.
 * Exactly 1 MiB A20-off RAM; IP = 1593h; entry SP in [000Ah,FFF8h] (pushes reach entry SP - 0Ah inside LongDiv, the
 * argument entry SP + 7). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_filepos_cpu;

typedef enum {
    PR_FILEPOS_RETURNED = 0,  /* RETF 4 executed */
    PR_FILEPOS_ARGUMENT,      /* null RAM/CPU/service, RAM not exactly 1 MiB, IP not 1593h, SP outside [000Ah,FFF8h]; nothing changed */
    PR_FILEPOS_SERVICE,       /* an INT 21h service declined or returned a non-continuation; stopped at that INT */
    PR_FILEPOS_ENV,           /* an INT 21h function other than 42h would be issued (structural guard) */
    PR_FILEPOS_DIVIDE,        /* EXIT-class: recsize 0, LongDiv's divisor-0 runtime error 200 (CS:00E2, AX 00C8h) */
    PR_FILEPOS_TRANSFER       /* a reread return slot sent control elsewhere; CS:IP is where it went */
} pr_filepos_status;

typedef int (*pr_filepos_dos_service)(void *context, const pr_filepos_cpu *before, pr_filepos_cpu *after);

pr_filepos_status pr_system_filepos_1593(uint8_t *ram, size_t ram_size, pr_filepos_cpu *cpu, pr_filepos_dos_service service,
                                         void *context);
#endif
