#ifndef POOLRAD_RECOVERED_SYSTEM_RESET_1410_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_RESET_1410_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_RESET_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of Turbo Pascal SYSTEM Reset(var f: file; recsize: word), START.EXE 0AF8:1410 (the Reset entry
 * of the shared Reset/Rewrite body [1410,1475), 101 bytes; the Rewrite entry +1419 is a separate endpoint and is not
 * modelled here). Readiness coverage only, no identity claim (docs/re/SYSTEM_RESET_1410_CPU.md).
 *
 * Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] recsize, [SP+6] the far address of the FileRec f. AL := FileMode
 * DS:[442C]; AH := 3Dh; DX := 0. Then by the magic word f+2:
 *   D7B0h (closed)  -> open;   D7B3h (open) -> Close(f) first (near CALL 1491, a named stop), then open;
 *   anything else   -> InOutRes DS:[4426] := 102 (file not assigned) and return.
 * Open: if the name f+30h is empty, NO INT: XCHG DX,AX makes the handle 0 (the standard input handle);
 *       otherwise INT 21h AH=3Dh, AL=FileMode, DS:DX = f+30h. Carry: InOutRes := AX (the DOS error) and return.
 *       Success: magic := D7B3h, handle f+0 := AX, record size f+4 := recsize. POP BP; RETF 6.
 *
 * Declared environment input: INT 21h AH=3Dh (open). The service receives the CPU at the INT (IP 1456h) and returns the
 * complete CPU after DOS returns at 1458h (same CS, SS, SP, BP); only AH 3Dh is listed (any other AH is ENV); it must not
 * write RAM (DOS's own frame below SP is outside the model). The nested Close is a named stop (CLOSE): the model stops
 * with PUSH CS; CALL 1491 executed (return 1445h pushed). To continue after Close returns, call again with IP 1445h and the
 * post-Close state (any state inside the profile); entries are 1410h and 1445h, any other IP is ARGUMENT.
 * Exactly 1 MiB A20-off RAM; entry SP in [000Eh,FFF6h] at 1410h (resume: [0006h,FFF6h]). Words are linear-contiguous with
 * 1 MiB wrap; operands are reread from RAM in source order. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_reset_cpu;

typedef enum {
    PR_RESET_RETURNED = 0,  /* RETF 6 executed */
    PR_RESET_ARGUMENT,      /* null RAM/CPU/service, RAM not exactly 1 MiB, an IP other than 1410h/1445h, SP outside the guard;
                               nothing changed */
    PR_RESET_SERVICE,       /* the INT 21h service declined or returned a non-continuation; stopped at the INT (IP 1456h) */
    PR_RESET_ENV,           /* an INT 21h function other than 3Dh would be issued (structural guard); stopped at the INT */
    PR_RESET_CLOSE          /* the nested Close: PUSH CS; CALL 1491 executed; CS:IP = this CS:1491h */
} pr_reset_status;

/* Returns 0 with *after filled, or nonzero to decline. */
typedef int (*pr_reset_dos_service)(void *context, const pr_reset_cpu *before, pr_reset_cpu *after);

pr_reset_status pr_system_reset_1410(uint8_t *ram, size_t ram_size, pr_reset_cpu *cpu, pr_reset_dos_service service, void *context);
#endif
