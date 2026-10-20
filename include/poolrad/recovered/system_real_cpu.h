#ifndef POOLRAD_RECOVERED_SYSTEM_REAL_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_REAL_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_SYSTEM_REAL_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_real_cpu;

typedef enum {
    PR_SYSTEM_REAL_RETURNED = 0,
    PR_SYSTEM_REAL_ARGUMENT,  /* null RAM/CPU, RAM not exactly 1 MiB, IP not the entry, or SP outside
                                 [0040,FFFC]; nothing changed */
    PR_SYSTEM_REAL_RUNERROR,  /* the original reached its JMP to the SYSTEM RunError entry (+00E2): the
                                 model stops there with AX the error code and every effect so far kept */
    PR_SYSTEM_REAL_TRANSFER   /* a near RET popped a return address other than the call's next
                                 instruction; unreachable under the stated profile */
} pr_system_real_status;

/* Raw-CPU, full-state models of three Turbo Pascal SYSTEM 6-byte Real entry points in the loaded
 * SYSTEM segment (START.EXE expanded image, segment 0AF8), each entered after a far CALL with
 * SS:[SP] the return IP and [SP+2] the return CS, and each ending in RETF. A Real is three words:
 * exponent byte in the low byte of the first word, mantissa words after it.
 *
 *  - 0AF8:1186 (12 bytes of wrapper, calls the multiply core +0F33, 263 bytes): Real * Real. On
 *    CF set after the core the wrapper loads AX=00CDh (205, overflow) and jumps to +00E2.
 *  - 0AF8:119A (4 bytes, calls +10D9, 63 bytes): LongInt DX:AX to Real.
 *  - 0AF8:119E (14 bytes, sets CH=0 and calls +1118, 92 bytes): Real to LongInt, truncating. On CF
 *    set the wrapper loads AX=00CFh (207) and jumps to +00E2.
 *
 * Every instruction of each closure is modelled in source order, including the pushes of the near
 * CALL return IP and of the multiply core's saved registers, which stay in RAM below the returned
 * SP (the differential compares them). Flags follow DOSBox-X normal/386 at the pinned revision
 * 5a9c7c53: AND/OR/XOR/TEST clear CF, OF and AF; SHL/SHR by 1 set AF (OF is the result/operand
 * MSB relation); RCL/RCR set CF and OF only; INC/DEC keep CF; MUL byte and word set ZF=(low result
 * == 0), PF = even parity over all 16 (byte) or all 32 (word) result bits, CF=OF=(high part != 0),
 * and keep SF and AF. Offsets wrap at 16 bits and physical addresses at 1 MiB; the RETF frame is
 * read physically contiguous. Code is immutable; no interrupts, faults or segment-register writes
 * occur on any path (the bodies contain none). CPU and RAM are distinct host objects; the
 * RunError entry is a stop boundary, not modelled. */
pr_system_real_status pr_system_real_mul_1186(uint8_t *ram, size_t ram_size, pr_system_real_cpu *cpu);
pr_system_real_status pr_system_real_from_long_119a(uint8_t *ram, size_t ram_size, pr_system_real_cpu *cpu);
pr_system_real_status pr_system_real_trunc_119e(uint8_t *ram, size_t ram_size, pr_system_real_cpu *cpu);

#endif
