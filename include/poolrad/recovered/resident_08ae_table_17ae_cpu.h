#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_TABLE_17AE_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_TABLE_17AE_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_R08AE_17AE_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_17ae_cpu;
typedef enum { PR_R08AE_17AE_OK, PR_R08AE_17AE_ARGUMENT } pr_r08ae_17ae_status;
/* Original 08AE [17AE,181A), 108 bytes / 43 instructions, RETF 2: the four-row table writer. Entered after a far
 * CALL with SS:[SP] the return IP, [SP+2] the return CS and [SP+4] one WORD argument A (BP+6). PUSH BP; MOV BP,SP;
 * the body saves AX, BX, DI, SI and DS, loads DS from the live relocated segment immediate at CS:17B7 (the 08AE
 * data segment), stores A at DS:01FF, forms SI = 8*A by three ADD SI,SI, sets bytes DS:01D8=1 and DS:01D9=4 and DI=03C7.
 * For each of four rows: AX := word DS:[SI+0607]; when nonzero, clear the 24 words at DS:[DI+2Eh] down to [DI]
 * (BX = 2Eh, 2Ch, ..., 0), store AX at DS:[DI+2], store the low byte of the word DS:01FF re-read at that point to
 * DS:[0209+row] (row = 4 - DS:01D9), and store the word 1 at DS:[DI]; in every row DI += 30h, SI += 2 and the byte
 * DS:01D9 is decremented. At the end DS:01D8 := 0, the six saved registers are popped from live RAM and RETF 2 returns.
 * Every operand is reread from RAM in source order, so the stack, the table, the records and the control bytes may
 * alias: a later row's source can observe an earlier row's writes, and a store can change a saved register or the
 * return frame. Raw CPU14 over exactly 1 MiB A20-off RAM; entry IP=17AE; SP in [000C,FFFA] (the six pushes and the
 * RETF 2 frame neither wrap nor reach beyond the segment). Code immutable; no interrupts, faults or arbitrary-IP resume.
 * Offsets wrap at 16 bits; words are read and written physically contiguous (a word at offset FFFF straddles into the
 * next paragraph, as the original does). The final flags are those of the last DEC BYTE DS:01D9 (result 0) with CF kept
 * from the preceding ADD SI,2; no other flag is observable. The body makes no call and no port access, so ARGUMENT
 * (any violation, nothing changed) and OK are the only statuses. */
pr_r08ae_17ae_status pr_r08ae_table_17ae_cpu(uint8_t *ram, size_t ram_size, pr_r08ae_17ae_cpu *cpu);
#endif
