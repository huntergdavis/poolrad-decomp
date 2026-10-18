#ifndef POOLRAD_RECOVERED_RESIDENT_A40_DELAY_029C_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_A40_DELAY_029C_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_A40_DELAY_CPU_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_a40_delay_cpu;

typedef enum {
    PR_A40_DELAY_CPU_RETURNED = 0,
    PR_A40_DELAY_CPU_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 029Ch; nothing changed */
    PR_A40_DELAY_CPU_LIMIT      /* max_loopbacks backward jumps were taken; the next stops at its target */
} pr_a40_delay_cpu_status;

/* Raw-CPU, full-state model of the Turbo Pascal CRT Delay in START.EXE
 * resident 0A40:029C..02BC and its near spin helper 02BD..02C8 (45 bytes,
 * 19 instructions), the CPU/stack-effect form of the bounded
 * resident_a40_delay_029c.h. Entered after a far call with SS:[SP] the
 * return IP, [SP+2] the return CS and [SP+4] the millisecond count; RETF 2.
 *  1. MOV BX,SP; MOV DX,SS:[BX+4]; OR DX,DX; a zero count returns.
 *  2. MOV AX,40h; MOV ES,AX; XOR DI,DI; MOV AL,ES:[DI].
 *  3. Per millisecond: MOV CX,DS:[84FCh]; CALL 02BD, whose passes run
 *     MOV BX,4; DEC BX; JNE (three loop-backs), then CMP AL,ES:[DI];
 *     LOOPE 02BD, so up to CX passes (65536 when CX is 0) while the byte
 *     is unchanged; RET; DEC DX; JNE 02B0.
 *  4. RETF 2.
 * Every operand is reread from RAM in source order, and the CALL writes its
 * return word 02B7h at SS:SP-2 each millisecond, so a BIOS byte or count
 * that aliases it changes the loop as on the CPU; the RET pops that word.
 * The RETF frame is read physically contiguous; offsets wrap at 16 bits,
 * physical addresses at 1 MiB. Flags follow every instruction (OR and XOR
 * clear AF; DEC keeps CF). max_loopbacks caps the taken backward jumps (JNE
 * 02C0, LOOPE 02BD, JNE 02B0) and stops at the target with LIMIT; a
 * millisecond of CX passes takes 4 CX loop-backs. The code must sit at its
 * original offsets (IP = 029Ch). */
pr_a40_delay_cpu_status pr_a40_delay_029c_cpu(uint8_t *ram, size_t ram_size, pr_a40_delay_cpu *cpu, uint64_t max_loopbacks);

#endif
