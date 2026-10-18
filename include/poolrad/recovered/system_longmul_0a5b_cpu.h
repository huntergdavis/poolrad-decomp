#ifndef POOLRAD_RECOVERED_SYSTEM_LONGMUL_0A5B_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_LONGMUL_0A5B_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_SYSTEM_LONGMUL_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_longmul_cpu;

typedef enum {
    PR_SYSTEM_LONGMUL_RETURNED = 0,
    PR_SYSTEM_LONGMUL_ARGUMENT  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
} pr_system_longmul_status;

/* Raw-CPU, full-state model of the Turbo Pascal SYSTEM LongInt multiply at
 * loaded 0AF8:0A5B..0A75 (START.EXE expanded image, 27 bytes, 16
 * instructions), entered after a far CALL with SS:[SP] return IP and
 * [SP+2] return CS, the multiplicand a in DX:AX and the multiplier b in
 * BX:CX. SI := a.lo; DI := a.hi; MUL CX; PUSH AX; PUSH DX (the low partial
 * product p = a.lo * b.lo); MUL BX on a.lo gives BX := low word of a.lo *
 * b.hi; MUL CX on a.hi gives CX := low word of a.hi * b.lo; POP DX; POP AX;
 * ADD DX,BX; ADD DX,CX; RETF. DX:AX is the low 32 bits of a * b, the same
 * for signed and unsigned operands. The final flags are ADD DX,CX's; the
 * MUL flags and the first ADD's are overwritten and folded, and the other
 * flags keep their entry values. The two words of p stay in RAM below the
 * returned SP. Offsets wrap at 16 bits, physical addresses at 1 MiB; the
 * POPs and the RETF frame are read physically contiguous. */
pr_system_longmul_status pr_system_longmul_0a5b(uint8_t *ram, size_t ram_size, pr_system_longmul_cpu *cpu);

#endif
