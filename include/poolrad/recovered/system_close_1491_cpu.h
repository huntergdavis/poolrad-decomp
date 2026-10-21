#ifndef POOLRAD_RECOVERED_SYSTEM_CLOSE_1491_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_CLOSE_1491_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_SYSTEM_CLOSE_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_close_cpu;
typedef enum {
    PR_SYSTEM_CLOSE_RETURNED = 0,  /* RETF 4 executed */
    PR_SYSTEM_CLOSE_ARGUMENT,      /* null RAM/CPU/service, RAM not exactly 1 MiB, IP not 1491h, or SP outside [0002h,FFF8h]; nothing changed */
    PR_SYSTEM_CLOSE_SERVICE,       /* the INT 21h service refused or returned a non-continuation; stopped at the INT (IP 14A6), nothing after it applied */
    PR_SYSTEM_CLOSE_TRANSFER       /* the check helper's near RET popped an IP other than 149Ah (its return slot was overwritten); stopped there */
} pr_system_close_status;
/* Raw-CPU, full-state model of the Turbo Pascal SYSTEM Close(var f) at START.EXE 0AF8:[1491,14B6) (37 bytes, 13 instructions, RETF 4)
 * and its near check helper [14B6,14C5) (15 bytes, 4 instructions, RET), runtime coverage only (no identity claim). Entered after a far
 * call: [SP] IP, [SP+2] CS, then the far address of the file variable [SP+4] offset, [SP+6] segment. The variable is [handle WORD][magic WORD]:
 *   MOV BX,SP; LES DI,SS:[BX+4]; CALL 14B6: CMP WORD ES:[DI+2],D7B3h; when it differs, InOutRes (DS:4426) := 0067h (103); RET (the CALL
 *   pushes 149Ah at SS:[SP-2], which the model writes and rereads).
 *   Magic != D7B3h: RETF 4 with the magic unchanged. Otherwise BX := ES:[DI]; when BX > 4 (unsigned) AH := 3Eh and INT 21h; on CF
 *   InOutRes := AX; THEN, whatever the DOS result, ES:[DI+2] := D7B0h (closed); RETF 4. Handles 0..4 are never closed, but still marked closed.
 *
 * INT 21h AH=3Eh is a DECLARED environment input and the ONLY AH the body issues. The host service receives the CPU at the INT (IP 14A6,
 * AH 3Eh, BX = the handle) and returns the complete CPU at the continuation 14A8. DOS preserves every register but AX, and changes only CF
 * in the flags: the model refuses (SERVICE, stopped at the INT, nothing after it applied) a result whose CS, IP, SS, SP, BP, BX, CX, DX, SI,
 * DI, DS or ES differs from the INT-time value, whose IP is not 14A8, or whose flags differ in any bit but CF; AX and CF are the result (AX
 * is the DOS error code when CF is set). The service must not write RAM: DOS's own interrupt frame and stack use below SP are outside this model
 * (stated limit). Every operand is reread from RAM in source order, so a variable that overlaps InOutRes or the stack is followed as written.
 * SP in [0002h,FFF8h] so the helper's return slot and the 8-byte frame never wrap; code immutable; words are linear-contiguous with 1 MiB wrap. */
typedef int (*pr_system_close_service)(void *context, const pr_system_close_cpu *before, pr_system_close_cpu *after);
pr_system_close_status pr_system_close_1491(uint8_t *ram, size_t ram_size, pr_system_close_cpu *cpu, pr_system_close_service service, void *context);
#endif
