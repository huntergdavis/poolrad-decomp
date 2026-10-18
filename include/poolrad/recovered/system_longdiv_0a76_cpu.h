#ifndef POOLRAD_RECOVERED_SYSTEM_LONGDIV_0A76_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_LONGDIV_0A76_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_SYSTEM_LONGDIV_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_longdiv_cpu;

typedef enum {
    PR_SYSTEM_LONGDIV_RETURNED = 0,
    PR_SYSTEM_LONGDIV_ARGUMENT,   /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_SYSTEM_LONGDIV_DIVIDE_ERROR /* divisor 0: stopped at the runtime-error JMP target CS:00E2 with AX = 00C8h (error 200) */
} pr_system_longdiv_status;

/* Raw-CPU, full-state model of the Turbo Pascal SYSTEM LongInt divide at
 * loaded 0AF8:0A76..0AE3 (START.EXE expanded image, 110 bytes, 55
 * instructions), entered after a far CALL with SS:[SP] return IP and
 * [SP+2] return CS, the dividend in DX:AX and the divisor in BX:CX.
 * PUSH BP; if the divisor is 0: POP BP; AX := 00C8h; JMP 00E2 (runtime
 * error 200, which does not return; DIVIDE_ERROR stops there). Otherwise
 * PUSHF after OR BX,BX (F1) and negate a negative divisor; PUSHF after
 * OR DX,DX (F2) and negate a negative dividend; DI:SI := |divisor|,
 * BX:CX := 0; 33 restoring steps (RCL CX/BX; SUB/SBB; restore on borrow;
 * CMC; RCL AX/DX) leave the unsigned quotient in DX:AX and the remainder in
 * BX:CX; POPF F2 and negate the remainder for a negative dividend; POPF F1
 * and negate the quotient when the signs differ; POP BP; RETF. The final
 * flags are POPF F1's, or the quotient negation's ADC DX,0 flags. PUSHF
 * stores FLAGS with bit 1 set and bits 3, 5 and 15 clear and POPF loads
 * that canonical form, as a 386 does; the loop's RCL/SUB/SBB/ADD/ADC/CMC/DEC
 * flags are overwritten by POPF and are folded. The two FLAGS words stay in
 * RAM below the returned SP. Physical addresses wrap at 1 MiB; the RETF
 * frame is read physically contiguous. */
pr_system_longdiv_status pr_system_longdiv_0a76(uint8_t *ram, size_t ram_size, pr_system_longdiv_cpu *cpu);

#endif
