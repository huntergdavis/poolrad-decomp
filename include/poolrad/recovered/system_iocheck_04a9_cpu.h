#ifndef POOLRAD_RECOVERED_SYSTEM_IOCHECK_04A9_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_IOCHECK_04A9_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_SYSTEM_IOCHECK_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_iocheck_cpu;
typedef enum {
    PR_SYSTEM_IOCHECK_RETURNED = 0,  /* InOutRes == 0 and RETF executed: IP and CS popped, SP advanced by 4 */
    PR_SYSTEM_IOCHECK_ARGUMENT,      /* host or profile violation; nothing changed */
    PR_SYSTEM_IOCHECK_RUNERROR       /* InOutRes != 0: AX := InOutRes and the JMP to 0AF8:00E2 (RunError) executed; TERMINAL */
} pr_system_iocheck_status;
/* Raw-CPU, full-state model of the Turbo Pascal SYSTEM I/O check at START.EXE 0AF8:[04A9,04B7) (14 bytes, 5 instructions,
 * RETF), runtime coverage only (no identity claim). Entered after a far call: [SP] IP, [SP+2] CS. The body is
 * CMP WORD DS:[4426],0 (InOutRes); JNE; RETF; MOV AX,DS:[4426]; JMP 00E2.
 *
 * It RETURNS only when InOutRes is zero. For every non-zero InOutRes it never returns: it loads AX := InOutRes and transfers to
 * 0AF8:00E2, the RunError entry, which pops the caller's return address as ErrorAddr, prints "Runtime error" and terminates the
 * process through INT 21h AH=4Ch (see docs/re/SYSTEM_IOCHECK_04A9_CPU.md; the checker asserts that no path from 00E2 returns to the caller).
 * RUNERROR is that TERMINAL stop: the model has no resume entry, executes nothing at 00E2, and the state is the one at the JMP's
 * target (IP = 00E2, SP unchanged, AX = InOutRes). DS is the live caller value. Exactly 1 MiB of A20-off RAM, CPU and RAM distinct
 * host objects; SP in [0000,FFFC] (the RETF frame neither wraps nor leaves SS); code immutable; no interrupts or faults. Words are
 * physically contiguous. Flags are those of CMP word,0: CF, OF and AF clear, SF/ZF/PF from InOutRes; the RETF and JMP change none. */
pr_system_iocheck_status pr_system_iocheck_04a9(uint8_t *ram, size_t ram_size, pr_system_iocheck_cpu *cpu);
#endif
