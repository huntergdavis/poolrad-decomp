#ifndef POOLRAD_RECOVERED_SYSTEM_FILECHECK_14B6_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_FILECHECK_14B6_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_SYSTEM_FILECHECK_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_filecheck_cpu;
typedef enum {
    PR_SYSTEM_FILECHECK_RETURNED = 0,  /* the near RET executed: IP := the word at SS:[SP], SP += 2 (whatever IP that is) */
    PR_SYSTEM_FILECHECK_ARGUMENT       /* null RAM/CPU, RAM not exactly 1 MiB, IP not 14B6h, or SP above FFFCh; nothing changed */
} pr_system_filecheck_status;
/* Raw-CPU, full-state model of the Turbo Pascal SYSTEM file-magic check at START.EXE 0AF8:[14B6,14C5) (15 bytes, 4 instructions, near RET),
 * the helper shared by FIVE call sites in four SYSTEM file-I/O endpoints (Close 1491 at 1497, and 147B, 14D7, 150D, 1569). Runtime coverage
 * only (no identity claim); proved ONCE here and composed by every caller.
 *
 * Entered by a near CALL: [SP] is the return IP. ES:DI is the file variable ([handle WORD][magic WORD]; only the magic is read). The body is
 *   CMP WORD ES:[DI+2],D7B3h; JE 14C4; MOV WORD DS:[4426],0067h; RET.
 * When the magic differs from D7B3h (the file is not open for I/O) InOutRes := 0067h (103); either way the flags are those of the CMP, so the caller's
 * following JNE/JE branches on "magic != D7B3h". The RET REREADS its slot from RAM: a store that lands on SS:[SP] (SS == DS and SP = 4426h, i.e.
 * InOutRes overlaps the return slot) redirects the return, and the model returns that IP as is (the caller decides whether it expected it).
 * The offset DI+2 wraps at 16 bits inside ES; words are linear-contiguous with 1 MiB wrap; DS is the live caller value. Exactly 1 MiB of
 * A20-off RAM, CPU and RAM distinct host objects; SP in [0000,FFFC]; code immutable; no interrupts or faults. Flags: CMP word: CF, AF, OF, SF, ZF, PF. */
pr_system_filecheck_status pr_system_filecheck_14b6(uint8_t *ram, size_t ram_size, pr_system_filecheck_cpu *cpu);
#endif
